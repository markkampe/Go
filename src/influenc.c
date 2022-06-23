/*
 * module:
 *	influence.c
 *
 * purpose:
 *	this module contains routines to maintain and idea of
 *	what the overall state of the board is, through the use
 *	of influence functions.  
 */
#include <stdio.h>
#include "go.h"
#include "disp.h"
#include "inf.h"

/*
 * the influence on a point is the sum of the influences of all stones
 * in the vicinity.  The influence of a stone is positive or negative
 * depending on whether it is white or black.  The magnitude of the
 * influence function is:
 */
int inf_func[7][7] =
{	0,	0,	0,	1,	0,	0,	0,
	0,	1,	2,	5,	2,	1,	0,
	0,	2,	6,	13,	6,	2,	0,
	1,	5,	13,	60,	13,	5,	1,
	0,	2,	6,	13,	6,	2,	0,
	0,	1,	2,	5,	2,	1,	0,
	0,	0,	0,	1,	0,	0,	0
};

/*
 * it would be awkward to directly add the influence function to the
 * points around a stone if that stone happened to be within three
 * lines of the edge.  Additionally, the influence function does not
 * properly recognize the increased strength associated with a stone near
 * the edge.  To deal with these problems, we allow the "excess" portion
 * of the influence function to fold back and doubly influence points 
 * near the edge.  This folding is accomplished with the help of the
 * folding array, which specifies which columns of the board should 
 * be incremented - based on the row/col in which the stone is placed.
 *
 * For stones placed between the fourth and sixteenth lines, the folding
 * array just offsets the row/col so that the influence function will
 * be centered on the point where the stone was set.  Near the edges
 * however, it folds the function back over the points near the edge.
 *
 * Note that this function is (and must be) symmetric about the center of 
 * the board, and identical for both rows and columns.
 */
int inf_fold[ MAXBOARD + 7 ] =
{	0,3,2,1,1,2,3,4,5,6,7,8,9,10,11,12,13,15,16,17,18,19,19,18,17 
};

/*
 * routine:
 *	i_reset
 *
 * purpose:
 *	to reset the influence database to zero
 */
void i_reset()
{	register int r, c;

	/* initialize the influence board to zero */
	for( r = 1; r <= boardsize; r++ )
		for( c = 1; c <= boardsize; c++ )
			inf_board[r][c] = 0;
	
	/* initialize the influence folding function for the board size */
	inf_fold[0] = 0; inf_fold[1] = 3; inf_fold[2] = 2; inf_fold[3] = 1;
	for( r = 1; r <= boardsize; r++ )
		inf_fold[3+r] = r;
	inf_fold[3+r] = r-1; inf_fold[4+r] = r-2; inf_fold[5+r] = r-3;
}

/*
 * routine:
 *	delta_inf
 *
 * purpose:
 *	to compute the delta influence associated with a move
 *
 * parms:
 *	row and column of move being made
 *	color of stone being placed or removed
 *	number of stones being placed (1,-1)
 *
 * note
 *	in order to gracefully/efficiently/reasonably handle placement
 *	of stones near the edges, we use the influence folding function
 *	to determine exactly which columns of the influence function
 *	should be added to which columns of the board.
 */
void delta_inf( unsigned row, unsigned col, unsigned color, int new )
{	register int i, j;
	register int r;
	int sign = (color == WHITE) ? new : -new;

	if (sign > 0)
		for( i = 0; i < 7; i++ )
		{	r = inf_fold[ row+i ];
			for( j = 0; j < 7; j++ )
				inf_board[r][inf_fold[col+j]] += inf_func[i][j];
		}
	else /* sign < 0 */
		for( i = 0; i < 7; i++ )
		{	r = inf_fold[ row+i ];
			for( j = 0; j < 7; j++ )
				inf_board[r][inf_fold[col+j]] -= inf_func[i][j];
		}
}

/*
* routine: 
*	c_estimate
*
* purpose:
*	to display teritory estimates based on influence
*
* options:
*	default - shade all armies and walls
*	i	- display influence functions
*	t	- shade all controlled vacancies
*/
void c_estimate( char *arg )
{	int score,net;
	char *winner;

	/* default is to display everything */
	if(arg == 0  ||  *arg==0)
		arg = "a";

	switch( *arg )
	{ case 't':	/* estimate territory */
	  case 'w':	/* estimate walls */
	  case 'a':	/* estimate everything */
		showterr( *arg );
		break;

	  case 's':	/* estimate score */
		score = est_terr();
		if (score < 0)
		{	winner = "black";
			net = -score;
		} else
		{	winner = "white";
			net = score;
		}
		d_msg("Score: %s by %d, b:%d-%d=%d, w:%d-%d=%d, unclaimed=%d",
			winner, net, b_terr, w_kills, b_terr - w_kills,
			w_terr, b_kills, w_terr - b_kills,
			vacancies - b_terr - w_terr );
		return;

	  case 'i':	/* display influence */
		showinf();
		break;

	  default:
		d_msg( "Unrecognized estimate request: %s", arg );
		return;
	}

	(void) confirm( "enter return to resume game" );
	b_redraw( FALSE );
}

/*
 * routine:
 *	showinf
 *
 * purpose:
 *	to display the influence functiion
 */
void showinf()
{	register unsigned r, c;
	unsigned color;
	int inf, ainf;

	for( r = 1; r <= boardsize; r++ )
		for( c = 1; c <= boardsize; c++ )
		{	if (inf = inf_board[r][c])
			{	if (inf < 0)
				{	color = BLACK;
					ainf = -inf;
				} else
				{	color = WHITE;
					ainf = inf;
				}

				if (ainf >= I_TH_WALL)
					d_value( r, c, color, B_WALL, inf );
				else 
					d_value( r, c, color, B_ARMY, inf );
			}
		}
	d_update();
}

/*
 * routine:
 *	showterr
 *
 * purpose:
 *	to display the influence influence estimated territory
 *
 * parms:
 *	what things we should show on display (parm to est command)
 */
void showterr( char what )
{	register unsigned r, c;
	unsigned color;
	int inf, ainf, stone, shade;

	for( r = 1; r <= boardsize; r++ )
		for( c = 1; c <= boardsize; c++ )
		{	if (inf = inf_board[r][c])
			{	if (inf < 0)
				{	color = BLACK;
					ainf = -inf;
				} else
				{	color = WHITE;
					ainf = inf;
				}
				stone = dsp_board[r-1][c-1];

				/*
				 * teritorry displays are wrong, for now
				 * I consider territory to be vancancies
				 * with an influence >= TH_WALL.  When this
				 * is fixed, this routine may be very different
				 */
				if (ainf < I_TH_WALL)
				{	/* army point */
					if (what != 'a')
						continue;
					else
						shade = B_ARMY;
				} else
				{	/* wall point */
					if (what == 't')
					{	if (stone)
							continue;
						shade = B_TERRITORY;
					} else
						shade = B_WALL;
				}

				/* now that we know what it is, display it */
				if (stone)
					d_stone( r, c, color, shade, B_NORMAL );
				else
					d_blank( r, c, color, shade );
			}
		}
	d_update();
}

/*
 * routine:
 *	est_terr
 *
 * purpose:
 *	to estimate territory
 *
 * returns:
 *	settings of b_terr and w_terr;
 */
int est_terr()
{	register int r, c;
	int inf, ainf;

	/* FIX THIS ROUTINE TO DO SOMETHING REASONABLE */
	b_terr = w_terr = 0;
	for( r = 1; r <= boardsize; r++ )
		for( c = 1; c <= boardsize; c++ )
		{	if (inf = inf_board[r][c])
			{	if (dsp_board[r-1][c-1])
					continue;
				ainf = (inf > 0) ? inf : -inf;
				if (ainf > I_TH_WALL)
					if (inf > 0)
						w_terr++;
					else
						b_terr++;
			}
		}

	return( w_terr + w_kills - b_terr - b_kills );
}
