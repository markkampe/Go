/*
 * module
 *	help.c
 *
 * purpose
 *	to diplay help information
 */
#include <stdio.h>
#include <string.h>
#include "go.h"

#define	GOHELP	"/usr/games/go/go.txt"

char *helpname = GOHELP;/* name of the file to open for help text */
FILE *helpfile;		/* file structure for the open help file */

/* display the next help-stanza worth of information */
void showhelp()
{	int lineno = 0;
	char linebuf[128];

	while( fgets( linebuf, sizeof linebuf, helpfile ) )
	{	if (strncmp( linebuf, "*HELP", 5 ) == 0)
			break;
		if (strncmp( linebuf, "*MORE", 5 ) == 0)
		{	(void) confirm( "Enter a return for next help screen" );
			lineno = 0;
			continue;
		}
		d_text( linebuf, lineno++ );
	}

	(void) confirm( "Enter a return to resume the game" );
}

/* locate the help text for a particular command */
int findhelp( char code )
{	register char *s;
	char linebuf[128];

	rewind( helpfile );

	/* search the help text file for lines beginning with *HELP */
	while( fgets( linebuf, sizeof linebuf, helpfile ) )
	{	if (strncmp( linebuf, "*HELP", 5 ) == 0)
		{	/* scan to the list of applicable help codes */
			for( s = &linebuf[5]; *s==' ' || *s=='\t'; s++ );

			/* see if it includes the specified code */
			while ( *s && *s!=' ' && *s!='\t' )
				if (*s++ == code)
					return( 1 );
		}
	}

	/* we didn't find the one we were looking for */
	return( 0 );
}

/* display a screen of help information */
void help( char *stuff )
{	
	if (helpfile == NULL)
		helpfile = fopen( helpname, "r" );

	if (helpfile == NULL)
	{	d_msg( "No help available, unable to open help text file <%s>",
			helpname );
		return;
	}

	if (stuff == 0)
		stuff = "?";

	if (findhelp( *stuff ))
		showhelp();
	else
		d_msg( "No help information available for <%s>", stuff );
}

