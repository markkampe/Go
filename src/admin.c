/*
 * module:
 *	admin
 *
 * purpose:
 *	to handle the administrative details of saving and
 *	restoring games from files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "go.h"

void c_save( char *file )
{	register int i;
	FILE *outfile;
	extern char *version;
	char *nicedate();

	outfile = fopen( file, "w" );
	if (outfile == NULL)
	{	d_msg("Unable to create output file: %s", file );
		return;
	}

	/* write a header at the front of the saved game */
	fprintf( outfile, "# GO game record, %s\n", version );
	fprintf( outfile, "#   Player name: %s", p_name );
	if (p_ability > 0)
		fprintf( outfile, ", ranking %d-Kyu\n", p_ability );
	else if (p_ability < 0)
		fprintf( outfile, ", ranking %d-Dan\n", -p_ability );
	else
		fprintf( outfile, "\n" );
	fprintf( outfile, "#   Game saved:  %s\n", nicedate(time(0)));
	fprintf( outfile, "#   Game name:   %s\n", gamename );

	if (boardsize != MAXBOARD)
		fprintf( outfile, "n %d\t(game played on a %d line board)\n",
			 boardsize, boardsize );

	fprintf( outfile, "p %s\n", p_name );
	if (p_ability)
		fprintf( outfile, "a %d\n", p_ability );

	fprintf( outfile, "g %s\n", gamename );

	if (no_blunders == 0)
		fprintf( outfile, "o b\t(%s said he wanted to)\n", p_name);

	/* write out all of the moves */
	for( i = 1; i < movenum; i++ )
		m_logmove( outfile, i );

	(void) fclose( outfile );
	d_msg( "%d moves saved to file %s", i-1, file );
}

/*
 * routine:
 *	c_replay
 *
 * purpose:
 *	purpose to replay one more move from a file
 *
 * parameters:
 *	number of lines to process
 *	name of new file
 *	process one more line
 */
void c_replay( char *arg )
{	int moves, limit, save;
	register char *s;
	static FILE *rfile;
	static char name[MAXLINE];
	char cmdbuf[MAXLINE];

	/* argument can be a number or a file name */
	if (arg == 0  ||  *arg == 0)
		moves = 1;
	else if (*arg >= '0' && *arg <= '9')
		moves = atoi( arg );
	else
	{	if (rfile)
		{	(void) fclose( rfile );
			rfile = NULL;
		}
		
		rfile = fopen( arg, "r" );
		if (rfile == NULL)
		{	d_msg( "Unable to open game file %s", arg );
			return;
		}
		(void) strcpy( name, arg );
		moves = 1;
	}

	if (rfile == NULL)
	{	d_msg( "No replay file is open" );
		return;
	}

	/* avoid distracting updates durring long replays */
	if (moves > 2)
	{	save = darkness;
		darkness = TRUE;
	}

	for( limit = moves + movenum; movenum < limit; )
	{	/* read the input line */
		if (fgets( cmdbuf, MAXLINE, rfile ) == NULL)
		{	d_msg( "End of file on %s", name );
			(void) fclose( rfile );
			rfile = NULL;
			break;
		}

		/* strip off the newline */
		for( s = cmdbuf; *s; s++ );
		if ( s[-1] == '\n' )
			s[-1] = 0;

		/* echo the command and perform it */
		if (file_echo)
			d_prompt( "%s:%s", name, cmdbuf );
		docmd( cmdbuf );
	}

	if (moves > 2)
	{	darkness = save;
		b_redraw( TRUE );
	}
}

/*
 * routine:
 *	nicedate
 *
 * purpose:
 *	to return a simply formatted date
 */
char *nicedate( long date )
{
	struct tm *tm, *localtime();
	static char dbuf[32];

	tm = localtime( &date );
	(void) sprintf( dbuf, "%d/%d/%d %d:%02d",
		tm->tm_mon, tm->tm_mday, tm->tm_year,
		tm->tm_hour, tm->tm_min );

	return( dbuf );
}

