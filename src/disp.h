/*
 * module:
 *	disp.h
 *
 * purpose:
 *	this module defines the data structure from which the board
 *	display is generated, and the fields and values stored therein.
 */

/*
 * for each point on the board, we maintain the folowing information
 * (for display purposes):
 *	the color of the stone
 *	the move number that placed the stone there
 *	any special notations on the stone
 *	any special notations on the point
 * this information is used to generate the board display on the screen
 */
unsigned short dsp_board[ MAXBOARD ][ MAXBOARD ];
unsigned short b_set(), b_tag();

/* fields of board structure */
#define B_COLOR		0x8000	/* stone color indication   */

#define B_SHADE		0x7000  /* background/rendition for stone display */
#define B_SPECIAL	0x1000	/* points of special interest */
#define B_PERRIL	0x2000	/* emperriled group */
#define B_VITAL		0x3000	/* group with assured life */
#define B_TERRITORY	0x4000	/* controlled vacancies */
#define B_WALL		0x5000	/* points in a wall or string */
#define	B_ARMY		0x6000	/* points in an army */
#define B_MISC		0x7000	/* misc other purposes */

/* macros to determine color and rendition of a point */
#define B_SHADENUM(shade)	(((shade) >> 12) & 0x7)
#define B_SHADECLR(shade,color)	((((shade)+(color*B_COLOR)) >> 12) & 0xf)

#define B_TYPE		0x0e00	/* stone symbol designation */
#define	B_NORMAL	0x0000	/* normal stone */
#define B_DELTA		0x0200	/* crucial stone */
#define B_QUAD		0x0400	/* other crucial stone */
#define B_CORPSE	0x0600	/* doomed stone */

/* macros to extract information from board */
#define B_WHT_STONE(point)	((point)&B_COLOR)
#define B_MOVENUM(point)	((point)&0x1ff)

/*
 * form of the display is determined by the value of b_display
 */
int b_display;		/* type of display being shown */
int b_basemove;		/* subtracted from move #s (for display purposes) */

#define BY_COLOR 0		/* mundane, no special indications */
#define BY_MOVE	1		/* normal, labeled with move numbers */
#define BY_TYPE 2		/* show stones according to type */

/*
 * for laying out handicaps and computing figuring out whether or not
 * a particular blank spot should be shown as a handicap point, we
 * build an array of handicap points.  This could be done statically,
 * but in this way we can locate the handicap points on a board of any
 * size.
 */
unsigned char hnd_board[MAXBOARD+1][MAXBOARD+1];
int num_hoshi;	/* number of hoshi points on board */
int num_handi;	/* total number of handicap points */
