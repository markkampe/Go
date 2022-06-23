/*
 * module:
 *	go.h
 *
 * purpose:
 *	contains major constants, parameters and general state variables
 */

/*
 * fundamental constants - if these are changed, things will break
 */
#define WHITE	1	/* standard definition for white things */
#define BLACK	0	/* standard definition for black things */
#define MAXSTR	256	/* maximum number of strings on board	*/

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif 

/*
 * tunable constants and parameters
 */
#define MAXBOARD 19	/* largest allowable go board */
#define MAXHAND 40	/* largest conceivable handicap */
#define MAXLINE 100	/* size of console input buffers */
#define MAXLIBS 1500	/* size of the pool of liberty descriptors */
#define MAXMOVE	300	/* size of the move history record */

#define GOSAVE	"go.sav"	/* default save file */

/* commonly used data types and structures */
union position
{	short	row_col;		/* note: 0 is an illegal position */
	struct
	{	unsigned char row;	/* range: 1 - MAXBOARD */
		unsigned char col;	/* range: 1 - MAXBOARD */
	} sub;
};

typedef union position pos_t;

/*
 * global parameters and state variables, likely to be of use in all
 * modules of the program
 */

/* general information information about the game and players */
char *version;		/* the program name / version */
char gamename[MAXLINE];	/* name that has been assigned to this game */
char p_name[32];	/* name of the human being we are playing */
int  boardsize;		/* number of lines on the board */
int  c_ability;		/* computer's ability (positive, I'm sure) */
int  my_color;		/* color the computer is playing */
int  p_ability;		/* player's ability (positive Q, negative Dan) */
int  spotpoints;	/* handicap given, by white, to black */

/* general parameters for the entire program */
int file_echo;		/* echo commands while processing files */
int no_blunders;	/* disallow self-atari moves */
int verbose;		/* verbose output is requested */

/* real numbers, relating to the current score */
int b_kills;	/* number of black prisoners taken */
int w_kills;	/* number of white prisoners taken */
int vacancies;	/* number of un-filled points on board */

/* real numbers, relating to the instantaneous state of the game */
int movenum;	/* number of the move about to be made */
int nxt_color;	/* color to move next */

/* parameters that affect the display of the game */
int darkness;		/* suppress display updates - a redraw is planned */
int fantasy;		/* suppress board updates - making imaginary moves */

/* estimates, describing the current perceived state of the board */
int dames;	/* estimated number of dames */
int b_terr;	/* estimated black points controlled */
int w_terr;	/* estimated white points controlled */

/* other incidental parameters */
int pillage;	/* the last move whose liberties have been canibalized */

/* declarations for general game functions */
void c_newgame( int bsize );
void h_reset();
void c_move( int color, char *position, int flags );
void c_unmove( int nmove );
void m_logmove( FILE *file, int num );
void c_handicap( int num );
void c_spot( char *arg );
int chkmove( char *arg );
void c_save( char *file );
void c_replay( char *arg );
char *nicedate( long date );
void c_debug( char *arg );
void c_info( char *arg );
void c_estimate( char *arg );
int confirm( char *str, ...);
void b_reset();
unsigned short b_set( unsigned row, unsigned col, unsigned color, int type, int shade, int moveno );
unsigned short b_tag( unsigned row, unsigned col, int type, int shade );
void b_boardsize( int size );
void b_remove( unsigned row, unsigned col );
void b_update( unsigned row, unsigned col );
void b_redraw( int all );

void l_reset();
void i_reset();
void delta_inf( unsigned row, unsigned col, unsigned color, int new );

/* declarations for display functions	*/
void d_init();
void d_msg( char *s, ...);
void d_header( char *s, ...);
void d_prompt( char *s, ...);
int d_readline( char *buf );
void d_text( char *str, ...);
void d_clear();
void d_cleanup();
void d_blank( unsigned row, unsigned col, unsigned color, int shade );
void d_value( int row, int col, int color, int shade, int num );
void d_stone( unsigned row, unsigned col, unsigned color, int shade, int num );
void d_update();
void d_showmove( int movnum, char *str );

void docmd( char *cmd );

int est_terr();
void showterr( char what );
void showinf();
