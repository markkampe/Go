/*
 * Module:
 *	mechanix.c
 *
 * Purpose:
 *	to keep track of the basic tactical implications of moves.
 *	basic tactical implications include liberties, groups and death.
 */

#include <stdio.h>
#include "go.h"
#include "move.h"
#include "dbg.h"
#include "disp.h"
#include "libs.h"
#include "strings.h"

int no_blunders = TRUE;		/* by default, forbid blunder moves */

/*
 * adjacent points array
 *	this structure is used to locate (in the string table, and on the
 *	board, the four points that are adjacent to a given point.  The
 *	offsets are to be applied (cumulatively) to the string table
 *	address.  The row and column of the adjustments are relative to
 *	the base position.  
 *
 * NOTE
 *	This array could be initialized statically, but we choose run-time
 *	for optimal performance.  The best order for visiting neighbors is
 *	machine dependent.  If neighbors are checked in order of decreasing
 *	pos.row_col, all liberty insertions will be at the beginning of the
 *	list, and more efficient than insertions at the middle or end.
 */
struct adjs
{	short 	str_offset;	/* offset from point to neighbor in str board */
	short	row_adj;	/* row adjustment to yield neighbor's posn */
	short	col_adj;	/* col adjustment to yield neighbor's posn */
} adjs[4];

/*
 * routine:
 *	m_reset
 *
 * purpose:
 *	to reinitialize the tactical data structures for a new game
 *
 */
void m_reset()
{	register int i, j;
	register struct string *sp;
	struct string *nsp;		/* MICROSOFT BLOWS DEAD RHINOS! */
	int row, col;
	pos_t a, b;

	/* initialize the string board */
	for( i = 0; i <= boardsize+1; i++ )
	{	str_board[0][i].s_moveno = -1;
		str_board[i][0].s_moveno = -1;
		str_board[boardsize+1][i].s_moveno = -1;
		str_board[i][boardsize+1].s_moveno = -1;
	}
	for( i = 1; i <= boardsize; i++ )
		for( j = 1; j <= boardsize; j++ )
			str_board[i][j].s_moveno = 0;

	/* initialize the various score related counters and state variables */
	w_kills = b_kills = 0;
	w_terr = b_terr = 0;
	dames = 0;
	fantasy = FALSE;
	vacancies = boardsize * boardsize;

	/*
	 * initialize the adjustments array to visit neighbors in the
	 * machine-optimal order.  First, however, we must determine
	 * what that order is.
	 */
	a.sub.row = 3; a.sub.col = 4;	/* higher numbered column */
	b.sub.row = 4; b.sub.col = 3;	/* higher numbered row */
	if (a.row_col > b.row_col)
	{	/* columns are more significant than rows */
		adjs[0].row_adj =  0; adjs[0].col_adj =  1;
		adjs[1].row_adj =  1; adjs[1].col_adj =  0;
		adjs[2].row_adj = -1; adjs[2].col_adj =  0;
		adjs[3].row_adj =  0; adjs[3].col_adj = -1;
	} else
	{	/* rows are more significant than columns */
		adjs[0].row_adj =  1; adjs[0].col_adj =  0;
		adjs[1].row_adj =  0; adjs[1].col_adj =  1;
		adjs[2].row_adj =  0; adjs[2].col_adj = -1;
		adjs[3].row_adj = -1; adjs[3].col_adj =  0;
	}

	/* now compute the string table offsets for that search order */
	sp = &str_board[4][4];
	for( i = 0; i < 4; i++ )
	{	row = 4 + adjs[i].row_adj;	/* #$! wimp microsoft C comp */
		col = 4 + adjs[i].col_adj;	/* can't handle all of these */
		nsp = &str_board[row][col];	/* lines of code if they are */
		j = nsp - sp;			/* combined into one expr.   */
		adjs[i].str_offset = j;		/* Fortunately, fast max nix */
		sp = nsp;			/* in this seldom-called rtn */
	}
}

/*
 * routine:
 *	m_move
 *
 * purpose:
 *	to check out, and possibly carry out a proposed move
 *
 * parameters:
 *	pointer to move structure for the move being considered
 *	NOTE: it is assumed that color and position are already filled out
 *
 * returns:
 *	TRUE	move OK
 *	FALSE 	move illegal (or foolish)
 */
int m_move( struct move *mp )
{	
	/* check for obviously illegal moves */
	if (str_board[mp->m_pos.sub.row][mp->m_pos.sub.col].s_moveno != 0)
	{	d_msg( "Illegal: position is not vacant" );
		return( FALSE );
	}
	if (mp->m_pos.row_col == mp[-1].m_ko.row_col)
	{	d_msg( "Illegal: attempt to fill KO" );
		return( FALSE );
	}
	
	/* figure out the implications of this move */
	m_estimate( mp );

	/* see if the move is flagrantly suicidal */
	if (mp->m_libs == 0  &&  mp->m_kills == 0)
	{	d_msg( "Illegal: played stone is dead" );
		if (TRACING( D_invalid ))
			fprintf(dbglog, 
				"ILLEGAL MOVE #%ld %c %c%d: kill 0, libs 0\n\n",
				mp - moves, mp->m_flags & M_COLOR ? 'w' : 'b', 
				'a'+mp->m_pos.sub.col-1, mp->m_pos.sub.row );
		return( FALSE );
	}

	/* see if move is patently foolish */
	if (no_blunders &&  (mp->m_flags & M_BLUNDER))
	{	d_msg( "Blunder: stone played into atari" );
		if (TRACING( D_invalid ))
			fprintf(dbglog, 
				"BLUNDER MOVE #%ld %c %c%d: kill 0, libs 1\n\n",
				mp - moves, mp->m_flags & M_COLOR ? 'w' : 'b', 
				'a'+mp->m_pos.sub.col-1, mp->m_pos.sub.row );
		return( FALSE );
	} 

	/* the move checks out as reasonable, so carry it out */
	m_do( mp );

	/* report on what has happened */
	if (verbose)
		d_msg("Results: %d liberties, %d stones killed, %d ataris", 
			mp->m_libs, mp->m_kills, mp->m_ataris );
	else if (mp->m_ataris  &&  !darkness)
		d_msg("Atari!");

	if (TRACING( D_moves ))
	{	fprintf(dbglog,
			"LOG MOVE #%ld %c %c%d: kill %d, atari %d, has %d libs",
			mp - moves, mp->m_flags & M_COLOR ? 'w' : 'b', 
			'a'+mp->m_pos.sub.col-1, mp->m_pos.sub.row, 
			mp->m_kills, mp->m_ataris, mp->m_libs );
		if (mp->m_flags&M_BLUNDER)
			fprintf( dbglog, ", BLUNDER!" );
		if (mp->m_netlib != 0)
			fprintf( dbglog, ", (est err %d)\n\n", mp->m_netlib);
		else
			fprintf( dbglog, "\n\n" );
	}

	return( TRUE );
}

/*
 * routine:
 *	m_estimate
 *
 * purpose:
 *	to estimate the implications of a move
 *
 * parameters:
 *	pointer to move struct for move to be estimated
 *		it is assumed that the m_pos field has been initialized
 *		to the position of the move, and that m_flags has been
 *		set to the color of stone being placed.
 *
 * notes:
 *	the m_libs count in the returned move structure does not
 *	include liberties that may be gained from kills - although
 *	there will surely be somewhere between m_gkills and m_kills
 *	liberties gained.  The m_netlib count does, however, include
 *	a rough estimate of the number of liberties to be gained from
 *	kills.  The actual determination of gained liberties is complex 
 *	and cannot be made without enumerating all of the neighbors of 
 *	all of the stones in the doomed groups.  This computation is
 *	too expensive for a cheap estimator, and will not be performed
 *	until m_do.  
 */
void m_estimate( struct move *mp )
{	unsigned char color = mp->m_flags & M_COLOR;	/* color making move */
	register struct adjs *ap;	/* pointer to neighbor offsets */
	register struct string *sp;	/* pointer into string board */
	register struct move *op;	/* pointer to neighbor move node */
	int m;				/* move number being considered */
	short *np;			/* pointer into m_neighbor array */
	pos_t pos;			/* address of adjacent liberty */

	/* start by initializing the move description */
	mp->m_stones = 1;
	mp->m_libs = 0;
	mp->m_netlib = 0;
	mp->m_ataris = 0;
	mp->m_kills = 0;
	mp->m_gkills = 0;
	mp->m_hurts = 0;
	mp->m_ko.row_col = 0;
	mp->m_neighbor[0] = 0; mp->m_neighbor[1] = 0; 
	mp->m_neighbor[2] = 0; mp->m_neighbor[3] = 0;
	mp->m_eyes = 0;	/* pity about this not being implemented */
	if (mp->m_liblist)
		l_free( mp );	/* this shouldn't happen */

	/*
	 * figure out where I am in the string table, and then
	 * cycle through my neighbor's string table entries to
	 * find out about my liberties, friends and ememies
	 */
	sp = &str_board[ mp->m_pos.sub.row ][ mp->m_pos.sub.col ];
	np = mp->m_neighbor;
	for( ap = adjs; ap < &adjs[4]; ap++ )
	{	/* find the next neighbor in the string table */
		sp += ap->str_offset;
		if ((m = sp->s_moveno) == 0)
		{	/* liberties should be added to this group */
			pos.sub.row = mp->m_pos.sub.row + ap->row_adj;
			pos.sub.col = mp->m_pos.sub.col + ap->col_adj;
			(void) l_gain( mp, pos );
		} else if (m > 0)
		{	/* determine whether it is hostile or friendly */
			op = &moves[ m ];
			if (sp->s_color == color)
			{	/* friendly group - make sure its a new one */
				if (mp->m_stones > 1)
				{	if (mp->m_neighbor[0] == m)
						continue;
					if (mp->m_neighbor[1] == m)
						continue;
					if (mp->m_neighbor[2] == m)
						continue;
				}

				/*
				 * not already known, set up the merge
				 * note that we subtract each groups liberties
				 * from m_netlib, because that is the number
				 * of liberties we started with.
				 */
				mp->m_stones += op->m_stones;
				mp->m_netlib -= op->m_libs;
				*np++ = m;
				(void) l_merge( mp, op );
			} else
			{	/* hostile group - make sure its a new one */
				m *= -1;
				if (mp->m_hurts > 0)
				{	if (mp->m_neighbor[0] == m)
						continue;
					if (mp->m_neighbor[1] == m)
						continue;
					if (mp->m_neighbor[2] == m)
						continue;
				}

				/* not already known, set up the impingement */
				if (op->m_libs == 1)
				{	/* we will kill this group */
					mp->m_kills += op->m_stones;
					mp->m_gkills++;
					mp->m_ko.row_col = op->m_pos.row_col;
				} else if (op->m_libs == 2)
				{	/* we will atari this group */
					if (mp->m_ataris)
						mp->m_flags |= M_MULTIPLE;
					mp->m_ataris += op->m_stones;
				}
				*np++ = m;
				mp->m_hurts++;
			}
		}
	}

	/* in a merge, there is a danger of counting myself as a liberty */
	if (mp->m_stones > 1)
		(void) l_lose( mp, mp->m_pos );

	/* add new groups liberties to the (probably negative) net */
	mp->m_netlib += mp->m_libs;

	/*
	 * Hazzard a crude estimate at the liberties gained by our kills.
	 *
	 * The heuristic hand waving says that a kill of four or fewer
	 * stones probably buys as many liberties for the killer.  In a
	 * larger group, some of the corpses may have been interior and
	 * will not buy any liberties for the killer.  We claim that about
	 * one third of the points in a large group are internal.
	 */
	mp->m_netlib += (mp->m_kills < 5) ? mp->m_kills
					  : (((mp->m_kills + 1) << 1) / 3);

	/* 
	 * we have noted the position of the last killed neighbor in m_ko.
	 * If this isn't really a ko situation, we should clear m_ko.
	 */
	if (mp->m_libs != 0  ||  mp->m_stones != 1  ||  mp->m_kills != 1)
		mp->m_ko.row_col = 0;

	/* see if it is patently foolish */
	if (mp->m_libs < 2  &&  mp->m_kills == 0)
		mp->m_flags |= M_BLUNDER;

	/*
	 * If I really want alot of output, I can log all estimates 
	 */
	if (TRACING( D_est ))
	{      fprintf(dbglog,"ESTIMATE #%ld %c %c%d: kill %d, atari %d, lib +%d\n",
			mp-moves, mp->m_flags & M_COLOR ? 'w' : 'b', 
			'a'+mp->m_pos.sub.col-1, mp->m_pos.sub.row, 
			mp->m_kills, mp->m_ataris, mp->m_netlib );
		if (TRACING( D_affected ))
		for( np = mp->m_neighbor; *np && np < &mp->m_neighbor[4]; np++ )
		{	if (*np > 0)
			{	op = &moves[ *np ];
				fprintf( dbglog, "    ABSORB %c%d, %d stones\n",
				    'a'+op->m_pos.sub.col-1, op->m_pos.sub.row, 
				    op->m_stones );
			} else
			{	op = &moves[ -(*np) ];
				fprintf( dbglog, "    ATTACK %c%d, %d stones\n",
				    'a'+op->m_pos.sub.col-1, op->m_pos.sub.row, 
				    op->m_stones );
			}
		}
	}
}

void m_label( struct move *mp, int value );
int m_zap( struct move *mp );

/*
 * routine:
 *	m_do
 *
 * purpose:
 *	to carry out the implications of a previously estimated move
 *
 * parameters:
 *	pointer to move to be carried out (as filled out by m_estimate)
 *
 * returns:
 *	void
 *
 * notes:
 *	the m_libs count in the estimate includes all liberties in the
 *	(soon-to-be) subsumed groups, but does not include any of the 
 *	liberties that may be gained from kills.
 *
 *	since this is the routine that actually performs moves, it is
 *	our responsibility to see that the display board gets updated.
 *	This routine, and the routines under it, will refrain from
 *	updating the display board if the "fantasy move" flag is
 *	set - indicating that we are performing tactical analysis rather 
 *	than real moves.
 */
void m_do( struct move *mp )
{	register struct move *np;
	register short *sp;
	register short s;

	/* note the placement of a new stone on the display board */
	if (!fantasy)
	{	(void)b_set( mp->m_pos.sub.row, mp->m_pos.sub.col, 
			mp->m_flags&M_COLOR, B_NORMAL, B_NORMAL, mp - moves);
		vacancies--;
	}

	/* coalesce all of the subsumed stones into a single string */
	m_label( mp, mp-moves );

	/* reset m_netlib to estimaged gains from kills, so we can check est */
	mp->m_netlib = -( mp->m_gkills );

	/* examine the damage done to adjacent opposing strings */
	for( sp = mp->m_neighbor; (sp < &mp->m_neighbor[4]) && (s = *sp++); )
	{	if (s < 0)
		{	/* a hostile neighbor to impinge upon, maybe kill */
			np = &moves[ -s ];
			if (l_lose( np, mp->m_pos )  &&  np->m_libs == 0)
			{	mp->m_netlib += m_zap( np );

				if (TRACING( D_kills ))
				{      fprintf( dbglog,
						"KILL %c %c%d: %d stones\n",
						np->m_flags&M_COLOR ? 'w' : 'b',
						'a'+np->m_pos.sub.col-1, 
						np->m_pos.sub.row, 
						np->m_stones );
				}
			}
		}
	}

	/* note, also, the influence implications of this move */
	delta_inf(mp->m_pos.sub.row, mp->m_pos.sub.col, mp->m_flags&M_COLOR, 1);
}

/*
 * routine:
 *	m_unmove
 *
 * purpose:
 *	to rescind a previously made move
 *
 * parameters:
 *	pointer to move to be rescinded
 *
 * returns:
 *	void
 *
 * notes:
 *	since this is the routine that actually performs moves, it is
 *	our responsibility to see that the display board gets updated.
 *	This routine, and the routines under it, will refrain from
 *	updating the display board if the "fantasy move" flag is
 *	set - indicating that we are performing tactical analysis rather 
 *	than real moves.
 */
void m_unmove( struct move *mp )
{	register struct move *np;
	register short *sp;
	register short s;

	/* start by clearing the position occupied by the move */
	str_board[ mp->m_pos.sub.row ][ mp->m_pos.sub.col ].s_moveno = 0;
	if (!fantasy)
	{	b_remove( mp->m_pos.sub.row, mp->m_pos.sub.col );
		vacancies++;
	}

	/* note, also, the influence implications of this removal */
	delta_inf(mp->m_pos.sub.row, mp->m_pos.sub.col, mp->m_flags&M_COLOR,-1);

	if (TRACING( D_moves ))
	{	fprintf(dbglog,
			"LOG UNMOVE #%ld %c %c%d: un-kill %d, un-atari %d\n",
			mp - moves, mp->m_flags & M_COLOR ? 'w' : 'b', 
			'a'+mp->m_pos.sub.col-1, mp->m_pos.sub.row, 
			mp->m_kills, mp->m_ataris );
	}

	/* examine the damage done to adjacent opposing strings */
	for( sp = mp->m_neighbor; (sp < &mp->m_neighbor[4]) && (s = *sp++); )
	{	if (s < 0)
		{	/* a hostile neighbor regains a liberty or even life */
			np = &moves[ -s ];
			if (TRACING( D_affected ))
			{    fprintf( dbglog, "    RETREAT %c%d, %d stones\n",
				    'a'+np->m_pos.sub.col-1, np->m_pos.sub.row, 
				    np->m_stones );
			}
			if (l_gain( np, mp->m_pos )  &&  np->m_libs == 1)
			{	m_restore( np, -s, mp-moves );
				if (TRACING( D_kills ))
				{      fprintf( dbglog,
						"UNKILL %c %c%d: %d stones\n",
						np->m_flags&M_COLOR ? 'w' : 'b',
						'a'+np->m_pos.sub.col-1, 
						np->m_pos.sub.row, 
						np->m_stones );
				}
			}
		}
	}

	/* return all subsumed groups to individuality */
	for( sp = mp->m_neighbor; (sp < &mp->m_neighbor[4]) && (s = *sp++); )
	{	if (s > 0)
		{	m_label( &moves[s], s );
			if (TRACING( D_affected ))
			{	np = &moves[s];
				fprintf( dbglog, "    FRAGMENT %c%d, %d stones\n",
				    'a'+np->m_pos.sub.col-1, np->m_pos.sub.row, 
				    np->m_stones );
			}
		}
	}

	/* free my liberty list */
	l_free( mp );

}


#ifndef FAST	/* these routines should all be in assembler */
/*
 * routine:
 *	m_zap
 *
 * purpose:
 *	to remove (from the string board) all points under a killed tree
 *
 * parameters:
 *	root of the tree to be killed
 *
 * returns:
 *	number of liberties gained by killer
 */
int m_zap( struct move *mp )
{	register struct string *sp;
	register struct adjs *ap;
	register short *np;
	int newlibs = 0;
	unsigned color = mp->m_flags & M_COLOR;

	/* take me off of the string board */
	str_board[ mp->m_pos.sub.row ][ mp->m_pos.sub.col ].s_moveno = 0;

	/* credit the kill for score purposes */
	if (color == BLACK)
		w_kills++;
	else
		b_kills++;

	/* note, also, the influence implications of this removal */
	delta_inf( mp->m_pos.sub.row, mp->m_pos.sub.col, color, -1 );

	/* take me off of the display board */
	if (!fantasy)
	{	b_remove( mp->m_pos.sub.row, mp->m_pos.sub.col ); 
		vacancies++;
	}

	/* recursively invoke myself on the subsumed substrings */
	for( np = mp->m_neighbor; np < &mp->m_neighbor[4] && *np; np++ )
	{	if (*np > 0)
			newlibs += m_zap( &moves[ *np ] );
	}

	/*
	 * figure out where I am in the string table, and then
	 * cycle through my neighbor's string table entries to
	 * find out which ones are hostiles who benefit from my demise
	 */
	sp = &str_board[ mp->m_pos.sub.row ][ mp->m_pos.sub.col ];
	for( ap = adjs; ap < &adjs[4]; ap++ )
	{	/* find the next neighbor in the string table */
		sp += ap->str_offset;
		if (sp->s_moveno > 0  &&  sp->s_color != color)
			newlibs += l_gain( &moves[ sp->s_moveno ], mp->m_pos );
	}

	return( newlibs );
}

/*
 * routine:
 *	m_restore
 *
 * purpose:
 *	to restore (to the string board) a previously dead tree
 *
 * parameters:
 *	root of the tree to be restored to the board
 *	move number of the tree top
 *	move number of the group that originally killed it 
 *
 * returns:
 *	void
 *
 * note:
 *	since we know the killer will disappear soon, we can save ourselves
 *	the work of impinging upon his liberties as our stones reappear
 */
void m_restore( struct move *mp, int value, int killer )
{	register struct string *sp;
	register struct adjs *ap;
	register short *np;
	unsigned color = mp->m_flags & M_COLOR;

	/* replace me on the display board */
	if (!fantasy)
	{	(void)b_set( mp->m_pos.sub.row, mp->m_pos.sub.col, 
			color, B_NORMAL, B_NORMAL, mp - moves);
		vacancies--;
	}

	/* un-credit the kill */
	if (color == BLACK)
		w_kills--;
	else
		b_kills--;

	/* note the influence implications of this replacement */
	delta_inf( mp->m_pos.sub.row, mp->m_pos.sub.col, color, 1 );

	/*
	 * figure out where I am in the string table, and then
	 * cycle through my neighbor's string table entries to
	 * find out which ones are hostiles on whom I will impinge
	 */
	sp = &str_board[ mp->m_pos.sub.row ][ mp->m_pos.sub.col ];
	for( ap = adjs; ap < &adjs[4]; ap++ )
	{	/* find the next neighbor in the string table */
		sp += ap->str_offset;
		if (sp->s_moveno > 0  &&  sp->s_color != color 
				      &&  sp->s_moveno != killer)
			(void) l_lose( &moves[ sp->s_moveno ], mp->m_pos );
	}

	/* recursively invoke myself on the subsumed substrings */
	for( np = mp->m_neighbor; np < &mp->m_neighbor[4] && *np; np++ )
	{	if (*np > 0)
			m_restore( &moves[ *np ], value, killer );
	}

	/* re-label my position on the string board */
	sp = &str_board[ mp->m_pos.sub.row ][ mp->m_pos.sub.col ];
	sp->s_moveno = value;
	sp->s_color = color;
}

/*
 * routine:
 *	m_label
 *
 * purpose:
 *	to label (on the string board) all points under a tree with its root
 *
 * parameters:
 *	root of the tree to be m_labeled
 *	move number in which the root of the tree was placed
 *
 * returns:
 *	void
 */
void m_label( struct move *mp, int value )
{	register struct string *sp;
	register short *ap;

	/* re-label my position on the string board */
	sp = &str_board[ mp->m_pos.sub.row ][ mp->m_pos.sub.col ];
	sp->s_moveno = value;
	sp->s_color = mp->m_flags & M_COLOR;

	/* recursively invoke myself on the subsumed substrings */
	for( ap = mp->m_neighbor; ap < &mp->m_neighbor[4] && *ap; ap++ )
	{	if (*ap > 0)
			m_label( &moves[ *ap ], value );
	}
}
#endif 	/* !FAST */

/*
 * routine:
 *	m_showcount
 *
 * purpose:
 *	to display all stones of a particular color with their group
 *	liberty counts
 *
 * parms:
 *	color to be displayed
 */
void m_showcount( char who )
{	register unsigned r, c;
	register struct move *mp;
	unsigned color;
	int count, eyes, shade, s;

	for( r = 1; r <= boardsize; r++ )
		for( c = 1; c <= boardsize; c++ )
		{	/* skip over empty spots */
			if ((s = str_board[r][c].s_moveno) == 0)
				continue;

			/* skip over the color he doesn't want to see */
			color = str_board[r][c].s_color;
			if (color == WHITE)
			{	if (who == 'b')
					continue;
			} else if (who == 'w')
				continue;

			/* assess this guy's liberty situation */
			mp = &moves[s];
			count = mp->m_libs;
			eyes = 0;	/* FIX ME */
			if (eyes >1 || count > 6)
				shade = B_VITAL;
			else if (count <= 2)
				shade = B_PERRIL;
			else
				shade = B_WALL;

			/* display this guy's liberty situation */
			d_value( r, c, color, shade, count );
		}

}

/*
 * routine:
 *	m_showstones
 *
 * purpose:
 *	to display all of the stones in a group in some color
 *
 * parms
 *	pointer to move at head of group
 *	background shade
 */
void m_showstones( struct move *mp, int shade )
{	register short *np;

	d_stone( mp->m_pos.sub.row, mp->m_pos.sub.col, 
		mp->m_flags&M_COLOR, shade, B_NORMAL );

	for( np = mp->m_neighbor; *np && np < &mp->m_neighbor[4]; np++ )
	{	if (*np > 0)
			m_showstones( &moves[ *np ], shade );
	}
}

/*
 * routines:
 *	m_showlibs
 *
 * purpose:
 *	to display the liberties of a group in some shade
 *
 * parms:
 *	pointer to move node for the group
 *	background shade
 */
void m_showlibs( struct move *mp, int shade )
{	register struct libs *lp;

	for( lp = mp->m_liblist; lp; lp = lp->l_next )
	{	d_blank( lp->l_pos.sub.row, lp->l_pos.sub.col, 
			mp->m_flags&M_COLOR, shade );
	}
}

/*
 * routine: 
 *	c_info
 *
 * purpose:
 *	to handle queries about strategic and tactical matters
 *
 * argument:
 *	NULL		liberty cout for all groups
 *	l		liberty cout for all groups
 *	b		liberty count for black groups
 *	w		liberty count for white groups
 *	position	liberties and stones for particular group
 */
 void c_info( char *arg )
{	register int m;
	register struct move *mp;
	pos_t pos;

	/* use default if necessary */
	if (arg == 0 || *arg == 0)
		arg = "l";

	/* arg is either a single letter or a position */
	if (arg[1] == 0)
	{	/* liberty counts for lots of people */
		m_showcount( arg[0] );
	} else
	{	/* information on group at a position */
		pos.row_col = chkmove( arg);
		if (pos.row_col == 0)
			return;

		m = str_board[ pos.sub.row ][ pos.sub.col ].s_moveno;
		if (m <= 0)
		{	d_msg( "Position %c%d: blank", 'a'+pos.sub.col-1, 
				pos.sub.row );
			return;
		} else
			mp = &moves[m];

		/* highlight the selected group on the screen */
		m_showstones( mp, B_WALL );
		m_showlibs( mp, B_TERRITORY );

		d_msg( "Position %c%d: %d %s stones, %d liberties (%d eyes)",
			'a'+pos.sub.col-1, pos.sub.row, mp->m_stones,
			(mp->m_flags & M_COLOR) ? "white" : "black", 
			mp->m_libs, 0 );
	}

	(void) confirm( "Enter a newline to resume game" );
	b_redraw( FALSE );
}

