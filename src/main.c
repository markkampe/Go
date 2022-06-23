/*
 * module
 *	main.c
 *
 * purpose
 *	GO program
 *		initialization, parameter processing, main loop
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "go.h"
#include "move.h"

int	doneflg;		/* we are done processing */
int	errcode;		/* return code */
char *initfile;			/* user's go initialization file */

void help(char *stuff );
void cmdloop( char *arg );
void machmain( int argc, char **argv );

/*
 * routine:
 *	docmd
 *
 * purpose:
 *	to perform a single command
 *
 * parms:
 *	command to be performed
 *
 * note:
 *	moves can be specified in either of two formats: <letter><number>
 *	or <number><letter>.  All other commands start with a letter.
 *	For convenience, we allow a slightly context sensitive input
 *	format.  If a command is a letter followed immediately by a 
 *	number, we treat it as a move.  If the letter is followed by
 *	a non-number (such as a blank or another letter) we treat it
 *	as a non-move command.  This creates a marginally ambiguous input
 *	format, where "h4" is a move but "h 4" is a request for four
 *	handicap stones.  The ambiguity is regrettable, but it probably
 *	lets people do what they want more easily than some other 
 *	mechanism that could be used to distinguish moves from commands:
 *		require each move to be prefixed with a command m, b, w, ...
 *		require all commands to be upper case or something
 *		require all moves to be specified with the # first
 *		
 */
void docmd( char *cmd )
{	register char *arg;
	int number;

	/* scan off any leading white space */
	while( *cmd == ' ' || *cmd == '\t' )
		cmd++;

	/* check for an semi-ambiguous move specification */
	if (*cmd >= 'a' && *cmd <= 's' && cmd[1] >= '0' && cmd[1] <= '9')
	{	c_move( nxt_color, cmd, 0 );
		return;
	}

	/* find the first argument */
	arg = cmd+1;
	while( *arg && *arg!=' ' && *arg!='\t')
		arg++;

	while( *arg && *arg==' ' || *arg=='\t')
		arg++;
	if (*arg == 0)
		arg = 0;

	switch( *cmd )
	{ /*
	   * administrative commands
	   */
	  case 'g': /* gamename */
		if (arg)
			(void) strcpy( gamename, arg );
		d_header( "" );
		break;

	  case 'f': /* process another command file */
		if (arg)
			cmdloop( arg );
		else
			d_msg( "file command requires file name argument" );
		break;

	  case 'n': /* new game - possibly changing board size */
		if (arg)
			number = atoi( arg );
		else
			number = boardsize;

		if (movenum == 1  || confirm("Start a new %d line game? (y)", 
						number))
			c_newgame( number );
		break;

	  case 'q': /* quit immedately */
		if (movenum == 1  || confirm("Quit without saving? (y)"))
			doneflg++;
		break;

	  case 's': /* save game state */
		if (arg == 0)
			arg = GOSAVE;
		if (confirm("Save game in file: %s? (y)", arg ))
			c_save( arg );
		break;

	  case 'x': /* save and exit */
		if (arg == 0)
			arg = GOSAVE;
		if (confirm("Save game in %s and exit? (y)", arg))
		{	c_save( arg );
			doneflg++;
		}
		break;
			
	  case '#': /* comment */
		if ( !darkness )
			d_msg( arg );
		break;

	  /* 
	   * COMMANDS TO MAKE MOVES 
	   */
	  case '0': case '1': case '2': case '3': case '4': 
	  case '5': case '6': case '7': case '8': case '9': 
		c_move( nxt_color, cmd, 0 );
		break;

	  case '-': /* pass */
		c_move( nxt_color, "-", 0 );
		break;

	  case 'a': /* specify player ability */
		if (arg)
		{	p_ability = atoi( arg );
		}

		if (!darkness)
			d_msg( "Abilities: program=%d-Kyu, player=%d-%s, handicap=%d",
				c_ability, p_ability, 
				p_ability > 0 ? "Kyu" : "Dan", spotpoints );
		break;

	  case 'b': /* black move */
		c_move( BLACK, arg, 0 );
		break;

	  case 'h': /* generic handicap */
		/* if we don't know how good player is, assume fair game */
		if (p_ability == 0)
			p_ability = c_ability;

		/* if no handicap is specified, compute based on abilities */
		if (arg == 0)
		{	if (p_ability <= c_ability)
			{	c_handicap( c_ability - p_ability );
				my_color = BLACK;
			} else if (p_ability > c_ability)
			{	c_handicap( p_ability - c_ability );
				my_color = WHITE;
			}
		} else
			c_handicap( atoi( arg ) );
			
		if (!darkness)
			d_msg( "%s play black with a %d-stone handicap", 
				my_color ? "You" : "I", spotpoints );
		break;

	  case 'B': /* specific handicap stone */
		c_spot( arg );
		break;

	  case 'w': /* white move */
		c_move( WHITE, arg, 0 );
		break;

	  case 'u': /* unmove */
		if (arg && *arg > '0' && *arg <= '9')
			c_unmove( atoi( arg ) );
		else
			c_unmove( 1 );
		break;

	  case 'r': /* replay - one move at a time */
		c_replay( arg );
		break;

	  /*
	   * miscelaneous commands
	   */
	  case 'd':	/* display type */
		d_msg( "display options are not yet implemented" );
		break;

	  case 'o': /* program options */
		while( arg && *arg && *arg != ' ' && *arg != '\t')
		    switch( *arg++ )
		    { case 'v':
			verbose = 1;
			break;
		      case 'q':
			verbose = 0;
			file_echo = 0;
			break;
		      case 'b':
			no_blunders = 0;
			break;
   		      case 'n':
			no_blunders = 1;
			break;
		     case 'e':
			file_echo = 1;
			break;
		    }

		if (!darkness)
			d_msg( "Options: verbose=%d, noblunder=%d, echo=%d", 
				verbose, no_blunders, file_echo );
		break;

	  case 'D': /* diagnostic functions */
		c_debug( arg );
		break;

	 case 'i':	/* information */
		c_info( arg );
		break;

	  case 'e':	/* estimation */
		c_estimate( arg );
		break;

	  case 'p':	/* specify player name */
		if (arg)
		{	strcpy( p_name, arg );
			d_header( "" );
		}
		break;

	/*
	 * unrecognized commands
	 */
	default:
		d_msg( "unrecognized command" );
		arg = 0;
		sleep( 2 );

	  case '?':
		help( arg );
		b_redraw( TRUE );
		break;
	}
}

/*
 * routine:
 *	cmdloop
 *
 * purpose:
 *	command processing loop for GO program
 *
 * parms:
 *	command input file
 */
void cmdloop( char *cmdfile )
{	register char *s;
	FILE *infile;
	char cmdbuf[ MAXLINE ];
	int save;

	/*
	 * if we are using a file, open it 
	 */
	if (cmdfile)
	{	infile = fopen( cmdfile, "r" );
		if (infile == NULL)
		{	d_msg( "Unable to open command file: %s", cmdfile );
			errcode = -1;
			return;
		} else
		{	d_msg( "Processing commands from file: %s", cmdfile );

			/* suppress screen updates while processing files */
			save = darkness;
			darkness = TRUE;

			while( doneflg == 0 )
			{	/* read the input line */
				if (fgets( cmdbuf, MAXLINE, infile ) == NULL)
					break;

				/* strip off the newline */
				for( s = cmdbuf; *s; s++ );
				if ( s[-1] == '\n' )
					s[-1] = 0;

				/* echo the command and perform it */
				if (file_echo)
					d_prompt( "%s:%s", cmdfile, cmdbuf );
				docmd( cmdbuf );
			}

			/* update the screen */
			darkness = save;
			b_redraw( TRUE );

			/* close the command file */
			(void) fclose( infile ); 
		}
	} else
	{	/* processing input from the console */
		while( doneflg == 0 )
		{	/* prompt for input */
			d_prompt( "GO: " );

			/* read the input line */
			if (d_readline( cmdbuf ) == 0)
				continue;

			/* process it */
			docmd( cmdbuf );
		}
	}
}

/*
 * routine:
 *	prompt
 *
 * purpose:
 *	prompt for a confirmation, wait for it, and return the result
 *
 * arguments:
 *	VARARGS1
 *	same as printf
 *
 * returns:
 *	0	non-confirmation
 *	1	confirmation

FIX FOR VARARGS 

int confirm( char *str, int a1, int a2, int a3 )
{
	char inbuf[ MAXLINE ];

	d_prompt( str, a1, a2, a3 );

	if (d_readline( inbuf ))
	{	if (inbuf[0] == 0 || inbuf[0] == 'y' || inbuf[0] == 'Y')
			return( 1 );
		else
			return( 0 );
	} else
		return( 1 );
}
 */

int main( int argc, char **argv )
{	register int i;
	register char *s;

	/* give the machine specific code first crack at everything */
	machmain(argc, argv);

	/* suppress screen updates during initialization */ 
	darkness = TRUE;

	/* initialize the display and screen */
	d_init();

	/* start a new game on a standard board */
	c_newgame( MAXBOARD );

	/* if user is a regular, run his initialization file */
	if (initfile)
		cmdloop( initfile );

	/* process the arguments */
	for( i = 1; i < argc; i++ )
	{	/* argument is a command or a file of commands */
		s = argv[i];
		if (*s == '-')
		{	if (file_echo)
				d_prompt( "arg: %s", &s[1] );
			docmd( &s[1] );
		} else if (*s == 0)
			continue;	/* machdep must have taken it */
		else
		{	/* argument seems to be a file name */
			cmdloop( s );
		}

		/* see if we should stop yet */
		if (errcode  ||  doneflg)
			break;
	}

	/* re-enable screen updates, and put up a reasonable one */
	darkness = FALSE;
	b_redraw( TRUE );

	/* and go on to process terminal input */
	if (!doneflg &&  !errcode)
		cmdloop( (char *) NULL );

	/* cleanup and return */
	d_cleanup();
	exit( errcode );
}
