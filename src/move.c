/*
 * module:
 *	move.c
 *
 * purpose:
 *	basic movement commands and utilities
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "go.h"
#include "disp.h"
#include "move.h"

/*
 * routine:
 *	c_newgame
 *
 * purpose:
 *	to reset all data structures to scratch for a new game
 *
 * parms:
 *	size of the board to be used
 */
void c_newgame( int bsize )
{	extern char *nicedate();

	/* reset the basic identification information */
	(void) strcpy( gamename, nicedate( time((time_t *) 0) ) );
	d_header( "" );

	b_boardsize( bsize ); /* create board of the appropriate size */

	h_reset();	/* clear out the move history */
	b_reset();	/* reset the display board */
	m_reset();	/* reset the mechanical boards */
	l_reset();	/* reset the liberty lists */
	i_reset();	/* reset the influence board */
}

/*
 * routine:
 *	h_reset
 *
 * purpose:
 *	to reset the move history for a new game
 */
void h_reset()
{	register struct move *mp;
	register int *ip;

	/* zero out the move history */
	mp = &moves[MAXMOVE];	/* stop at end of the move table */
	ip = (int *) moves;	/* start of the move table */
	while( ip < (int *) mp )
		*ip++ = 0;

	movenum = 1;
	nxt_color = BLACK;
	spotpoints = 0;
}

/*
 * routine:
 *	c_move
 *
 * purpose:
 *	to process a move command from the user
 *
 * parms:
 *	color of stone 
 *	position (<row><col> or <col><row>)
 *	flags to be set in move
 */
void c_move( int color, char *position, int flags )
{	register struct move *mp = &moves[ movenum ];

	/* note whose move this is going to be */
	mp->m_flags = color * M_COLOR;

	if (position == 0  ||  *position == '-')
	{	/* pass moves are particularly easy to handle */
		mp->m_pos.row_col = 0;
		mp->m_stones = 0;
		mp->m_libs = 0;
		mp->m_ko.row_col = 0;
	} else	
	{	/* make sure the move is syntactically valid and non-absurd */
		mp->m_pos.row_col = chkmove( position );
		if (mp->m_pos.row_col == 0)
			return;

		/* do a quick tactical analysis of the move */
		if (!m_move( mp ))
			return;

		/* make special note of handicap moves */
		if (flags & M_HANDICAP)
		{	spotpoints++;
			mp->m_flags |= M_HANDICAP;
		}
	} 

	/* update the move log */
	m_logmove( (FILE *) NULL, movenum );
	d_update();

	/* figure out whose move it is next */
	nxt_color = (color == WHITE) ? BLACK : WHITE;
	movenum++;
}

/*
 * routine:
 *	c_unmove
 *
 * purpose:
 *	to allow the user to rescind moves
 *
 * parms:
 *	number of moves to be undone
 */
void c_unmove( int nmove )
{	register int i = nmove;
	register struct move *mp;
	int save;

	/* suppress screen updates if we are going to undo alot */
	if (nmove > 2)
	{	save = darkness;
		darkness = TRUE;
	}

	/* back out the specified number of moves */
	while( i--  &&  movenum > 1 )
	{	/* make sure this move is reversible */
		if (movenum <= pillage + 1)
		{	d_msg( "Impossible to undo beyond move %d", movenum-1 );
			break;
		}

		/* back up our idea of whose turn it is */
		mp = &moves[ --movenum ];
		nxt_color = mp->m_flags & M_COLOR;

		/* undo the tactical implications of the move */
		if (mp->m_pos.row_col)
			m_unmove( mp );

		/* void the move in the internal move record */
		mp->m_pos.row_col = 0;
		mp->m_flags = 0;

		/* take the move out of the displayed game history */
		if (!darkness)
			d_showmove( movenum, "...         " );
	}

	/* and update the screen (if we suppressed updates) */
	if (nmove > 2)
	{	darkness = save;
		b_redraw( TRUE );
	}
}

/*
 * routine:
 *	m_logmove
 *
 * purpose:
 *	to log a move, either to a game record or to the screen
 *
 * parameters:
 *	FILE for log file (NULL -> screen)
 *	move number to be logged
 */
void m_logmove( FILE *file, int num )
{	char row, col, color;
	register struct move *mp;
	char buf[32];

	mp = &moves[ num ];
	row = mp->m_pos.sub.row;
	col = 'a' + mp->m_pos.sub.col - 1;
	if (mp->m_flags & M_HANDICAP)
		color = 'B';
	else
		color = (mp->m_flags & M_COLOR) ? 'w' : 'b';

	if (file)
	{	if (row > 0 && col > 0)
			fprintf(file,"%c: %c%d\t(move %d",color,col,row,num );
		else
			fprintf(file,"%c: -\t(move %d, PASS", color, num );
		if (mp->m_ataris)
			fprintf( file, ", atari" );
		if (mp->m_ko.row_col)
			fprintf( file, ", ko" );
		if (mp->m_flags&M_BLUNDER)
			fprintf( file, ", blunder" );
		fprintf( file, ")\n" );
	} else if (darkness == FALSE)
	{	if (row > 0 && col > 0)
			(void) sprintf( buf, "%2d: %c %c%d %c", num, color, 
				col, row, (mp->m_ataris) ? '!' : ' ' );
		else
			(void) sprintf( buf, "%2d: %c PASS", num, color );
		d_showmove( num, buf );
	}
}

/*
 * routine:
 *	c_handicap
 *
 * purpose:
 *	to give black some handicap
 *
 * parms:
 *	number of handicap stones desired
 *
 * note:
 *	this routine does its job in a rather strange way.  I defense
 *	I offer that it can handle arbitrary handicap patterns on
 *	boards of arbitrary size, and that this code is executed
 *	for at most a few iterations in each game.  I needed to know where
 *	the hoshi points were in order to draw the board, so I used the
 *	same board to locate all of the other handicap points.
 */
void c_handicap( int num )
{	register int i, row, col;
	char hmoves[MAXHAND][4];	/* buffer for handicap moves */

	if (movenum > 1)
	{	d_msg( "handicaps only possible at start of game" );
		return;
	}

	/* find the appropriate handycap pattern */
	if (num <= 0 || num > num_handi)
	{	d_msg( "ridiculous handicap - %d assumed", num_handi );
		num = num_handi;
	}

	/* search the handicap board and find out all known handicap points */
	for( row = 1; row <= boardsize; row++ )
		for( col = 1; col <= boardsize; col++ )
			if (i = hnd_board[row][col])
			{	hmoves[i][0] = 'a' + col - 1;
				hmoves[i][1] = '0' + (row/10);
				hmoves[i][2] = '0' + (row%10);
				hmoves[i][3] = 0;
			}

	/*
	 * this is code is bizzare, but attractive isn't always
	 * simple.  Even handicaps over four stones exclude the
	 * center point, while odd ones include it.  This results
	 * in more attractive (symmetric) handicap patterns.  It is
	 * a terrible kluge that lets me know that hoshi #5 is the
	 * center.  I would appreciate a more elegant implementation.
	 */
	if (num > 5 && (num&1) == 0)
	{	hmoves[5][0] = 0;
		num++;
	}

	/* now that we know what stones to place, lets to it */
	for( i = 1; i <= num; i++ )
	{	if (hmoves[i][0])
			c_move( BLACK, hmoves[i], M_HANDICAP );
	}
}

/*
 * routine:
 *	c_spot
 *
 * purpose:
 *	to make a particular (non-generic) handicap move - handles the B cmd
 *
 * parms:
 *	position of move
 */
void c_spot( char *arg )
{
	c_move( BLACK, arg, M_HANDICAP );
}

/*
 * routine:
 *	chkmove
 *
 * purpose:
 *	check a move for syntactic reasonableness or flagrant illegality
 *
 * parms:
 *	position for new move (char *)
 *
 * returns:
 *	postion
 *	0 - move was invalid
 */
int chkmove( char *arg )
{	register int row = 0;
	register int col = 0;
	pos_t pos;

	/* figure out whether the postion is row-first or col-first */
	if (*arg >= 'a' && *arg <= 's')
	{	/* column first */
		col = 1 + *arg++ - 'a';

		while( *arg >= '0' && *arg <= '9' )
		{	row *= 10;
			row += *arg - '0';
			arg++;
		}
	} else
	{	/* row first */
		while( *arg >= '0' && *arg <= '9' )
		{	row *= 10;
			row += *arg - '0';
			arg++;
		}

		if (*arg >= 'a' && *arg < 'z')
			col = 1 + *arg - 'a';
	}

	/* check them for reasonableness */
	if (row <= 0 || row > boardsize)
	{	d_msg( "*** illegal row designation" );
		return( 0 );
	}
	if (col <= 0 || col > boardsize)
	{	d_msg( "*** illegal column designation" );
		return( 0 );
	}

	/* looks like a legal move to me */
	pos.sub.row = row;
	pos.sub.col = col;
	return( pos.row_col );
}
