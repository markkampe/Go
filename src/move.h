/*
 * module:
 *	move.h
 *
 * purpose:
 *	to describe the data structures which are used to keep track of
 *	the move history, and tactical state of the board.
 */

/*
 * this data structure is used to record the move history in a way that
 * permits cheap un-moves.  When old stones are merged into new groups,
 * the old group definitions are left unchanged, but hung under the
 * move structure for the stone that joined them.  Similarly, when
 * stones are killed, the group definitions are left intact, but hung
 * under the group definition for the stone that killed them.  Undoing
 * a move can be acomplished by restoring the trees that have been
 * hung under that move.  These trees are maintained by tactics.c
 *
 * note that these structures are used in a three step process
 *	someone wants to make a move, and fills in color and position
 *	t_estimate figures out what merges and kills will result
 *	t_do carries out the entailed merges and kills
 *
 *	The affected neighbor array has only as many non-zero entries
 *	as the stone has neighboring stones - to slightly speed up
 *	searches.  If an entry is postive, it is the move number of
 *	a friendly neighbor.  If it is negative, it is the move number
 *	of a hostile neighbor.  One can`t tell from the neighbor array
 *	whether the hostile neighbor was killed or merely impinged upon.
 */
struct move
{	struct libs *m_liblist;	/* pointer to list of liberties	*/
	short	m_neighbor[4];	/* links to affected neighbors	*/
	short   m_netlib;	/* net liberties gained by move */
	pos_t	m_pos;		/* position of this move	*/
	pos_t	m_ko;		/* position of created KO	*/
	unsigned char m_flags;	/* flags to describe this move	*/
	unsigned char m_stones;	/* # of stones in new group	*/
	unsigned char m_libs;	/* # of liberties to this group	*/
	unsigned char m_eyes;	/* # of eyes to this group	*/
	unsigned char m_kills;	/* # of hostile stones killed	*/
	unsigned char m_ataris;	/* # of hostile stones atari'd	*/
	unsigned char m_gkills;	/* # of hostile groups killed	*/
	unsigned char m_hurts;	/* # of hostile liberties lost	*/
} moves[ MAXMOVE ];

/* bits in the m_flags field */
#define M_COLOR		0x01	/* is this move black or white	*/
#define M_HANDICAP	0x02	/* is this move a handicap	*/
#define M_MULTIPLE	0x04	/* does it make multiple atari	*/
#define M_BLUNDER	0x08	/* was this move a foolish one	*/

/* move related functions */
void m_reset();
int m_move( struct move *mp );
void m_estimate( struct move *mp );
void m_do( struct move *mp );
void m_unmove( struct move *mp );
void m_restore( struct move *mp, int value, int killer );
void l_free( struct move *mp );
int l_merge( struct move *np, struct move *op );
int l_lose( struct move *mp, pos_t pos );
int l_gain( struct move *mp, pos_t pos );
