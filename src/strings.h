/*
 * module:
 *	strings.h
 *
 * purpose:
 *	to describe the data structures which are used to keep track of
 *	the strings currently on the board
 */

/*
 * The string board associates positions on the board with existing
 * strings.  For each position on the board, it indicates the move
 * number associated with the top node for the associated string.
 * For convenience, it also contains a little bit more information
 * which could be computed, but is commonly needed.
 *
 * note that a move number of 0 indicates a vacant point, and a move
 *	number of -1 indicates a border (unavailable point).
 */
struct string
{	short	s_moveno;	/* move number of string description node */
	char	s_color;	/* color of group that owns this string */
	char	s_flags;	/* I had to pad it out with something */
} str_board[ MAXBOARD + 2 ][ MAXBOARD + 2 ];
