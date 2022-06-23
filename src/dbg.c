/*
 * module:
 *	dbg.c
 *
 * purpose:
 *	contains routines to control and perform diagnostic logging
 */

#include <stdio.h>
#include "dbg.h"
#include "go.h"
#include "libs.h"
#include "move.h"
#include "strings.h"

#define DBGLOG	"go.dbg"

/*
 * this array associates trace masks with the codes that specify them
 */
struct dbgbit
{	char	d_char;		/* character to sepcify this trace */
	int	d_mask;		/* mask to specify this trace */
} dbgbits[] =
{ 	'm',	D_moves,	
	'e',	D_est,	
	'k',	D_kills,
	'i',	D_invalid,
	'a',	D_affected,
	'f',	D_fantasy,	
	'*',	D_all,
	0,	0
};

/*
 * routine:
 *	dbgstat
 *
 * purpose:
 *	to set and display the debug status flags
 *
 * parms:
 *	command string
 *	    null - display current debug status
 *	    +xxx - turn on the specified modes
 *	    -xxx - turn off the specified modes
 */
void dbgstat(char *cmd )
{	register int mask = 0;
	register int i;
	register char *sp;
	char buf[32];

	if (cmd && *cmd)
	{	/* figure out which debug options are to be changed */
		for( sp = cmd; *sp && *sp != ' ' && *sp != '\t'; sp++ )
			for( i = 0; dbgbits[i].d_mask; i++ )
				if (*sp == dbgbits[i].d_char)
					mask |= dbgbits[i].d_mask;

		/* construct the new debug mask appropriately */
		if (*cmd == '+')
			debug |= mask;
		else
			debug &= ~mask;
	}

	/* if we should be tracing, make sure we have a debug file */
	if (debug && dbglog == NULL)
	{	dbglog = fopen( DBGLOG, "w" );
		if (dbglog == NULL)
		{	d_msg( "Unable to create trace log %s", DBGLOG );
			return;
		}
	}

	/* see if we can and should be performing our tracing */
	if (debug == 0 )
	{	if (!darkness)
			d_msg( "Diagnostic tracing is disabled" );
		return;
	}

	/* find out which bits are on and display them */
	for( sp = buf, i = 0; dbgbits[i].d_mask; i++ )
		if ((debug & dbgbits[i].d_mask) && (dbgbits[i].d_char != '*'))
			*sp++ = dbgbits[i].d_char;
	*sp = 0;

	if (!darkness)
		d_msg( "The following traces are enabled: %s", buf );
}

/* this string is used alot in the following routines */
static char *contin = "enter return to continue the audit";

/*
 * routine:
 *	l_audit
 *
 * purpose:
 *	to audit the liberty lists and print the results
 */
void l_audit()
{	register struct libs *lp;
	register int loopcnt;
	register int freecnt = 0;
	register int foundcnt = 0;
	register struct move *mp;
	extern struct libs *freelibs;

	/* count the free liberties */
	for( lp = freelibs, loopcnt = 0; lp; lp = lp->l_next )
	{	freecnt++;
		if (loopcnt++ > MAXLIBS)
		{	d_msg( "LIBERTY ERROR: LOOP IN FREE LIST" );
			(void) confirm( contin );
			break;
		}
	}

	/* tally up the liberties associated with each move */
	for( mp = moves; mp < &moves[ MAXMOVE ]; mp++ )
	{	loopcnt = 0;
		/* count liberties in this move, checking for loops */
		for( lp = mp->m_liblist; lp; lp = lp->l_next )
		{	foundcnt++;
			if (loopcnt++ > MAXLIBS)
			{	d_msg( "LIBERTY ERROR: loop in move %d", 
					mp-moves );
				(void) confirm( contin );
				break;
			}
		}

		/* make sure the list length agrees with the liberty count */
		if ((loopcnt != mp->m_libs)  &&  
			( (mp > &moves[ pillage ])  ||  (loopcnt != 0)) )
		{	d_msg( "LIBERTY ERROR: move %d, count %d, list %d",
				mp-moves, mp->m_libs, loopcnt );
			(void) confirm( contin );
		}
	}

	/* report on results */
	d_msg( "LIBERTIES: %d free, %d in use, total %d, lost %d, pillage %d",
		freecnt, foundcnt, freecnt+foundcnt, MAXLIBS-(freecnt+foundcnt),
		pillage );
	(void) confirm( contin );
}

/*
 * routine:
 *	m_audit
 *
 * purpose:
 *	to check the consistency of the move structures
 *
 * note:
 *	We only audit the merge counts, as it is metaphysically difficult
 *	to audit kill counts.  If a neighbor pointer refers to a friendly
 *	move, we know that we subsummed that group and took it off the string
 *	board.  If a neighbor pointer refers to a hostile group, we don't
 *	know whether we killed it or merely impinged upon it.  That
 *	determination is only possible when the move in question is at the
 *	top of the move stack.
 */
void m_audit()
{	register struct move *mp;
	register short *np;
	int stones, hurts, m;
	char absbd[MAXMOVE];	/* how many times is this move absorbed */
	char hurt[MAXMOVE];	/* how many times is this move hurt */

	/* look at each move structure and establish the connectivity */
	for( mp = &moves[1]; mp < &moves[ movenum ]; mp++ )
	{	absbd[ mp - moves ] =hurt[ mp - moves ] = hurts = 0;
		stones = 1;

		/* see whether or not this is a real move */
		if (mp->m_pos.row_col == 0)
		{	absbd[ mp - moves ] = 1; /* not really, but sort of */
			stones = 0;
		}

		/* examine the subsumed and impinged neighbors */
		for( np = mp->m_neighbor; np < &mp->m_neighbor[4]; np++ )
		{	/* ignore non-neighbor-neighbors */
			if ((m = *np) == 0)
				continue;
			if (m > 0)	/* subsumed */
			{	stones += moves[ m ].m_stones;
				absbd[m]++;
			} else
			{	hurts++;
				m *= -1;
				hurt[m]++;
			}
			if (m >= (mp - moves))
			{	d_msg( "MOVE ERROR: %d refers to %d",
					mp-moves, m );
				(void) confirm( contin );
			}
		}

		/* compare our counts with recorded counts */
		if (stones != mp->m_stones)
		{	d_msg( "MOVE ERROR: %d - m_stones %d, counted %d",
				mp-moves, mp->m_stones, stones );
			(void) confirm( contin );
		}

		if (hurts != mp->m_hurts)
		{	d_msg( "MOVE ERROR: %d - m_hurts %d, counted %d",
				mp-moves, mp->m_hurts, hurts );
			(void) confirm( contin );
		}
	}

#define ROW(n)	moves[n].m_pos.sub.row
#define COL(n)	moves[n].m_pos.sub.col
	/* confirm that all moves are properly subsumed or on string board */
	for( m = 1; m < movenum; m++ )
	{	/* no move should be multiply absorbed */	
		if (absbd[m] > 1)
		{	d_msg( "MOVE ERROR: %d referenced %dx", m, absbd[m] );
			(void) confirm( contin );
			continue;
		}

		/* absorbed moves shouldn't be on the board */
		if (absbd[m] == 1)
		{	if (str_board[ ROW(m) ][ COL(m) ].s_moveno == m)
			{	d_msg( "MOVE ERROR: %d still on board", m );
				(void) confirm( contin );
			}
			continue;
		}

		/* not absorbed - should be dead or on string board */
		if (str_board[ ROW(m) ][ COL(m) ].s_moveno != m)
		{	/* move is not on string board - dead ? */
			if (hurt[m] > 0 && moves[m].m_libs == 0)
				continue;
			d_msg( "MOVE ERROR: %d not referenced", m );
			(void) confirm( contin );
		} else
		{	/* move is on string board */
			if (moves[m].m_libs > 0)
				continue;
			d_msg( "MOVE ERROR: %d appears dead", m );
			(void) confirm( contin );
		}
	}
#undef	ROW
#undef	COL
}
