/*
 * module:
 *	libs.h
 *
 * purpose:
 *	to describe the data structures which are used to keep track of
 *	the liberties associated with strings
 */

/*
 * Each string has its liberties enumerated in an ordered linked list.
 * All addition, removal, and merging of liberties is done as operations
 * on these lists.
 */
struct libs
{	struct libs *l_next;	/* address of next liberty in chain */
	pos_t l_pos;		/* position of this liberty */
} libs[MAXLIBS];
