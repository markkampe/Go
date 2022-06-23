/*
 * module:
 *	pcdos.c
 *
 * purpose:
 *	go display driver module - PCDOS version
 *
 * note:
 *	this version does not attempt to do any bit-map stuff, but
 *	only uses the screen in a character addressable fashion
 */
#include <stdio.h>
#include <unistd.h>
#include "go.h"
#include "disp.h"

/* screen layout parameters - based 25 line screen and a max 19 line board */
#define HEADLINE  0				/* line for herald/score info */
#define HISTLINE  1				/* line where history starts */
#define BOARDLINE 2				/* line where board begins */
#define MSGLINE	  (BOARDLINE+MAXBOARD+2)	/* line for messages */
#define PRMLINE	  (MSGLINE+1)			/* line for input prompts */
#define LASTLINE  25				/* last line on screen */

#define POINT_WID 3				/* width of a point on board */
#define FREECOL	  (POINT_WID*(MAXBOARD+2))	/* first column beyond board */
#define HISTCOL   (FREECOL+2)			/* colum for move history */
#define LASTCOL	  80				/* width of a screen */
#define HISTLEN	  (MSGLINE-HISTLINE)		/* lines for move history */
		
/* board to screen mapping macros */
#define boardrow(r) (unsigned)(BOARDLINE+MAXBOARD-(r))
#define boardcol(c) (unsigned)(POINT_WID*(c))
#define UNROW(y)	(BOARDLINE+MAXBOARD-y)
#define UNCOL(x)	((x - POINT_WID) / POINT_WID)

/* imported and exported parameters */
int d_type = -1;			/* import from pcdos.c */
int d_histlen = HISTLEN;		/* export to board.c */
#define EGO_TIME	3		/* durration of ego screen */

/* internal parameters */
static char w_char = 'o';		/* character for normal white stones */
static char b_char = 'x';		/* character for normal black stones */
static int grafset = 1;			/* are we using a graphics char set */

/* internal state information */
static char header[MAXLINE];		/* current game headline */
static unsigned newmode;		/* video mode for game use */
static unsigned oldmode;		/* video mode for game use */
static unsigned newpage;		/* video page we are using */
static unsigned oldpage;		/* video page we were in before game */
static unsigned oldcrsr;		/* cursor size upon entry */
static unsigned cur_row;		/* row cursor is currently in */
static unsigned cur_col;		/* column cursor is currently in */
static char empty[MAXBOARD+1][MAXBOARD+1];	/* how to draw board points */
static int knownsize;			/* size of last known board */

/*
 * these arrays determine what display attributes are used for each of
 * the various display purposes.   The ATTRIBUTE macro uses a specified
 * rendition and stone color to determine what display attributes would
 * be appropriate.
 */
#define ATTRIBUTE(s,c) 		attrs[ B_SHADECLR( s, c ) ].norml
#define B_ATTRIBUTE(s,c) 	attrs[ B_SHADECLR( s, c ) ].blank

struct d_atrs
{	unsigned char norml;	/* attributes for normal points/chars */
	unsigned char blank;	/* attributes for side blanks of stones */
} *attrs;		/* pointer to active atr array */

/*
 * monochrome display attributes
 *	normal board positions in inverse video
 *	interesting teritory in normal video
 *	interesting points bright or blinking
 *	text in normal video
 */
struct d_atrs m_atrs[17] =
{	0x70,0x70,	/* monochrome - normal board positions */
	0xf0,0xf0,	/* monochrome - black stones of special interest */
	0xf0,0xf0,	/* monochrome - emperiled black stones */
	0xf0,0xf0,	/* monochrome - live black stones */
	0x07,0x07,	/* monochrome - black territory */
	0x0f,0x0f,	/* monochrome - black walls */
	0x07,0x07,	/* monochrome - black armies */
	0x70,0x70,	/* monochrome - black misc */
	0x70,0x70,	/* monochrome - normal board positions */
	0xf0,0xf0,	/* monochrome - white stones of special interest */
	0xf0,0xf0,	/* monochrome - emperiled white stones */
	0xf0,0xf0,	/* monochrome - live white stones */
	0x07,0x07,	/* monochrome - white territory */
	0x0f,0x0f,	/* monochrome - white walls */
	0x07,0x07,	/* monochrome - white armies */
	0x07,0x07,	/* monochrome - white misc */
	0x07,0x07	/* monochrome - normal text */
};

/*
 * black and white graphics display attributes
 *	normal board positions in inverse video
 *	interesting points and teritory in normal video
 *	text in normal video
 */
struct d_atrs bw_atrs[17] =
{	0x70,0x70,	/* blk & wht - normal board positions */
	0x70,0x70,	/* blk & wht - black stones of special interest */
	0x70,0x70,	/* blk & wht - emperiled black stones */
	0x70,0x70,	/* blk & wht - live black stones */
	0x07,0x07,	/* blk & wht - black territory */
	0x07,0x07,	/* blk & wht - black walls */
	0x07,0x07,	/* blk & wht - black armies */
	0x70,0x70,	/* blk & wht - black misc */
	0x70,0x70,	/* blk & wht - normal board positions */
	0x70,0x70,	/* blk & wht - white stones of special interest */
	0x70,0x70,	/* blk & wht - emperiled white stones */
	0x70,0x70,	/* blk & wht - live white stones */
	0x07,0x07,	/* blk & wht - white territory */
	0x07,0x07,	/* blk & wht - white walls */
	0x07,0x07,	/* blk & wht - white armies */
	0x07,0x07,	/* blk & wht - white misc */
	0x07,0x07	/* blk & wht - normal text */
};

/*
 * sixteen color display attributes
 *	normal board shown in light grey
 *	white stones shown in white
 *	black stones shown in black
 *	interesting territory and positions shown with background color
 *	text in yellow on black
 */
struct d_atrs p_atrs[17] =
{	0x70,0x70,	/* polychrome - normal board positions */
	0xf0,0xf0,	/* polychrome - black stones of special interest */
	0xcd,0xc0,	/* polychrome - emperiled black stones */
	0x4a,0x40,	/* polychrome - live black stones */
	0x57,0x50,	/* polychrome - black territory */
	0x40,0x40,	/* polychrome - black walls */
	0x50,0x50,	/* polychrome - black armies */
	0x60,0x60,	/* polychrome - black misc */
	0x7f,0x70,	/* polychrome - normal board positions */
	0xff,0xf0,	/* polychrome - white stones of special interest */
	0xad,0xa0,	/* polychrome - emperiled white stones */
	0x2a,0x20,	/* polychrome - live white stones */
	0x37,0x30,	/* polychrome - white territory */
	0x2f,0x20,	/* polychrome - white walls */
	0x3f,0x30,	/* polychrome - white armies */
	0x1f,0x10,	/* polychrome - white misc */
	0x0e,0x00	/* polychrome - normal text */
};

/*
 * sixteen grey-tone monotone display attributes
 *	normal board positions in inverse video, neutral grey
 *	white stones displayed in brighter tones
 *	black stones displayed in darker tones
 *	teritory and intersting points shown with varying contrast
 */
struct d_atrs g_atrs[17] =
{	0x24,0x24,	/* grey-tones - normal board positions */
	0xf4,0xf4,	/* grey-tones - black stones of special interest */
	0xe0,0xe4,	/* grey-tones - emperiled black stones */
	0x30,0x34,	/* grey-tones - live black stones */
	0x34,0x34,	/* grey-tones - black territory */
	0x70,0x74,	/* grey-tones - black walls */
	0x64,0x64,	/* grey-tones - black armies */
	0x78,0x74,	/* grey-tones - black misc */
	0x2b,0x24,	/* grey-tones - normal board positions */
	0x9b,0x94,	/* grey-tones - white stones of special interest */
	0xcf,0xc4,	/* grey-tones - emperiled white stones */
	0x5f,0x54,	/* grey-tones - live white stones */
	0x5b,0x54,	/* grey-tones - white territory */
	0x1f,0x14,	/* grey-tones - white walls */
	0x4b,0x44,	/* grey-tones - white armies */
	0x1a,0x14,	/* grey-tones - white misc */
	0x07,0x07	/* grey-tones - normal text */
};

/* this array determines which attributes should be used for which modes */
struct d_atrs *d_atrs[17] = {
	g_atrs,		/* #0:  40x25/16 g-tone   */
	p_atrs,		/* #1:  40x25/16 colors   */
	g_atrs,		/* #2:  80x25/16 g-tone   */
	p_atrs,		/* #3:  80x25/16 colors   */
	p_atrs,		/* #4:  320x200/4 colors  */
	g_atrs,		/* #5:  320x200/4 g-tone  */
	bw_atrs,	/* #6:  320x200/2 colors  */
	m_atrs,		/* #7:  80x25/monotone    */
	p_atrs,		/* #8:  160x200/16 colors */
	p_atrs,		/* #9:  320x200/16 colors */
	p_atrs,		/* #10: 640x200/4 colors  */
	g_atrs,		/* #11: secret EGA mode   */
	g_atrs,		/* #12: secret EGA mode   */
	p_atrs,		/* #13: 320x200/16 colors */
	p_atrs,		/* #14: 640x200/16 colors */
	g_atrs,		/* #15: 640x350/monotone  */
	p_atrs,		/* #16: 640x350/64 colors */
};

#define A_BOARD	0	/* attribute index for normal board */
#define A_TEXT  16	/* attribute index for normal text */

/*
 * special characters in the PC character set, to be used for drawing
 * the board and stones on the board
 */
#define BLOCK  (char) 219	/* solid block character */
#define HOLLOW (char) 0x01	/* hollow circle character */
#define SOLID  (char) 0x02	/* solid circle character */
#define DOT    (char) 0xf9	/* empty point character */
#define CROSS  '+' 		/* hoshi mark character */
#define C_right (char) 180	/* box drawing - single right side */
#define C_tright (char)191	/* box drawing - single top right corner */
#define C_bleft	(char) 192	/* box drawing - single bottom left corner */
#define C_bot	(char) 193	/* box drawing - single bottom side */
#define C_top	(char) 194	/* box drawing - single top side */
#define C_left	(char) 195	/* box drawing - single left side */
#define C_bar	(char) 196	/* box drawing - single horizontal line */
#define C_cross	(char) 197	/* box drawing - single intersection */
#define C_hosh	(char) 216	/* box drawing - double intersection */
#define C_bright (char)217	/* box drawing - single bottom right corner */
#define C_tleft (char) 218	/* box drawing - single top left corner */

/*
 * returned character codes for cursor control functions
 */
#define C_c_ul	(unsigned) 199	/* cursor home */
#define C_c_up	(unsigned) 200	/* cursor up */
#define C_c_ur	(unsigned) 201	/* pg-up */
#define C_c_lf	(unsigned) 203	/* cursor left */
#define C_c_rt	(unsigned) 205	/* cursor right */
#define C_c_ll	(unsigned) 207	/* end */
#define C_c_dn	(unsigned) 208	/* cursor down */
#define C_c_lr	(unsigned) 209	/* pg dn */
#define C_c_ins	(unsigned) 210	/* ins */
#define C_c_del	(unsigned) 211	/* del */

/*
 * This map combines the initial display mode with the current
 * display mode to guess the best mode in which we can operate.
 *
 * note that we do not attempt to use graphics modes.  The happy
 * faces are truely hideous, but it is not worth the slower updates,
 * generally uglier characters, and loss of colors/grey tones to get
 * into graphics modes and make prettier stones.
 */
static char modemap[17] = 
{/* current mode		mode to use for game	*/
/* #0:  40x25/16 g-tone   */	2,	/* use grey tones */
/* #1:  40x25/16 colors   */	3,	/* use colors	  */
/* #2:  80x25/16 g-tone   */	2,	/* use grey tones */
/* #3:  80x25/16 colors   */	3,	/* use colors	  */
/* #4:  320x200/4 colors  */	3,	/* use colors	  */
/* #5:  320x200/4 g-tone  */	2,	/* use grey tones */
/* #6:  320x200/2 colors  */	3,	/* use colors	  */
/* #7:  80x25/monotone    */	7,	/* how boring!    */
/* #8:  160x200/16 colors */	3,	/* use colors	  */
/* #9:  320x200/16 colors */	3,	/* use colors	  */
/* #10: 640x200/4 colors  */	3,	/* use colors	  */
/* #11: secret EGA mode   */	2,	/* what the hay?  */
/* #12: secret EGA mode   */	2,	/* what the hay?  */
/* #13: 320x200/16 colors */	3,	/* use colors	  */
/* #14: 640x200/16 colors */	3,	/* use colors	  */
/* #15: 640x350/monotone  */	2,	/* use grey tones */
/* #16: 640x350/64 colors */	3	/* use colors	  */
};

/* 
 * these two arrays tell us whether any given mode has color or graphics
 * capabilities.  The first array tells us how many colors a mode has.
 * A negative number of colors indicates a number of available grey tones.
 * The second tells us how many pixels of horizontal resolution it has.
 */
static int colors[17] =
{	-16, 16, -16, 16, 4, -4, 2,	/* CGA modes: 0-3 text, 4-6 graphics */
	0,				/* monochrome adaptor text mode: 7 */
	16, 16, 4,			/* PCjr color graphics: 8-10 */
	0, 0,				/* EGA internal modes: 11-12 */
	16, 16, 0, 64			/* EGA high-res graphics: 13-16 */
};

static int graphics[17] =
{	0, 0, 0, 0, 		/* CGA text modes: 0-3 */
	320, 320, 320,		/* CGA graphics modes: 4-6 */
	0,			/* monochrome adaptor text mode: 7 */
	160, 320, 640,		/* PCjr color graphics modes: 8-10 */
	0, 0,			/* EGA internal modes: 11-12 */
	320, 640, 640, 640	/* EGA high resolution graphics: 13-16 */
};

/* this table describes the names of each type of display */
static char *dsplyname[4] =
{	"type 0",
	"pc/jr",
	"c/ega",
	"mono"
};

/* this table describes the names of each of the display modes */
static char *modename[17] =
{	"40x25, 16 grey-tones",
	"40x25, 16 colors",
	"80x25, 16 grey-tones",
	"80x25, 16 colors",
	"320x200, 4 colors",
	"320x200, 4 grey-tones",
	"320x200, 2 colors",
	"80x25, monochrome",
	"PC/jr 160x200, 16 colors",
	"PC/jr 320x200, 16 colors",
	"PC/jr 640x200, 4 colors",
	"Secret EGA mode #11",
	"Secret EGA mode #12",
	"320x200, 16 colors",
	"640x200, 16 colors",
	"640x350, 16 grey-tones",
	"640x350, 64 colors"
};

/*
 * routine:
 *	pc_d_type
 *
 * purpose:
 *	ascertain (via int 11) what type of display we have
 *
 * returns:
 *	setting of appropriate global variables
 *
 * note:
 *	our techniques for telling what we have and want are poor
 */
void pc_d_type()
{	
#ifdef DOS
	union REGS regs;
	int dsptype;
	char *msg;

	/* figure out what video mode and display page we were using */
	regs.h.ah = 0x0f;	/* get current video mode */
	int86( 0x10, &regs, &regs );
	oldmode = regs.h.al;
	oldpage = regs.h.bh;

	/* figure out what type of cursor we were using */
	regs.h.ah = 0x03;	/* get current cursor info */
	regs.h.bh = oldpage;
	int86( 0x10, &regs, &regs );
	oldcrsr = regs.h.cl;

	/* select a different video page than the one in use */
	newpage = (oldpage >= 2) ? 0 : 3;

	/* default screen modes based on modes user set before the game */
	int86( 0x11, &regs, &regs ); /* get the machine configuration */
	dsptype = regs.x.ax & 0x0030 >> 4;

	newmode = modemap[ oldmode ];

	/* see if the user has told us to use some particular mode */
	if (d_type >= ' ')
		switch( d_type )
		{ case 'c':
			newmode = 3;	/* color text mode */
			break;
		  case 'm':
			newmode = 2;	/* monochrome text mode */
			break;
		  case 'd':
			newmode = 7;	/* monochrome dumb mode */
			break;
		  case 'g':
			grafset = 1;	/* use graphics characters */
			break;
		  case '.':
			grafset = 0;	/* don't use graphics characters */
			break;
		}
	else if (d_type >= 0)
		newmode = d_type;	/* see if he picked specific mode */

	/* select appropriate display attributes */
	attrs = d_atrs[ newmode ];
	
	/* if any display argument was given, tell user what we're doing */
	if (d_type >= 0)
	{	d_msg( "display type: %s, mode at entry: %s", 
			dsplyname[dsptype], modename[ oldmode ] );
		d_prompt( "mode for game: %s, %sspecials",
			modename[ newmode ], grafset ? "w/" : "w/o " );
		sleep( 4 );
	}
#endif
}

/*
 * routine:
 *	pc_d_mode
 *
 * purpose:
 *	to set the display mode
 *
 * parmameters:
 *	new mode
 *	display page to use
 *
 * note:
 *	we will only change the mode if it REALLY needs doing, since
 *	changing the mode purges display memory.
 */
void pc_d_mode( unsigned mode, unsigned page )
{	
#ifdef DOS
	union REGS regs;
	static int curmode = -1;

	if (curmode == -1)
		curmode = oldmode;

	if (mode != curmode)
	{	regs.h.ah = 0x00;	/* set current video mode */
		regs.h.al = mode;
		int86( 0x10, &regs, &regs );
		curmode = mode;
	}

	regs.h.ah = 0x05;	/* set active display page */
	regs.h.al = page;
	int86( 0x10, &regs, &regs );
#endif
}

/*
 * routine:
 *	pc_cursize
 *
 * purpose:
 *	to change the cursor type
 *
 * parameters:
 *	number of scan lines high to make it
 */
void pc_cursize( unsigned size )
{	
#ifdef DOS
	union REGS regs;

	/* select a large cursor */
	regs.h.ah = 0x01;	/* set cursor type */
	regs.h.ch = 0;
	regs.h.cl = size;
	int86( 0x10, &regs, &regs );
#endif
}

/*
 * routine:
 *	pc_d_char
 *
 * purpose:
 *	to display a character on the screen
 *
 * parmameters:
 *	row, col
 *	character code
 *	attribute byte
 *	replication count
 */
void pc_d_char( unsigned row, unsigned col, int code, unsigned dispattr, int count )
{	
#ifdef DOS
	union REGS regs;

	regs.h.ah = 0x02;	/* set cursor position */
	regs.h.dh = row;
	regs.h.dl = col;
	regs.h.bh = newpage;
	int86( 0x10, &regs, &regs );

	regs.h.ah = 0x09;	/* display character and attributes */
	regs.h.al = code;
	regs.h.bl = dispattr;
	regs.h.bh = newpage;
	regs.x.cx = count;
	int86( 0x10, &regs, &regs );
#endif
}

/*
 * routine:
 *	pc_d_crsr
 *
 * purpose:
 *	to position the cursor at a specified place on the screen
 *
 * parameters:
 *	row, col
 */
void pc_d_crsr( unsigned row, unsigned col )
{	
#ifdef DOS
	union REGS regs;

	regs.h.ah = 0x02;	/* set cursor position */
	regs.h.dh = row;
	regs.h.dl = col;
	regs.h.bh = newpage;
	int86( 0x10, &regs, &regs );
#endif
}

/*
 * routine:
 * 	pc_d_scroll
 *
 * purpose:
 *	to scroll a window up
 *
 * parms:
 *	upper left row, col
 *	lower right row, col
 *	lines to scroll
 */
void pc_d_scroll( unsigned toprow, unsigned leftcol, unsigned botrow, unsigned rightcol, int lines )
{	unsigned dispatr = attrs[A_TEXT].norml;
#ifdef DOS
	union REGS regs;

	regs.h.ah = 0x06;	/* set cursor position */
	regs.h.al = lines;
	regs.h.ch = toprow;
	regs.h.cl = leftcol;
	regs.h.dh = botrow;
	regs.h.dl = rightcol;
	regs.h.bh = dispatr;
	int86( 0x10, &regs, &regs );
#endif
}

/*
 * routine:
 *	pc_d_line
 *
 * purpose:
 *	to write an entire line of the screen
 *
 * parameters:
 *	row, starting col
 *	text to be written
 */
void pc_d_line( unsigned row, unsigned col, char *str )
{	unsigned dispatr = attrs[A_TEXT].norml;
#ifdef DOS
	union REGS regs;

	/* start by blanking the line to be written */
	pc_d_char( row, col, ' ', dispatr, LASTCOL - col );

	/* display each character in the line */
	while( *str )
	{	/* check for non-graphic motion characters */
		if (*str == '\t')
		{	str++;
			while( col & 7 )
				col++;
			pc_d_crsr( row, col );
			continue;
		} else if (*str == '\n')
		{	/* newlines take us to the next line */
			str++;
			row++;
			col = 0;
			pc_d_crsr( row, col );
			continue;
		} else if (*str < ' ')
		{	/* skip over other non-graphics */
			str++;
			continue;
		}

		/* check for line wrap */
		if (++col == 80)
		{	row++;
			col = 0;
		}

		/* now write the normal character */
		regs.h.ah = 0x0e;	/* display character and move cursor */
		regs.h.al = *str++;
		int86( 0x10, &regs, &regs );
	}

	/* note where we left the cursor */
	cur_row = row;
	cur_col = col;
#endif
}

/*
 * routine:
 *	pc_getch - in case the library should be unavailable some day
 *
 * purpose:
 *	to read a byte from the keyboard
 *
 * returns:
 *	code for byte read
 *	wierd characters returned as scan code + 128
 */
unsigned pc_getch()
{	
#ifdef DOS
	union REGS regs;

	regs.h.ah = 0x00;	
	int86( 0x16, &regs, &regs );

	if (regs.h.al == 0)
		return( regs.h.ah + 128 );
	else
		return( regs.h.al );
#endif
}

/*
 * routine:
 *	d_init
 *
 * purpose:
 *	initialize the screen for use
 *
 * returns:
 *	void
 */
void d_init( )
{
	/* determine what type of interface we have and initialize it */
	pc_d_type();

	/* put the display into the appropriate modes for the game */
	pc_d_mode( newmode, newpage );
	pc_cursize( 8 );

	/* build the default header line */
	d_header( "" );
}

/*
 * routine:
 *	d_cleanup
 *
 * purpose:
 *	do any cleanup of display, necessary before we exit
 *
 * parameters:
 *	void
 *
 * returns:
 *	void
 */
void d_cleanup()
{	
	/* go back to normal display mode */
	pc_cursize( oldcrsr );
	pc_d_mode( oldmode, oldpage );
}

/*
 * routine:
 *	d_header
 *
 * purpose:
 *	to set the game headline message
 *
 * parameters:
 *	VARARGS1
 *	same as printf
 *
 * returns:
 *	void
 */
void d_header( char *s, void *a1, void *a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8 )
{	char tbuf[MAXLINE];

	/* merge provided information with other arguments to form header */
	(void) sprintf( tbuf, s, a1, a2, a3, a4, a5, a6, a7, a8 );
	(void) sprintf( header, "%s  \tGame: %s, %s   \t%s", 
		version, p_name, gamename, tbuf );
	
	if (!darkness)
		pc_d_line( HEADLINE, 0, header );
};

/*
 * routine:
 *	d_msg
 *
 * purpose:
 *	display a message to the user
 *
 * parameters:
 *	VARARGS1
 *	same as printf
 *
 * returns:
 *	void
 */
void d_msg( char *s, char *a1, char *a2, char *a3, char *a4, char *a5, char *a6, char *a7, char *a8, char *a9 )
{	char tbuf[ MAXLINE ];

	/* run the format and args through sprintf */
	(void) sprintf( tbuf, s, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	
	pc_d_line( MSGLINE, 0, tbuf );
};

/*
 * routine:
 *	d_prompt
 *
 * purpose:
 *	prompt the user for input
 *
 * parameters:
 *	VARARGS1
 *	input prompt - same parameters as printf
 *
 * returns:
 *	void
 */
void d_prompt( char *s, char *a1, char *a2, char *a3 )
{	char tbuf[ MAXLINE ];

	/* run the format and args through sprintf */
	(void) sprintf( tbuf, s, a1, a2, a3 );
	
	pc_d_line( PRMLINE, 0, tbuf );
};
	
/*
 * routine:
 *	d_readline
 *
 * purpose:
 *	to read an input line from the user terminal
 *
 * parameters:
 *	address of buffer
 *
 * returns
 *	count/0 
 */
int d_readline( char *buf )
{	unsigned int col, c, offset;
	unsigned pc_getch();
	unsigned atrs = attrs[ A_TEXT ].norml;
	int col_incr = 1;
	int lastcol = 0;
	register char *p;
	static int insmd = 0;

	offset = cur_col;
	col = 0;

	/* 
	 * read and echo characters until we have a full line
	 *	note that we only move the cursor with the input
	 *	as long as it is on the prompt line.  Once it
	 *	leaves the prompt line, the cursor only responds
	 *	to cursor motion keys.  This is because we want
	 *	the cursor to be able to select board positions to
	 *	be affected by a command.  If this has been done,
	 *	the board position will be appended to the end of
	 *	the command before it is returned.
	 */
	for(;;)
	{	/* read the next character from the keyboard */
		c = pc_getch();

		/* tabs are treated as blanks */
		if (c == '\t')
			c = ' ';

		/* normal characters are collected and echoed */
		if (c >= ' ' && c < 0177)
		{	if (insmd  &&  col < lastcol)
			{	/* shift the text right at col */
				buf[lastcol] = 0;
				for( p = &buf[lastcol]; p >= &buf[col]; p-- )
					p[1] = *p;
				lastcol++;
				pc_d_line( PRMLINE, offset+col, &buf[col] );
			}

			pc_d_char( PRMLINE, offset+col, (char) c, atrs, 1 );
			buf[ col++ ] = c;
			if (col > lastcol)
				lastcol = col;
			if (cur_row == PRMLINE)
				cur_col = offset + col;
			pc_d_crsr( cur_row, cur_col );
		} else switch( c )
		{ case '\n':
		  case '\r':
		  case 004:	/* line terminators */
			/* clear obsolete messages */
			pc_d_char( MSGLINE, 0, ' ', atrs, LASTCOL );

			/*
			 * if we have an implicit board position specified
			 * by the cursor, append it to the buffered command
			 * and then echo the command line in final form
			 */
			if (cur_row != PRMLINE)
			{	buf[lastcol++] = ' ';
				buf[lastcol++] = '0' + UNROW( cur_row )/10;
				buf[lastcol++] = '0' + UNROW( cur_row )%10;
				buf[lastcol++] = 'a' + UNCOL( cur_col );
				buf[lastcol] = 0;
				pc_d_line( PRMLINE, offset, buf );
			} else
				buf[lastcol] = 0;

			return( lastcol );

		 case 027:	/* ^W - word delete */
			do { col--; cur_col--; }
				while( col > 0 && buf[col] == ' ' );
			do { col--; cur_col--; }
				while( col > 0 && buf[col] != ' ' );
			if (col > 0)
				goto backup;

		 case 025:	/* ^U - line delete */
		 case 030:	/* ^X - line delete */
			cur_row = PRMLINE;
			col = 0;
			goto backup;

		 case '\b': /* backspace */
			if (col > 0)
				col--;

		backup:	pc_d_char( PRMLINE, offset+col, ' ', atrs, 
					(int) (LASTCOL - offset - col) );
			lastcol = col;
			if (cur_row == PRMLINE)
			{	cur_col = offset + col;
				pc_d_crsr( PRMLINE, cur_col );
			}
			continue;

		 case C_c_del:	/* delch a character */
			buf[lastcol] = 0;
			for( p = &buf[col]; p < &buf[lastcol]; p++ )
				*p = p[1];
			lastcol--;
			pc_d_line( PRMLINE, offset+col, &buf[col] );
			pc_d_char( PRMLINE, offset+lastcol, ' ', atrs, 1 );
			cur_col = col+offset;
			pc_d_crsr( cur_row, cur_col );
			continue;
			
		 case C_c_ins:	/* toggle insert mode */
			insmd = !insmd;
			continue;

		 case '\f': /* form feed */
			/* refresh not implemented */
			continue;

		/*
		 * cursor positioning commands to take us to a
		 * fixed particular place on the screen.  These
		 * are all around the cursor pad.
		 */
		case C_c_ul:	/* home to upper-left of board */
			cur_row = boardrow( boardsize-3 );
			cur_col = boardcol( 4 )+1;
			goto movecur;

		case C_c_ur:	/* home to upper-right of board */
			cur_row = boardrow( boardsize-3 );
			cur_col = boardcol( boardsize-3 )+1;
			goto movecur;

		case C_c_ll:	/* home to lower-left of board */
			cur_row = boardrow( 4 );
			cur_col = boardcol( 4 )+1;
			goto movecur;

		case C_c_lr:	/* home to lower-right of board */
			cur_row = boardrow( 4 );
			cur_col = boardcol( boardsize-3 )+1;
			goto movecur;

		/*
		 * relative cursor motion commands
		 */
		case C_c_up:	/* cursor up one row */
			if (--cur_row < boardrow( boardsize ))
				cur_row = boardrow( 1 );
			goto movecur;

		case C_c_dn:	/* cursor down one row */
			if (++cur_row > boardrow( 1 ))
				cur_row = boardrow( boardsize );
			goto movecur;

		case C_c_rt:	/* cursor right one column */
			cur_col += col_incr;
			if (cur_row == PRMLINE)
			{	if (cur_col >= lastcol)
					cur_col = offset;
			} else
			{	if (cur_col > (boardcol( boardsize )+1))
					cur_col = boardcol(1) + 1;
			}
			goto movecur;

		case C_c_lf:	/* curwor left one column */
			cur_col -= col_incr;
			if (cur_row == PRMLINE)
			{	if (cur_col < offset)
					cur_col = lastcol+offset;
			} else
			{	if (cur_col <= boardcol( 1 ))
					cur_col = boardcol( boardsize ) + 1;
			}
			goto movecur;

		 default: /* all other control functions toggle board/line */ 
			if (cur_row == PRMLINE)
			{	cur_row = boardrow( (boardsize+1)/2 );
				cur_col = boardcol( (boardsize+1)/2 ) + 1;
			} else
			{	cur_row = PRMLINE;
				cur_col = offset + lastcol;
			}

		 movecur: /* reposition the cursor to cur_row, col */
			pc_d_crsr( cur_row, cur_col );

			/* determine appropriate column width for this row */
			col_incr = (cur_row == PRMLINE) ? 1 : POINT_WID;

			/* if on input line, set input colum by cursor */
			if (cur_row == PRMLINE  &&  cur_col >= offset)
				col = cur_col - offset;
			continue;
		}
	}
}

/*
 * routine:
 *	d_text
 *
 * purpose:
 *	to display a line of normal (help-type) text
 *
 * parms:
 *	pointer to text to be displayed
 *	line number it should be printed on
 */
void d_text( char *str, unsigned line )
{	
#ifdef DOS
	/* if we are displaying line 0, clear the screen */
	if (line == 0)
		pc_d_char( 0, 0, ' ', attrs[A_TEXT], LASTLINE * LASTCOL );

	pc_d_line( line, 0, str );
#endif
}

/*
 * routine:
 *	d_clear
 *
 * purpose:
 *	clear the board
 *
 * parameters:
 *	void
 *
 * returns:
 *	void
 */
void d_clear()
{	register int i, j;
	unsigned atrs = attrs[ A_TEXT ].norml;

	/* if we haven't yet figured out how to draw this board, do so */
	if (knownsize != boardsize)
	{	/* 
		 * rather than figure out which characters to put where
		 * on the board each time we display something, we build
	 	 * up a map each time the board changes size
		 *
		 *	1. label all of the points in the middle of the board
		 *	2. label all of the points on the edges of the board
		 *	3. label the corners
		 *	4. figure out what to use for stones
		 */
		for( i = 2; i < boardsize; i++ )
		    for( j = 2; j < boardsize; j++ )
			if (hnd_board[i][j]>0 && hnd_board[i][j]<=num_hoshi)
				empty[i][j] = grafset ? C_hosh : CROSS;
			else
				empty[i][j] = grafset ? C_cross : DOT;

		for( i = 2; i < boardsize; i++ )
		{	empty[1][i] = grafset ? C_bot : DOT;
			empty[i][1] = grafset ? C_left : DOT;
			empty[boardsize][i]= grafset ? C_top : DOT;
			empty[i][boardsize] = grafset ? C_right : DOT;
		}

		empty[1][1] = grafset ? C_bleft : DOT;
		empty[1][boardsize] = grafset ? C_bright : DOT;
		empty[boardsize][1] = grafset ? C_tleft : DOT;
		empty[boardsize][boardsize] = grafset ? C_tright : DOT;

		w_char = grafset ? (colors[newmode] ? SOLID : HOLLOW) : 'o';
		b_char = grafset ? SOLID : 'x';

		knownsize = boardsize;
	}

	/* start with a blank screen */
#ifdef DOS
	pc_d_char( 0, 0, ' ', attrs[A_TEXT], LASTLINE * LASTCOL );
#endif

	/* replace the herald notices */
	pc_d_line( HEADLINE, 0, header );

	/* label the horizontal axes */
	for( i = 1; i <= boardsize; i++ )
	{	pc_d_char( boardrow(0), boardcol(i)+1, 'a'+i-1, atrs, 1 );
		pc_d_char( boardrow(boardsize+1),boardcol(i)+1,'a'+i-1,atrs,1 );
	}

	/* label the vertical axes */
	for( i = 1; i <= boardsize; i++ )
	{	if (i >= 10)
		{	pc_d_char( boardrow(i),boardcol(0),'0'+(i/10),atrs,1 );
			pc_d_char( boardrow(i), boardcol(boardsize+1)+1, 
				'0'+(i/10), atrs, 1 );
			pc_d_char( boardrow(i), boardcol(boardsize+1)+2, 
				'0'+(i%10), atrs, 1 );
		} else
		{	pc_d_char( boardrow(i), boardcol(boardsize+1)+1,
				'0'+i, atrs, 1 );
		}
		pc_d_char( boardrow(i), boardcol(0)+1, '0'+(i%10), atrs, 1 );
	}

	/* the board itself is put up by d_blank and d_stone */
}

/*
 * routine:
 *	d_blank
 *
 * purpose:
 *	to clear a position on the board
 *
 * parameters:
 *	row, col
 *	controlling color, and rendition
 *
 * returns:
 *	void
 */
void d_blank( unsigned row, unsigned col, unsigned color, int shade )
{	unsigned char atr, lsp, rsp;

	/*
	 * each point takes up three spaces on the screen, and what
	 * we put in the spaces depends on where the point is
	 */
	lsp = (grafset && col > 1) ? C_bar : ' ';
	rsp = (grafset && col < boardsize) ? C_bar : ' ';
	atr = B_ATTRIBUTE( shade, color );
	pc_d_char( boardrow( row ), boardcol( col )+0, lsp, atr, 1 );
	pc_d_char( boardrow( row ), boardcol( col )+1, empty[row][col], atr, 1);
	pc_d_char( boardrow( row ), boardcol( col )+2, rsp, atr, 1 );
}

/*
 * routine:
 *	d_stone
 *
 * purpose:
 *	to display a stone on the board
 *
 * parameters:
 *	row, col
 *	color of stone (B_BLACK, B_WHITE)
 *	shade of background / special rendition
 *	number / symbol to place in stone
 *
 * returns:
 *	void
 *
 * note:
 *	the curses version does not display numbered stones, or
 *	distinguish among various shades.  Stones are black or white,
 *	shaded or unshaded, normal or special symbol.
 */
void d_stone( unsigned row, unsigned col, unsigned color, int shade, int num )
{	
	char lsp, rsp;
	unsigned char atrs = ATTRIBUTE( shade, color );

	/*
	 * each point takes up three spaces on the screen, and what
	 * we put in the spaces depends on where the point is
	 */
	lsp = (grafset && col > 1) ? C_bar : ' ';
	rsp = (grafset && col < boardsize) ? C_bar : ' ';
	pc_d_char( boardrow( row ), boardcol( col )+0, lsp, 
		B_ATTRIBUTE( shade, color ), 1 );
	pc_d_char( boardrow( row ), boardcol( col )+2, rsp, 
		B_ATTRIBUTE( shade, color ), 1 );
	
	/* now figure out what to display for the stone */
	if (num >= 100  ||  num < 0)
	{	pc_d_char( boardrow(row), boardcol(col)+1, 
			color ? 'O' : 'X', atrs, 1 );
	} else if (num == 0)
	{	pc_d_char( boardrow(row), boardcol(col)+1, 
			color ? w_char : b_char, atrs, 1 );
	} else if (num < 10)
	{	pc_d_char( boardrow(row), boardcol(col)+1, '0'+num, atrs, 1);
	} else
	{	/* 10 <= num < 100 */
		pc_d_char( boardrow(row),boardcol(col), '0'+(num/10), atrs, 1 );
		pc_d_char( boardrow(row),boardcol(col)+1,'0'+(num%10),atrs, 1 );
	}
}

/*
 * routine:
 *	d_value
 *
 * purpose:
 *	to display a value at a position on the board
 *
 * parameters:
 *	row, col
 *	shade of background 
 *	value to be displayed
 *
 * returns:
 *	void
 */
void d_value( unsigned row, unsigned col, unsigned color, int shade, int num )
{	unsigned char lsp, rsp;
	unsigned char atrs = ATTRIBUTE( shade, color );

	/* start by writing out some appropriate white space */
	lsp = (grafset && col > 1) ? C_bar : ' ';
	rsp = (grafset && col < boardsize) ? C_bar : ' ';
	pc_d_char( boardrow( row ), boardcol( col )+0, lsp, 
		B_ATTRIBUTE( shade, color ), 1 );
	pc_d_char( boardrow( row ), boardcol( col )+2, rsp, 
		B_ATTRIBUTE( shade, color ), 1 );

	/* then figure out what to write for the value */
	if (num < 0)
	{	pc_d_char( boardrow(row), boardcol(col), '-', atrs, 1 );
		if (num < -99)
		{	pc_d_char( boardrow(row),boardcol(col)+1,'*', atrs, 2 );
		} else
		{	num = -num;
			pc_d_char( boardrow(row), boardcol(col)+1, 
				'0'+(num/10), atrs, 1 );
			pc_d_char( boardrow(row), boardcol(col)+2, 
				'0'+(num%10), atrs, 1 );
		}
	} else
	{	if (num > 100)
		{	pc_d_char( boardrow(row), boardcol(col), '*', atrs, 2 );
		} else
		{	if (num >= 10)
			{	pc_d_char( boardrow(row), boardcol(col),
					'0'+(num/10), atrs, 1 );
				pc_d_char( boardrow(row), boardcol(col)+1, 
					'0'+(num%10), atrs, 1 );
			} else
				pc_d_char( boardrow(row), boardcol(col)+1, 
					'0'+num, atrs, 1 );
		}
	}
}

/*
 * routine:
 *	d_update
 *
 * purpose:
 *	cause all changes made to display (thus far) to take effect
 *
 * parameters:
 *	void
 *
 * returns:
 *	void
 */
void d_update()
{
	/* this routine is unnecessary, because we do immediate update */
}

/*
 * routine:
 *	d_showmove
 *
 * purpose:
 *	to display a move history
 *
 * parms:
 *	move number 
 *	string
 */
void d_showmove( int movnum, char *str )
{	unsigned line;

	if (movnum > HISTLEN)
	{	pc_d_scroll(HISTLINE, HISTCOL, HISTLINE+HISTLEN-1, LASTCOL-1,1);
		line = HISTLINE+HISTLEN-1;
	} else
		line = HISTLINE+movnum-1;

	/* display the move on the appropriate line */
	pc_d_line( line, HISTCOL, str );
}


