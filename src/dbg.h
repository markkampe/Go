/*
 * module:
 *	dbg.h
 *
 * purpose:
 *	declarations and definitions for debugging output
 */

FILE *dbglog;	/* FILE structure for the debug log */
int debug;	/* word of debug flags */

/* 
 * TRACING macro is used to determine whether or not a particular
 * 	type of event is being traced
 */
#define TRACING(m)	((debug&(m)) ? (!fantasy) || (debug&D_fantasy) : 0)

/* bits for various debug options */
#define	D_all		0xff		/* all debugging options */

#define	D_moves		0x01		/* log moves that are made */
#define D_est		0x02		/* log estimates that are made */
#define D_kills		0x04		/* log group kills */
#define D_invalid	0x08		/* log invalid moves */
#define	D_affected	0x10		/* log info on affected groups */
#define D_fantasy	0x80		/* log imaginary moves too */
