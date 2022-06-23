/*
 * module:
 *	pcdos.c
 *
 * purpose:
 *	pcdos specific functions
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "go.h"

#define INITFILE	"go.rc"	/* location of user profile */

/*
 * routine:
 *	machmain
 *
 * purpose:
 *	to perform machine specific initialization prior to any other
 *	initialization in the go program.  In particular, this routine
 *	is expected to initialize the user name, the name of the user's
 *	go profile, and other files that might be in machine specific
 *	locations.
 *
 * parms:
 *	ARGSUSED
 *	number of arguments passed to program
 *	pointers to those arguments
 *
 * returns:
 *	nothing - but argument text may have been changed.  
 *		  In particular, the first character of an
 *		  argument can be NULLed to prevent the main
 *		  go program from seeing it.
 */
void machmain( int argc, char **argv )
{	char *s;
	int i;
	static char namebuf[64];
	extern char *helpname;		/* name of the help-text file */ 
	extern char *initfile;		/* name of users go profile   */
	extern char *getenv(), *strcpy(), *strcat(); 
	extern int d_type;

	/* for test purposes, use my own help text file */
	helpname = "go.txt";

	/* determine the name of the user who is running us */
	s = getenv( "USER" );
	if (s)
		(void) strcpy( p_name, s );

	/* determine whether or not he has a go profile */
	(void) strcat( namebuf, INITFILE );
	if (access( namebuf, 04 ) == 0)
		initfile = namebuf;

	/* search the arguments for a display type force */
	for( i = 1; i < argc; i++ )
	{	if (argv[i][0] != '+')
			continue;

		/*
	         * a number is an explicit mode setting, anything
		 * else is a character, with some special meaning
		 */
		if (argv[i][1] >= '0'  &&  argv[i][1] <= '9')
			d_type = atoi( &argv[i][1] );
		else
			d_type = argv[i][1];

		argv[i][0] = 0;
	}
}
