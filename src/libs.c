/*
 * module
 *	libs.c
 *
 * purpose
 *	routines for the management and manipulation of liberty lists
 */
#include <stdio.h>
#include "go.h"
#include "move.h"
#include "libs.h"

struct libs *freelibs;	/* head of the chain ef liberty elements */
int l_quota = MAXLIBS/4;	/* quota for liberty pillaging */

/*
 * routine:
 *	l_reset
 *
 * purpose:
 *	to (re-)initialize the free liberty list
 */
void l_reset()
{	register struct libs *lp;

	/* reinitialize free liberty list to contain all lib nodes */
	freelibs = 0;
	for( lp = libs; lp < &libs[ MAXLIBS ]; lp++ )
	{	lp->l_next = freelibs;
		freelibs = lp;
	}

	/* we have not canibalized any liberties */
	pillage = 0;
}

/*
 * routine:
 *	l_ransack
 *
 * purpose:
 *	to loot and pillage very old moves in order to reclaim liberties
 *
 * parms:
 *	number of liberties to be reclaimed
 */
void l_ransack( int quota )
{	register struct libs *lp, *np;
	register struct move *mp;
	register int needed = quota;

	while( needed > 0 )
	{	mp = &moves[ ++pillage ];
		for( lp = mp->m_liblist; lp; lp = np )
		{	np = lp->l_next;
			lp->l_next = freelibs;
			freelibs = lp;
			needed--;
		}
		mp->m_liblist = 0;
	}

	d_msg( "*** Pilaging liberties up to move %d, plundered %d",
		pillage, quota - needed );
}

/*
 * routine:
 *	l_gain
 *
 * purpose:
 *	to add a liberty to a string
 *
 * parms:
 *	address of move node for beneficiary
 *	position to be added as a liberty
 *
 * returns:
 *	number of liberties gained (1/0)
 *
 * notes:
 *	to optimize searches, merges, and deletions, liberties lists
 *	are sorted in order of ascending positions.
 */
int l_gain( struct move *mp, pos_t pos )
{	register struct libs *lp, **pp;

	pp = &mp->m_liblist;
	for( lp = *pp; lp; lp = *pp )
	{	/* see if we found the liberty we're seeking */
		if (lp->l_pos.row_col == pos.row_col)
			return( 0 );

		/* see if we've reached where it should be */
		if (lp->l_pos.row_col > pos.row_col)
			break;

		/* continue on down the list */
		pp = &lp->l_next;
	}

	/* get a free liberty node */
	if (freelibs == 0)
		l_ransack( l_quota );
	*pp = freelibs;
	freelibs = freelibs->l_next;

	/* label it with the position and chain it in before lp */
	(*pp)->l_pos = pos;
	(*pp)->l_next = lp;

	/* and record the benefit */
	mp->m_libs++;
	return( 1 );
}

/*
 * routine:
 *	l_lose
 *
 * purpose:
 *	to remove a liberty from a string
 *
 * parms:
 *	pointer to move node being deprived of a liberty
 *	position to be removed as a liberty
 *
 * returns:
 *	number of liberties lost (1/0)
 */
int l_lose( struct move *mp, pos_t pos )
{	register struct libs *lp, **pp;

	pp = &mp->m_liblist;
	for( lp = *pp; lp; lp = *pp )
	{	/* see if we found the liberty we're seeking */
		if (lp->l_pos.row_col == pos.row_col)
		{	*pp = lp->l_next;
			lp->l_next = freelibs;
			freelibs = lp;
			mp->m_libs--;
			return( 1 );
		}

		/* see if we've passed where it should be */
		if (lp->l_pos.row_col > pos.row_col)
			return( 0 );

		/* continue on down the list */
		pp = &lp->l_next;
	}

	/* we didn't find it to delete */
	return( 0 );
}

/*
 * routine:
 *	l_merge
 *
 * purpose:
 *	to union/merge the libertys of two strings
 *
 * parms:
 *	address of liberty list head in the new string
 *	address of liberty list head in the old string
 *
 * returns:
 *	number of liberties in the merged string
 *
 * notes:
 *	for efficiency reason, liberty lists are sorted in order
 *	of increasing positions.  Thus, this is a merge of two
 *	sorted lists.
 */
int l_merge( struct move *np, struct move *op )
{	register struct libs *lp;	/* place to insert before */
	register struct libs **pp;	/* pointer to insert after */
	register struct libs *xp;	/* pointer to liberty being merged */

	/* start with insertion pointers at the front of the newstring list */
	pp = &np->m_liblist; 
	lp = *pp;

	/* we must merge in each element of the old strings liberty list */
	for( xp = op->m_liblist; xp; xp = xp->l_next )
	{	/* skip to the proper insertion point */
		while( lp != 0  &&  lp->l_pos.row_col < xp->l_pos.row_col)
		{	pp = &lp->l_next;
			lp = *pp;
		}

		/* see if this liberty is already present */
		if (lp->l_pos.row_col == xp->l_pos.row_col)
			continue;

		/* we found insertion point, so get a free liberty node */
		if (freelibs == 0)
			l_ransack( l_quota );
		*pp = freelibs;
		freelibs = freelibs->l_next;

		/* label it with the position and chain it in before lp */
		(*pp)->l_pos.row_col = xp->l_pos.row_col;
		(*pp)->l_next = lp;
		pp = &((*pp)->l_next);
		np->m_libs++;
	}

	return( np->m_libs );
}

/*
 * routine:
 *	l_free
 *
 * purpose:
 *	to free all the liberty nodes associated with a string
 *
 * parms:
 *	pointer to the move node being deallocated
 *
 * returns:
 *	void for now
 */
void l_free( struct move *mp )
{	register struct libs *lp, *np;

	for( lp = mp->m_liblist; lp; lp = np )
	{	np = lp->l_next;
		lp->l_next = freelibs;
		freelibs = lp;
	}

	mp->m_libs = 0;
	mp->m_liblist = 0;
}

