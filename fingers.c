
//
// fingers - shopbot program generator for cutting finger joints
//
//	Questions...
//	(1) Does pgm create just the side requested all both side of an edge ?
//			curently only what is asked. Should create both sides to avoid
//			specification differences.

/*
:! gcc -g -o fingers %
*/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define	PGM_NAME		"Fingers"
#define	PGM_VERSION		"1.0"
#define	OPTION_STRING	"?c:d:j:l:o:n:s:t:v"

#define	TITLE_MAX 		50

#define	SIDE_A			0
#define	SIDE_B			1
#define	SIDE_BOTH		3
#define	SIDE_UNKNOWN	-1

#define	MAX_TOOL_DIAMETER	0.50
#define	MAX_JOINT_LENGTH	30.0
#define	MAX_EDGE_LENGTH		30.0

void usage( void );
void summary( char *, char * );
void generate( int );
void header( char * );
void footer( void );
void toolPath( char * , int  );
int validate( void );
void cut( int, int );
void subs( void );

typedef struct	{
	char * name;
	float	dia;
} Tool ;


Tool	tools[] = { {".25 End Mill", 0.25 }, {".5 End Mill", .498 } };
int		toolSelect = 1;

// foutput to stdout unless a filename is given;
FILE *	fout;

// command line parameters...
//	-o output filename
//	-l	workpiece length
//	-t	workpiece thickness
//	-j	joints
//	-n	name
//	-c	cut depth

char  fn[ PATH_MAX ];			// where to put the ShopBot program
int		fnSet = 0;				// monitors filename
char  * name = NULL;			// name provided by the user
int		side = SIDE_UNKNOWN;	// which side of the joint we are programming for
float	wlen = 0.0;				// the length of the joint
float	joints = 0.0;			// how many segments along the joint
float	thickness = 0.0;		// thickness of the work piece
float	diameter = 0.0;			// tool diameter provided by the user
float	jointLen = 0.0;			// calculated segment length
float	cutDepth = 0.0;			// max cut depth

int		verbose = 0;
#define	VERBOSE 	1
#define	PROLOGUE	2
#define	DEBUG 		4	// not used...

char * pgm = NULL;

int
main( int argc, char * argv[] )
{
	int	rc = 0;
	int	ch;
	char * s;
	char   si;

	fout = stdout;

	memset( fn, 0, PATH_MAX );
	pgm = argv[ 0 ];
	while((ch = getopt(argc,argv, OPTION_STRING )) != -1) {
		switch( ch ) {
      default:	//  '?' usually
        //fprintf(stderr, "%s: invalid option -%c\n\n", pgm, ch);
		opterr = 1;
        usage();
        exit(1);
        break;

      case '?': 						// tool diameter
	  	usage();
		exit(0);
		break;

      case 'c': 						// cut depth
		cutDepth = atof( optarg );
        break;

      case 'd': 						// tool diameter
		diameter = atof( optarg );
        break;

      case 'l': 						// workpiece length
		wlen = atof( optarg );
        break;

      case 'n': // job name
          name = optarg;				// job name
        break;

      case 's': // edge side A/B
          s = argv[ optind -1 ];
		  si = *s;
		  switch( si ) {
		  	default : fprintf( stderr, "Invalid side selection: %c\t Use A or B\n", si );
			opterr = 1;
			break;
		  case 'A' :
		  case 'a' :
		  	side = SIDE_A;
			break;
		  case 'B' :
		  case 'b' :
		  	side = SIDE_B;
			break;
		  case 'C' :
		  case 'c' :
		  	side = SIDE_BOTH;
			break;
		  }
        break;

      case 't': // workpiece thickness
		thickness = atof( optarg );
        break;

      case 'j': // number of joints
		joints = atof( optarg );
        break;

      case 'v': // verbose
		verbose |= VERBOSE;
        break;

      case 'o': // output filename
          strncpy( fn, optarg, PATH_MAX);
		  fnSet = 1;
        break;
		}
	}

	jointLen = wlen / joints;

	if( ! validate() ) {
		  	fprintf( stderr, "\nThe job validation failed!!\n", fn );
		  	fprintf( stderr, "Please review your job options and retry.\n", fn );
			exit( -2 );
	}


	switch( side ) {
		default :
		  	fprintf( stderr, "Side selection mix-up, quitting.\n", fn );
			break;

		case SIDE_BOTH:
			generate( SIDE_A );
			generate( SIDE_B );
			break;
		case SIDE_A:
			generate( SIDE_A );
			break;
		case SIDE_B:
			generate( SIDE_B );
			break;
	}

	fclose( fout );
	return rc;
}

//
// usage - give helpful information about this pogram
// 

void
usage() {
	fprintf( stderr, "%s - %s\n", PGM_NAME, PGM_VERSION );
	fprintf( stderr, "%s - {%s}\n", pgm, OPTION_STRING );
	fprintf( stderr, "\n\n" );
	fprintf( stderr, "\t%s - %s\n", "?", "Ask for help." );
	fprintf( stderr, "\t%s - %s\n", "c","Maximum tool cut depth.");
	fprintf( stderr, "\t%s - %s\n", "d","Tool diameter.");
	fprintf( stderr, "\t%s - %s\n", "j","Number of joint segments.");
	fprintf( stderr, "\t%s - %s\n", "l","Edge length." );
	fprintf( stderr, "\t%s - %s\n", "o","ShopBot file name." );
	fprintf( stderr, "\t%s - %s\n", "n","Job name." );
	fprintf( stderr, "\t%s - %s\n", "s","Side selection. A or B. Default is both." );
	fprintf( stderr, "\t%s - %s\n", "t","Work piece thickness." );
	fprintf( stderr, "\t%s - %s\n", "v","Be verbose." );
	fprintf( stderr, "\n\n" );
}

//
// summary - summarize this program's parameters and actions
//

void
summary( char * n, char * filename ) {
	fprintf( fout, "'\n'Job summary for %s\n", n );
	fprintf( fout, "'verbose\t%d\n", verbose );
	fprintf( fout, "'filename\t%s\n", filename );
	fprintf( fout, "'workpiece length\t%f\n", wlen );
	fprintf( fout, "'workpiece thickness\t%f\n", thickness );
	fprintf( fout, "'joints \t%f\n", joints );
	fprintf( fout, "'jointLen \t%f\n", jointLen );
	fprintf( fout, "'tool diameter \t%f\n", diameter );
	fprintf( fout, "\n'Assumes workpiece is homed with no offset....\n'\n" );
}

//
// generate all segments of the edge, for one or both sides of the edge.
// A edges have odd numbered segments removed.
// B edges have even numbered segments removed.
//
void
generate( int si ) {
	int	x = 0;
	char	fname[ PATH_MAX ];

	strcpy( fname, fn );
	if( si == SIDE_A )
		strcat( fname, "_A.sbp" );
	else
		strcat( fname, "_B.sbp" );

	// open output file
	fout = fopen( fname, "w" );
	if( fout == NULL ) {
		fprintf( stderr, "Could not open %s\n", fn );
		exit( -1 );
	}

	summary( name , fname);
	header( name );

	fprintf( fout, "'----------------------------------------------------------------\n" );
	fprintf( fout, "'Turning router ON\n" );
	fprintf( fout, "SO,1,1\n" );
	fprintf( fout, "PAUSE 2\n" );
	fprintf( fout, "'----------------------------------------------------------------\n" );
	fprintf( fout, "'\n" );

	for( x=1; x <= joints; x++ ) {		// for all joints
		cut( si, x );					// generate a joint subroutine.
	}
	footer();

	fclose( fout );

}

// cut - cut out segments....
void
cut( int sideSelect, int id ) {
	int	cside = id - 1;						// computers count from zero...
	float	yStart = cside * jointLen;		// this is where we start
	float	xStart = 0.0;					// seg subs reference these points....

	if( (sideSelect == SIDE_A) && !(id & 1) ) {
		return;
	} else if( (sideSelect == SIDE_B) && (id & 1) ) {
		return;
	}

	toolPath( tools[ toolSelect ].name, id );			// probably needs to change....
														// to construct actual parameters for tool
	// compute parameters and add a subroutine call to cut out the slot
	// put parameters into ShopBot variables

	float step = diameter / 2.0;				// establish work zone
	float baseX = (float) (0.0 - step);			// sides...
	float baseY = (float) (cside * jointLen);
	float top = (float) id * jointLen;			// top and bottom
	float bot = (float) cside * jointLen;

	fprintf( fout, "&startX = %.3f	' left edge X\n", baseX - step );
	fprintf( fout, "&startY = %.3f	' left edge Y\n", baseY + step );	// 
	fprintf( fout, "&bot = %.3f		' bottom of work area\n",  bot);	// 
	fprintf( fout, "&top = %.3f		' top of work area\n",  top);	// 
	fprintf( fout, "&lenX = %.3f	' length of x edge\n", thickness + diameter );	// 
	fprintf( fout, "&lenY = %.3f	' length of y edge\n", jointLen);	// 
	fprintf( fout, "\tGOSUB	sub1	' cut work piece\n" );

	fprintf( fout, "'----------------------------------------------------------------\n" );
}

void
header( char * t ) {

	time_t	now = 0;
	char *	today ;
	
	time( &now);

	fprintf( fout, "'%s\n", t );
	fprintf( fout, "'File created: %s\n", ctime( &now) );
	fprintf( fout, "'By %s - %s\n", PGM_NAME, PGM_VERSION );
	fprintf( fout, "'\n" );
	fprintf( fout, "'SHOPBOT FILE IN INCHES\n" );
	fprintf( fout, "' 25 is UNIT, 0 or 1. asuming 0 means inches...\n" );
	fprintf( fout, "IF %(25)=1 THEN GOTO UNIT_ERROR	'check to see software is set to standard\n" );
	fprintf( fout, "C#,90				 	'Lookup offset values\n" );
	fprintf( fout, "'\n" );
	fprintf( fout, "TR,12000,1\n" );
	fprintf( fout, "MS,0.08,0.05					' move speed: cut,plunge\n" );
	fprintf( fout, "JS,0.15,0.05					' jog speeds\n" );
	fprintf( fout, "VC,%.3f						' cutter diameter\n", diameter );
	fprintf( fout, "'\n" );
	fprintf( fout, "'\n" );
	fprintf( fout, "'----------------------------------------------------------------\n" );
}

void
footer() {

	fprintf( fout, "'----------------------------------------------------------------\n" );
	fprintf( fout, "'Turning router OFF\n" );
	fprintf( fout, "SO,1,0\n" );
	fprintf( fout, "END\n" );
	fprintf( fout, "UNIT_ERROR:\n" );
	fprintf( fout, "SO,1,0\n" );
	fprintf( fout, "C#,91					'Run file explaining unit error\n" );
	fprintf( fout, "END\n" );
	subs();
}

//
// subs - create subroutine to perform the actual cutting.
// for best accuracy the cut should move from right to left through the work piece. (jig specific ?)
// However, the easy way to do this is via the "cut rectangle" which does not allow for that control.
//
// Notes
//		Using CR (cut rectangle) and preset variables.
void
subs() {

	fprintf( fout, "'\n'\n" );
	fprintf( fout, "'------------------------ subroutines -----------------------------\n" );
	fprintf( fout, "'\n'\n'\tsub1 - cut segment of edge for a slot\n" );
	fprintf( fout, "' starting from a specific location at the bottom of a segment. remove the segment from the edge.\n" );
	fprintf( fout, "' &startX and &startY denote the starting location.\n" );
	fprintf( fout, "' &endX and &endY denotes the end of the cut.\n" );
	fprintf( fout, "'&bot  = bottom of work area\n");	// 
	fprintf( fout, "'&top  = top of work area\n" );	// 
	fprintf( fout, "'&lenX = length of x edge\n" );	// 
	fprintf( fout, "'&lenY = length of y edge\n" );	// 
	fprintf( fout, "'------------------------------------------------------------------\n" );
	fprintf( fout, "'\nsub1:'\n" );
	fprintf( fout, "MS,0.08,0.05						' move speed: cut,plunge\n" );
	fprintf( fout, "JS,0.15,0.05						' jog speeds\n" );
	fprintf( fout, "JZ,0.950000							' raise tool\n" );
	fprintf( fout, "J2,0.000000,0.000000				' home tool at start of cut\n" );
	fprintf( fout, "J3,&startX,&startY,0.200000			' position tool for cut\n" );
	fprintf( fout, "CR,&lenX,&lenY,'I',1,4,0.25,3,2,1	' cut rectangle built-i\n" );
	fprintf( fout, "JZ,0.950000							' raise tool\n" );
	fprintf( fout, "J2,0.000000,0.000000				' home tool at start of cut\n" );
	fprintf( fout, "'\n\tRETURN'\n'\n" );
	fprintf( fout, "'------------------------------------------------------------------\n" );
	fprintf( fout, "'------------------------------------------------------------------\n" );

}

void
toolPath( char * tool, int id ) {

	fprintf( fout, "'Toolpath Name = Profile %d\n", id );
	fprintf( fout, "'Tool Name   = %s\n", tool );
	fprintf( fout, "'----------------------------------------------------------------\n" );

}

//
// output a error message.
// if rc parameter is non-zero, include a small banner to draw attention to the error (s)
// that follows.

int
fail( int rc, char * msg ) {
	if( rc > 0 ) {
		fprintf( stderr, "\n\n**************************************************************\n" );
		fprintf( stderr, "n***********   setup errors     setup errors    ***************\n" );
		fprintf( stderr, "**************************************************************\n\n" );
	}
	fprintf( stderr, "\t%s\n", msg );
	return 0;
}


//
// validate job setup
//
// returns true if job looks OK, reasonable, possible.
//
// Easy is prone to errors, make user specify all aspects of the job....

int
validate() {
#define	BUFLEN	80
	int	rc = 1;
	char	buf[ BUFLEN ];
	int		diaOK = 1;

	// be careful with untrusted user input arguments....
	if( name == NULL ) {
		rc = fail( rc, "No job name specified, use -n option" );
	}

	if( fnSet == 0 ) {
		rc = fail( rc, "No output file specified, use -o option" );
	}

	if( side == SIDE_UNKNOWN ) {
		rc = fail( rc, "No side specified, use -s option" );
	} else switch( side ) {
		default :
			rc = fail( rc, "Incorrect side specified, use -s option" );
			break;
		case SIDE_A :
		case SIDE_B :
		case SIDE_BOTH :
			break;
	}

	if( diameter == 0.0 ) {
		diaOK = 0;
		rc = fail( rc, "No tool diameter specified, use -d option" );
	} else if( diameter < 0.0 ) {
		diaOK = 0;
		rc = fail( rc, "Negative tool diameters are not possible" );
	} else if( diameter > MAX_TOOL_DIAMETER ) {
		diaOK = 0;
		memset( buf, 0, BUFLEN );
		snprintf( buf,  BUFLEN, "Tool diameters larger than %.3f are not possible", MAX_TOOL_DIAMETER );
		rc = fail( rc, buf );
	}

	if( cutDepth == 0.0 ) {
		rc = fail( rc, "No tool cut depth specified, use -c option" );
	} else if( cutDepth < 0.0 ) {
		rc = fail( rc, "Negative tool cutDepth are not supported" );
	} else if( diaOK == 1 ) {
		if( cutDepth > diameter ) {
			memset( buf, 0, BUFLEN );
			snprintf( buf,  BUFLEN, "Tool cut depth greater than the tool diameter (%.3f) are not reccommended", diameter );
			rc = fail( rc, buf );
		}	
	} else {
		rc = fail( rc, "No tool cut depth verification due to diameter parameter problems" );
	}

	if( diaOK == 1 ) {
		if( diameter >= jointLen ) {
			rc = fail( rc, "The tool diameter specified is too large for the joint" );
		}
	} else {
		rc = fail( rc, "No tool diameter depth, joint segment length  verification due to diameter parameter problems" );
	}

	if( wlen == 0.0 ) {
		rc = fail( rc, "No edge length specified, use -l option" );
	} else if( wlen < 0.0 ) {
		rc = fail( rc, "Negative length edges are not supported" );
	} else if( wlen > MAX_EDGE_LENGTH ) {
		memset( buf, 0, BUFLEN );
		snprintf( buf,  BUFLEN, "Edges longer than %.3f inches are not supported", MAX_EDGE_LENGTH );
		rc = fail( rc, buf );
	}

	if( joints == 0.0 ) {
		rc = fail( rc, "Number of joint segments need to be specified, use -j option" );
	} else if( joints < 0.0 ) {
		rc = fail( rc, "Negative number of joint segments are not possible" );
	} 
	
	// reasonable checks....
	if( (joints * MAX_TOOL_DIAMETER) >= MAX_EDGE_LENGTH ) {
		memset( buf, 0, BUFLEN );
		snprintf( buf,  BUFLEN, "Edges longer than %.3f inches are not supported", MAX_EDGE_LENGTH );
		rc = fail( rc, buf );
	}

	if( thickness == 0.0 ) {
		rc = fail( rc, "Work piece thickness need to be specified, use -t option" );
	} else if( thickness < 0.0 ) {
		memset( buf, 0, BUFLEN );
		snprintf( buf, BUFLEN, "Negative number for work piece (%.3f) thickness are not recommended", thickness );
		rc = fail( rc, "Negative number for work piece thickness are not possible" );
	} 
	


	return rc;
}

