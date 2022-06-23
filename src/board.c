/*
 * module:
 *	board.c
 *
 * purpose:
 *	GO - basic board management functions
 */
#include <stdio.h>
#include "go.h"
#include "disp.h"

/*
 * routine:
 *	b_reset
 *
 * purpose:
 *	to reset the board to be empty
 */
 void b_reset()
 {	register int i;
	register unsigned short *p, *e;

	/* reset the board */
	i = sizeof dsp_board / sizeof (*p);
	p = &dsp_board[0][0];
	e = &p[i];
	while( p < e )
		*p++ = 0;

	b_redraw( TRUE );
 }

/*
 * routine:
 *	b_set
 *
 * purpose:
 *	to set a particular board position
 *
 * parameters:
 *	row, col, 
 *	color
 *	stone type
 *	background
 *	move number
 *
 * returns:
 *	previous contents
 */
unsigned short b_set( unsigned row, unsigned col, unsigned color, int type, int shade, int moveno )
 {	register short save, new;

 	save = dsp_board[ row-1 ][ col-1 ];

	/* update the board */
	new = ((color*B_COLOR) | type | shade | moveno);
	dsp_board[ row-1 ][ col-1 ] = new;

	/* update the screen */
	b_update( row, col );

	return( save );
}


/*
 * routine:
 *	b_tag
 *
 * purpose:
 *	to set tags associated with a position
 *
 * parameters:
 *	row, col
 *	type
 *	shade
 *
 * returns:
 *	previous contents
 */
unsigned short b_tag( unsigned row, unsigned col, int type, int shade )
 {	register short save, new;

 	save = dsp_board[ row-1 ][ col-1 ];

	/* update the board */
	new = save & ~(B_SHADE+B_TYPE);
	new |= shade + type;
	dsp_board[ row-1 ][ col-1 ] = new;

	/* update the screen */
	b_update( row, col );

	return( save );
}

/*
 * routine:
 *	b_remove
 *
 * purpose:
 *	to remove a dead stone from the board
 *
 * parmameters:
 *	row, col
 *
 * returns:
 *	void
 */
 void b_remove( unsigned row, unsigned col )
{
	dsp_board[row-1][col-1] = 0;
	b_update( row, col );
}

/*
 * routine:
 *	b_update
 *
 * purpose:
 *	to update a position on the screen
 *
 * parameters:
 *	row, col to be updated
 */
 void b_update( unsigned row, unsigned col )
 {	register short value;
	unsigned color;
	int shade, num;

	if (darkness)
		return;

	/* find out about the indicated board position */
	value = dsp_board[ row-1 ][ col-1 ];
	color = B_WHT_STONE( value ) ? WHITE : BLACK;
	num   = B_MOVENUM( value );
	shade = value & B_SHADE;

	/* see if we need to put a stone on the spot */
	if (num)
	{	/* figure out how to label it */
		switch( b_display )
		{ case BY_TYPE:
			num = value & B_TYPE;
			d_stone( row, col, color, shade, B_TYPE );
		  	break;

		  case BY_MOVE:
			if (num > b_basemove)
			{	d_stone(row, col, color, shade, num-b_basemove);
				break;
			}

		  default:
		  	d_stone( row, col, color, shade, 0 );
		}
	} else
		d_blank( row, col, BLACK, shade );
 }

/*
 * routine:
 *	b_redraw
 *
 * purpose:
 *	to reconstruct the entire board screen
 *
 * parms:
 *	whether to redraw entire screen, or merely the board
 */
void b_redraw( int all )
 {	register unsigned row, col;
 	register int i;
	extern int d_histlen;	/* imported from display module */

	if (darkness)
		return;

	if (all)
	{	/* start by clearing the screen */
		d_clear();

		/* re-display the last screen-full of move history */
		if (movenum-1 > d_histlen)
			i = movenum - d_histlen;
		else
			i = 1;

		while( i < movenum )
			m_logmove( (FILE *) NULL, i++ );
	}

	/* initialize every point on the blank board */
	for( row = 1; row <= boardsize; row++ )
		for( col = 1; col <= boardsize; col++ )
			b_update( row, col );

	/* and force it all to really happen */
	d_update();
}

/*
 * routine:
 *	b_boardsize
 *
 * purpose:
 *	to set the size of the board and locate the handicap points
 *
 * note:
 *	we initialize handicap points in reverse order so that 
 *	on smaller boards, overlapping points will get the smaller
 *	numbers
 */
void b_boardsize( int size )
{	
	register int row, col;
	int min_ha;	/* near fourth line */
	int max_ha;	/* far fourth line */
	int min_san;	/* near third line */
	int max_san;	/* far third line */
	int middle;	/* middle line */		
	int min_huh;	/* near quarter line */
	int max_huh;	/* far quarter line */

	if (size > MAXBOARD || size < 5)
	{	d_msg("Ridiculous board size - %d assumed", MAXBOARD );
		size = MAXBOARD;
	}
	boardsize = size;

	/* start with a cleared board */
	for( row = 1; row <= MAXBOARD; row++ )
		for( col = 1; col <= MAXBOARD; col++)
			hnd_board[row][col] = 0;

	/* very small boards have no handicap points */
	if (size < 9)
	{	num_hoshi = 0;
		num_handi = 0;
		return;
	}

	/* locate the interesting lines on the board */
	min_ha = 4;
	max_ha = size-3;
	min_san = 3;
	max_san = size-2;
	middle = (size+1)/2;
	min_huh = (min_ha+middle)/2;
	max_huh = (max_ha+middle+1)/2;

	/* wierdest of all are the  mid-huh points */
	hnd_board[ max_huh ][ middle ]	= 33;
	hnd_board[ min_huh ][ middle ]	= 32;
	hnd_board[ middle ][ max_huh ]	= 31;
	hnd_board[ middle ][ min_huh ]	= 30;

	/* next come the san-huh points */
	hnd_board[ max_huh ][ max_san ]	= 29;
	hnd_board[ max_huh ][ min_san ]	= 28;
	hnd_board[ min_huh ][ max_san ]	= 27;
	hnd_board[ min_huh ][ min_san ]	= 26;
	hnd_board[ max_san ][ max_huh ]	= 25;
	hnd_board[ max_san ][ min_huh ]	= 24;
	hnd_board[ min_san ][ max_huh ]	= 23;
	hnd_board[ min_san ][ min_huh ]	= 22;

	/* next come the mid-san points */
	hnd_board[ max_san ][ middle ]	= 21;
	hnd_board[ min_san ][ middle ]	= 20;
	hnd_board[ middle ][ max_san ]	= 19;
	hnd_board[ middle ][ min_san ]	= 18;

	/* next come the four huh-points about the center */
	hnd_board[ max_huh ][ min_huh ]	= 17;
	hnd_board[ min_huh ][ max_huh ]	= 16;
	hnd_board[ max_huh ][ max_huh ]	= 15;
	hnd_board[ min_huh ][ min_huh ]	= 14;

	/* next come the four san-san points */
	hnd_board[ max_san ][ min_san ]	= 13;
	hnd_board[ min_san ][ max_san ]	= 12;
	hnd_board[ max_san ][ max_san ]	= 11;
	hnd_board[ min_san ][ min_san ]	= 10;

	/* finish with the hoshi points */
	hnd_board[ max_ha ][ middle ]	= 9;
	hnd_board[ min_ha ][ middle ]	= 8;
	hnd_board[ middle ][ max_ha ]	= 7;
	hnd_board[ middle ][ min_ha ]	= 6;
	hnd_board[ middle ][ middle ]	= 5;
	hnd_board[ max_ha ][ min_ha ]	= 4;
	hnd_board[ min_ha ][ max_ha ]	= 3;
	hnd_board[ max_ha ][ max_ha ]	= 2;
	hnd_board[ min_ha ][ min_ha ]	= 1;

	/* if he needs more than this, he's playing the wrong game */

	/* note how many points were recorded */
	num_hoshi = (size > 14) ? 9 : 4;
	num_handi = 33;
}
