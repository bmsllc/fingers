//
// fingers - shopbot program generator for cutting finger joints
// aka comb joint, box-pin or box joint
//	Questions...
//	(1) Does pgm create just the side requested all both side of an edge ?
//			curently only what is asked. Should create both sides to avoid
//			specification differences.
// Softwood Cedar Pine Spruce
// Hardwood Ash Aspen Balsa Birch Cherry Elm Hazel Linden/ Lime/ Basswood Mahogany Maple Oak Teak Walnut
/*
:! gcc -g -o fingers %
*/
//
// debug notes:
// use fg in preview mode for single stepping the bot.
// use got line to bypass setup/initialization code.
// look for edge taylor (sp?) on shopbot site


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define	PGM_NAME		"Fingers"
#define	PGM_VERSION		"1.0"
#define	OPTION_STRING	"?Bc:d:j:l:m:n:o:s:t:vw:"

// command line parameters...
//  -B put dummy command in clipboard
//  -c cutDepth
//  -d tool diameter
//  -j number of joint segments
//  -l length of edge
//	-m cut method
//	-n job name
//	-o output filename
//	-l	workpiece length
//	-s joint side to cut
//	-t	workpiece thickness
//	-v verbose flag
//  -c cutWidth


#define	TITLE_MAX 		50

#define	SIDE_UNKNOWN	0
#define	SIDE_A			1
#define	SIDE_B			2
#define	SIDE_BOTH		3

char *	sides[] = { "Unknown", "Side A", "Side B", "Both sides" };

#define	MAX_TOOL_DIAMETER	0.50
#define	MAX_JOINT_LENGTH	30.0
#define	MAX_EDGE_LENGTH		30.0
#define MAX_DEPTH_RATIO		5.0
#define MAX_CUT_WIDTH		0.50	// as a percentage
#define MAX_SUB_BUFFER		60000	// subroutine space
		
#define	METHOD_ZERO				0
#define	METHOD_ONE				1
#define	METHOD_TWO				2

#define	FALSE					0
#define	TRUE					1

void dummy( void  );
void usage( void );
void summary( char *, char * );
void generate( int );
void header( char * );
void footer( void );
void toolPath( char * , int  );
int validate( void );
int finalCheck( void );
void cut( int, int );
void sub1( void );
void sub2( void );
void errorExit( int e  );
float evenCuts( float * cutD, float ratio );

#if 0
typedef struct	{
	char * name;
	float	dia;
} Tool ;
#endif


#if 0
Tool	tools[] = { {".25 End Mill", 0.25 }, {".5 End Mill", .498 } };
int		toolSelect = 1;
#endif

// foutput to stdout unless a filename is given;
FILE *	fout = NULL;
FILE *	fsub = NULL;

int		method = METHOD_ZERO;	// no default cut method
int		didSub2 = FALSE;		// one sub2 per file
char	fn[ PATH_MAX ];			// where to put the ShopBot program
char	fname[ PATH_MAX ];
char	subfname[ PATH_MAX ];
int		fnSet = 0;				// monitors filename
char  * name = NULL;			// name provided by the user
int		side = SIDE_UNKNOWN;	// which side of the joint we are programming for
float	wlen = 0.0;				// the length of the joint
int		joints = 0;				// how many segments along the joint
float	thickness = 0.0;		// thickness of the work piece
float	diameter = 0.0;			// tool diameter provided by the user
float	jointLen = 0.0;			// calculated segment length
float	cutDepth = 0.0;			// max cut depth
float	cutWidth = 0.0;			// max cut width
float	moveSpeed = 0.08;		// actual machine number
float	plungeSpeed = 0.05;	// actual machine number
float	jogSpeed = 0.15;		// actual machine number
float	jogPlungeSpeed = 0.15;	// actual machine number

float	theCut = 0.0;
float	passes = 0.0;
float	depth = 0.0;
float	minimumSegment = 0.0;
float	halfDiameter = 0.0;
float	X1  = 0.0;
float	X2  = 0.0;
float	Y1  = 0.0;
float	Y1a = 0.0;
float	Y2  = 0.0;
float	Y2a = 0.0;
float	Y3  = 0.0;
float	Y4  = 0.0;

int		verbose = 0;
#define	VERBOSE 	1
#define	PROLOGUE	2
#define	DEBUG 		4	// not used...

// these variables are used during the actual cuts
float step = 0.0;
float baseX = 0.0;
float baseY = 0.0;
float top = 0.0;
float high = 0.0;
float bot = 0.0;
float low = 0.0;

char *  subBuffer = NULL;		// holds subroutine code lines
size_t	subSize = 0;			// maximum subroutine size allowed
size_t	subUsed = 0;			// maximum subroutine size used

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
		opterr = 1;
        usage();
        exit(1);
        break;

      case '?': 						// tool diameter
	  	usage();
		exit(0);
		break;

      case 'B': 						// stuff clipboard
	  	dummy();
		exit( 0 );
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

      case 'm': 						// cutting method
		method = atoi( optarg );
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
		joints = atoi( optarg );
        break;

      case 'v': // verbose
		verbose |= VERBOSE;
        break;

      case 'o': // output filename
          strncpy( fn, optarg, PATH_MAX);
		  fnSet = 1;
        break;
      
	  case 'w': 						// cut width
		cutWidth = atof( optarg );
        break;

		}
	}

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
			didSub2 = FALSE;		// one sub2 per file
			generate( SIDE_A );
			didSub2 = FALSE;		// one sub2 per file
			generate( SIDE_B );
			break;
		case SIDE_A:
			didSub2 = FALSE;		// one sub2 per file
			generate( SIDE_A );
			break;
		case SIDE_B:
			didSub2 = FALSE;		// one sub2 per file
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
	fprintf( stderr, "\t%s - %s\n", "B","Put dummy command line in clipboard, for pasting....");
	fprintf( stderr, "\t%s - %s\n", "c","Maximum vertical tool cut depth, per pass.");				// cutDepth
	fprintf( stderr, "\t%s - %s\n", "d","Tool diameter.");
	fprintf( stderr, "\t%s - %s\n", "j","Number of joint segments.");
	fprintf( stderr, "\t%s - %s\n", "l","Edge length." );
	fprintf( stderr, "\t%s - %s\n", "m","Cut method 1 or  2. Method one is the default." );
	fprintf( stderr, "\t%s - %s\n", "n","Job name." );
	fprintf( stderr, "\t%s - %s\n", "o","ShopBot file name." );
	fprintf( stderr, "\t%s - %s\n", "s","Side selection. A, B, or C. Default is both (C)." );
	fprintf( stderr, "\t%s - %s\n", "t","Work piece thickness, i.e. finger length..." );
	fprintf( stderr, "\t%s - %s\n", "v","Be verbose." );
	fprintf( stderr, "\t%s - %s\n", "w","Maximum horizontal tool cut width, per pass.");			// cutWidth
	fprintf( stderr, "\n\n" );
}

void
dummy( ) {
FILE *	cb;

	cb = fopen( "/dev/clipboard", "w" );
	if( cb == NULL ) {
		fprintf( stderr, "Error openinmg clipboard...\n\n" );
	}
	// #define	OPTION_STRING	"?Bc:d:j:l:n:o:s:t:vw:"
	fprintf( cb, "%s -c -d -j -l -n -o -s -t -w", pgm );
	fclose( cb );
}

//
// summary - summarize this program's parameters and actions
//

void
summary( char * n, char * filename ) {
	fprintf( fout, "'\n'Job summary for %s\n", n );
	fprintf( fout, "'verbose\t%d\n", verbose );
	fprintf( fout, "'filename\t%s\n", filename );
	fprintf( fout, "'workpiece length\t%.3f\n", wlen );
	fprintf( fout, "'workpiece thickness\t%.3f\n", thickness );
	fprintf( fout, "'joints \t%d\n", joints );
	fprintf( fout, "'jointLen \t%.3f\n", jointLen );
	fprintf( fout, "'tool diameter \t%.3f\n", diameter );
	fprintf( fout, "'cut width \t%.3f\n", cutWidth );
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

	strcpy( fname, fn );				// setup SBP file
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

	if( method == METHOD_TWO ) {
		strcpy( subfname, fn );			// setup subroutine two file
		strcat( subfname, ".sub" );
		// open output file
		
		fsub = fopen( subfname, "w+" );
		if( fsub == NULL ) {
			fprintf( stderr, "Could not open subroutine file %s\n", subfname );
			perror( subfname );
			exit( -1 );
		}
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

	if( fout != NULL ) {
		fclose( fout );
	}
	if( fsub != NULL ) {
		fclose( fsub );
	}

}

// cut - cut out segments.... (profiles)
// cut is called once per segment.
//
// inputs:
//		sideSelect - A/B
//		id			- segment ID  (base 1)
//
// METHOD_ONE uses the built-in CR command.
//
// CR may not be ideal for this process. The design of the jig suggests that a right to left
// motion when cutting would be better than a left to right motion. A second cutting method may be 
// provided for in the future.
//
// METHOD_TWO uses custom program to hog out the segment.
//

void
cut( int sideSelect, int id ) {

	int	segment = id - 1;					// computers count from zero...
	theCut = thickness;
	passes = evenCuts( &theCut, (diameter / MAX_DEPTH_RATIO) );
	depth = 0.0;				// starting depth
	halfDiameter = diameter / 2.0;			// horizontal position stepping

	if( theCut >= halfDiameter ) {
		fprintf( stderr, "The cut width(%.3f) and the tool diameter(%.3f) aspects are not correct for this joint.\n", cutWidth, diameter );
		fprintf( stderr, "Change either -c or -d options, or both, to correct the problem.\n" );
		exit(-1);
	}

	if( (sideSelect == SIDE_A) && !(id & 1) ) {
		return;
	} else if( (sideSelect == SIDE_B) && (id & 1) ) {
		return;
	}

	// compute parameters and add a subroutine call to cut out the slot
	// put parameters into ShopBot variables

	step = cutWidth;						// horizontal cut stepping
											// establish work zone
	baseX = (float) 0.0;					// workpiece is homed to x=0.0
	baseY = (float) (segment * jointLen);	// Y height varies with segment

	top = (float) baseY + jointLen;			// top and bottom
	bot = (float) baseY;

	high = top - diameter;					// end of work area after top cut
	low = bot + diameter;					// start of work area after bot cut
	float toolHeight = 0.0;

	float	startLine = baseY + halfDiameter;			// Y1
	float	topLine = baseY + jointLen - halfDiameter;	// Y2
	float	finishLine = baseY + diameter;				// Y3

	X1  = baseX - halfDiameter;
	X2  = baseX + thickness + halfDiameter;
	Y1  = baseY + cutWidth +halfDiameter;
	Y1a = baseY + halfDiameter - cutWidth ;
	Y2  = baseY + jointLen - cutWidth - halfDiameter;
	Y2a = baseY + jointLen - halfDiameter;
	Y3  = baseY + cutWidth + diameter;
	Y4  = baseY + jointLen - diameter - cutWidth + cutWidth;

//	if( finalCheck() ) {
//	}

	fprintf( fout, "'----------------------------------------------------------------\n" );
	fprintf( fout, "'----   %s - segment %d  %.3f - %.3f  %.3f \n", sides[ sideSelect ], segment, baseY, top, jointLen );
	fprintf( fout, "'----------------------------------------------------------------\n" );
	fprintf( fout, "					' define segment work area\n" );
	fprintf( fout, "&X1 = %.3f\n", X1);
	fprintf( fout, "&X2 = %.3f\n", X2);
	fprintf( fout, "&Y1 = %.3f\n", Y1);
	fprintf( fout, "&Y1a = %.3f\n", Y1a);
	fprintf( fout, "&Y2 = %.3f\n", Y2);
	fprintf( fout, "&Y2a = %.3f\n", Y2a);
	fprintf( fout, "&Y3 = %.3f\n", Y3);
	fprintf( fout, "&Y4 = %.3f\n", Y4);

	fprintf( fout, "&startX = %.3f		' horizontal right edge X\n", baseX + thickness + halfDiameter );
	fprintf( fout, "&endX = %.3f		' horizontal left edge X\n", baseX - halfDiameter );
	fprintf( fout, "&lowY = %.3f		' vertical bottom edge Y\n", baseY + diameter );
	fprintf( fout, "&hiY = %.3f			' vertical top edge Y\n", baseY + jointLen - diameter );
	fprintf( fout, "&startY = %.3f		' vertical Y\n", baseY + halfDiameter );
	fprintf( fout, "&endY = %.3f		' vertical Y\n", baseY + jointLen - halfDiameter );
	fprintf( fout, "&finishLine = %.3f\n", startLine + diameter );
	fprintf( fout, "&safeHeight = %.3f	' left edge Y\n\n",  1.0 );	// 

	fprintf( fout, "&bot = %.3f		' bottom of segment area\n",  bot);	// 
	fprintf( fout, "&top = %.3f		' top of segment area\n",  top);	// 
	fprintf( fout, "&lenX = %.3f	' length of x edge\n", thickness + (diameter * 2.0 ) );	// 
	fprintf( fout, "&lenY = %.3f	' length of y edge\n", jointLen);	// 
	fprintf( fout, "&cutY = &top	' top cutting edge of Y\n"  );	// 
	fprintf( fout, "&step = %.3f	' Y stepping amount\n", step  );	// 
	fflush( fout );

	switch( method ) {
		default:
			fprintf( stderr, "Invalid method (%d) use in cut\n", method  );
			exit( 1 );
			break;

		case METHOD_ONE	 : 		// sub1 setup
			// In this method the position only has to be established once, at the beginning since
			// the built-in CR command knows how to cut the rectangle.
			fprintf( fout, "'-----  setup has changed sine testing OK  ----------------------\n" );
			fprintf( fout, "\tGOSUB	sub1	' cut work piece\n" );

			fprintf( fout, "'----------------------------------------------------------------\n" );
			break;

		case METHOD_TWO : 						// sub2 needs to cut multiple passes at different cut depths...

			// In this method the position has to be established before each cut.
			// create setup and code to call custom sub2 routine to hog out one segment
			// these ShopBot variables will vary per segment
			fprintf( fout, "&bot = %.3f		' bottom of segment area\n",  bot);	// 
			fprintf( fout, "&top = %.3f		' top of segment area\n",  top);	// 
			fprintf( fout, "&cbot = %.3f	' bottom of center area\n",  bot + diameter );	// 
			fprintf( fout, "&ctop = %.3f	' top of center area\n",  top - diameter );	// 
			fprintf( fout, "&lenX = %.3f	' length of x edge\n", thickness + (diameter * 2.0 ) );	// 
			fprintf( fout, "&lenY = %.3f	' length of y edge\n", jointLen);	// 


			// compute number of times sub2 will be called per segment
			for( depth = theCut; depth <= thickness; depth += theCut) {
				if( depth > thickness )
					depth = thickness;
				fprintf( fout, "&depth = %.3f	' set cutting depth\n", depth );
				fprintf( fout, "\tGOSUB	sub2	' cut segment at depth %.3f\n", depth );
			}
			//fprintf( fout, "' -- setup next pass -- \n" );
			//fprintf( fout, "\tGOSUB	sub2	' cut segment from work piece\n" );
			fprintf( fout, "'------------------------------------------------------------------\n" );

			fprintf( fout, "'------------------------------------------------------------------\n" );
			if( didSub2 == FALSE ) {
				didSub2 = TRUE;					// only done once per file....
				// open sub2 file....
				// create custom routine to hog out segment
				//   Absolute cut bot and top to mark segment
				//   Relative cut to remove center area
				fprintf( fsub, "'\n'\n" );
				fprintf( fsub, "'------------------------ subroutines -----------------------------\n" );
				fprintf( fsub, "'--- cut right to left                                          ---\n" );
				fprintf( fsub, "'--  requires startX, endX, startY, endY     			        --\n" );
				fprintf( fsub, "'--  sub2 CAN NOT HAVE HARD Y ADDRESSES CODED WITHIN !!    		--\n" );
				fprintf( fsub, "'------------------------------------------------------------------\n" );
				fprintf( fsub, "'\nsub2:'\n" );
				fprintf( fsub, "SA									' absolute addressing\n" );
				//fprintf( fsub, "&startingX = &X2				' setup abs addresses\n" );
				//fprintf( fsub, "&endingX = &X1\n" );
				fprintf( fsub, "&startingY = &Y1\n" );
				fprintf( fsub, "&endingY = &Y1\n" );

				fprintf( fsub, "JZ,&safeheight						' raise tool\n" );
				fprintf( fsub, "J2,0.000000,0.000000				' jog home at start of cut\n" );
				fprintf( fsub, "\tGOSUB	sub3						' make first cut \n\n" );

				//fprintf( fsub, "&startingX = &X2				' setup abs addresses\n" );
				//fprintf( fsub, "&endingX = &X1\n" );
				fprintf( fsub, "&startingY = %.3f\n", Y1a );
				fprintf( fsub, "&endingY = %.3f\n", Y1a );

				fprintf( fsub, "JZ,&safeheight						' raise tool\n" );
				fprintf( fsub, "J2,0.000000,0.000000				' jog home at start of cut\n" );
				fprintf( fsub, "\tGOSUB	sub3						' make first cut \n\n" );

				//fprintf( fsub, "&startingX = &X2				' setup abs addresses\n" );
				//fprintf( fsub, "&endingX = &X1\n" );
				fprintf( fsub, "&startingY = &Y2\n" );
				fprintf( fsub, "&endingY = &Y2\n" );

				fprintf( fsub, "JZ,&safeheight						' raise tool\n" );
				fprintf( fsub, "J2,0.000000,0.000000				' jog home at start of cut\n" );
				fprintf( fsub, "\tGOSUB	sub3						' make first cut \n\n" );

				//fprintf( fsub, "&startingX = &X1				' reverse directioncut\n" );
				//fprintf( fsub, "&endingX = &X2\n" );
				fprintf( fsub, "&startingY = &Y2A\n" );
				fprintf( fsub, "&endingY = &Y2A\n" );

				fprintf( fsub, "JZ,&safeheight						' raise tool\n" );
				fprintf( fsub, "J2,0.000000,0.000000				' jog home at start of cut\n" );
				fprintf( fsub, "\tGOSUB	sub4						' make first cut \n\n" );

				float	y;
				fprintf( fsub, "\n'clearing the segment from top to bottom: %.3f, %.3f\n", Y4, Y3 );
				for( y=Y4; y > Y3; y -= step ) {
					//fprintf( fsub, "&startingX = &X2				' setup abs addresses\n" );
					//fprintf( fsub, "&endingX = &X1\n" );

					fprintf( fsub, "&startingY = %.3f\n", y );	// PRPBLEM... needs to be different in 
					fprintf( fsub, "&endingY = %.3f\n", y );	// different segments...

					fprintf( fsub, "JZ,&safeheight						' raise tool\n" );
					fprintf( fsub, "J2,0.000000,0.000000				' jog home at start of cut\n" );
					fprintf( fsub, "\tGOSUB	sub3						' make the cut \n\n" );
				}
				fprintf( fsub, "\tRETURN'\n'\n" );

				// sub3 does the real segment cut at a depth determined by sub2.
				fprintf( fsub, "'------------------------------------------------------------------\n" );
				fprintf( fsub, "'--  single cut right to left                                    --\n" );
				fprintf( fsub, "'--  requires startingX, endingX, startingY, endingY             --\n" );
				fprintf( fsub, "'------------------------------------------------------------------\n" );
				fprintf( fsub, "\nsub3:'\n" );
				fprintf( fsub, "J3,&startingX,&startingY,&safeHeight	' position tool for cut\n" );
				fprintf( fsub, "MZ,-&depth								' drop tool to cut height\n" );
				fprintf( fsub, "M3,&endingX,&endingY-&depth				' cut right to left\n" );
				fprintf( fsub, "MZ,-&safeHeight							' drop tool to cut height\n" );
				fprintf( fsub, "'\n\tRETURN'\n'\n" );
				fprintf( fsub, "'------------------------------------------------------------------\n" );
				fprintf( fsub, "'--  single cut left to right                                    --\n" );
				fprintf( fsub, "'--  requires startingX, endingX, startingY, endingY             --\n" );
				fprintf( fsub, "'------------------------------------------------------------------\n" );
				fprintf( fsub, "\nsub4:'\n" );
				fprintf( fsub, "J3,&startingX,&startingY,&safeHeight	' position tool for cut\n" );
				fprintf( fsub, "MZ,-&depth								' drop tool to cut height\n" );
				fprintf( fsub, "M3,&endingX,&endingY-&depth				' cut left to right\n" );
				fprintf( fsub, "MZ,-&safeHeight							' drop tool to cut height\n" );
				fprintf( fsub, "'\n\tRETURN'\n'\n" );
				fprintf( fsub, "'------------------------------------------------------------------\n" );
				fflush( fsub );
			}
			break;
		}
}

// header - add machine setup to output file.
//
// Move and jog speeds could be an option...
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
	fprintf( fout, "SA								' absolute addressing\n" );
	fprintf( fout, "TR,12000,1\n" );
	fprintf( fout, "MS,%.3f,%.3f					' move speed: cut,plunge\n", moveSpeed, plungeSpeed );
	fprintf( fout, "JS,%.3f,%.3f					' jog speeds\n", jogSpeed, jogPlungeSpeed );
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
	switch( method ) {
		default :	// error
			fprintf( stderr, "Invalid method (%d) in footer\n", method );
			exit(1);
			break;
		case METHOD_ONE :
			sub1();									// sub1 one is an easy CR cut
			break;
		case METHOD_TWO :
			sub2();
			break;
	}
}

// evenCut - compute cut passes and cutDepth per pass
// inputs: 
//			cutD - pointer to best cut depth.
//			step - number of cut passes to be used.
// outputs:
//			cutD - best cut depth per pass.
//			passes to be used.
//

float
evenCuts( float * cutD, float step ) {
	float cut = * cutD;
	float passes = round( cut / step );

	*cutD = thickness / passes;

	return passes;
}

//
// sub1 - create subroutine to perform the actual cutting via the built-in CR command.
// for best accuracy the cut should move from right to left through the work piece. (jig specific ?)
// However, the easy way to do this is via the "cut rectangle" which does not allow for that control.
//
// Notes
//		Using CR (cut rectangle) and preset variables.
//		.25 is a deep cut...
void
sub1() {

	float	theCut = thickness;
	float	passes = evenCuts( &theCut, (diameter / MAX_DEPTH_RATIO) );

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
	fprintf( fout, "'-- Use the built-in Cut Rectangle to remove material           ---\n" );
	fprintf( fout, "'-- Parameters are pre-set before calling sub1.                 ---\n" );
	fprintf( fout, "'------------------------------------------------------------------\n" );
	fprintf( fout, "'\nsub1:'\n" );
	fprintf( fout, "SA									' absolute addressing\n" );
	fprintf( fout, "JZ,0.950000							' raise tool\n" );
	fprintf( fout, "J2,0.000000,0.000000				' home tool at start of cut\n" );
	fprintf( fout, "J3,&startX,&startY,0.000000			' position tool for CR cut\n" );
	fprintf( fout, "CR,&lenX,&lenY,I,1,4,%s%.3f,%.3f,2,1		' cut rectangle built-in\n", 
			 ( theCut < 0.0 ? "" : "-" ), theCut, passes );
	fprintf( fout, "JZ,0.950000							' raise tool\n" );
	fprintf( fout, "J2,0.000000,0.000000				' home tool at start of cut\n" );
	fprintf( fout, "'\n\tRETURN'\n'\n" );
	fprintf( fout, "'\n'\n" );
	fprintf( fout, "'------------------------------------------------------------------\n" );
	fprintf( fout, "'-- Use low level moves to remove material                      ---\n" );
	fprintf( fout, "'------------------------------------------------------------------\n" );

	fprintf( fout, "'------------------------------------------------------------------\n" );
	fprintf( fout, "'------------------------------------------------------------------\n" );

}

// append subroutine sub2 into the output file
void
sub2() {

	subBuffer = malloc( MAX_SUB_BUFFER );
	if( subBuffer == NULL ) {
		 fprintf( stderr, "No memory for the subroutine buffer.\n" );
		exit( -3 );
	} else
		subSize = MAX_SUB_BUFFER;

#if 0
	strcpy( subfname, fn );
	strcat( fname, ".sub" );
	// open input file
	fsub = fopen( subfname, "w" );
	if( fsub == NULL ) {
		fprintf( stderr, "Could not open subroutine file: %s\n", subfname );
		exit( -1 );
	}
#endif

	long rc = ftell( fsub );
	rc = fseek( fsub, 0L, SEEK_SET );
	if( rc != 0L ) {
		fprintf( stderr, "Could not reposition subroutine file position....\nAborting!" );
		errorExit( 3 );
	}

	subUsed = fread( subBuffer, 1, subSize, fsub );
	if( subUsed != 0 ) {
		size_t fcnt = fwrite( subBuffer, 1, subUsed, fout );
		if( fcnt != subUsed ) {
			fprintf( stderr, "Could not copy subroutine file ....\nAborting!" );
			errorExit( 4 );
		}
	}
	free( subBuffer );
}

// error Exit - we're here because of an error. don't leave any bad files....
void
errorExit( int e ) {
	if( fout != NULL ) {
		fclose(fout );
	}
	if( fsub != NULL ) {
		fclose(fsub );
	}
}

#if 0
void
toolPath( char * tool, int id ) {

	fprintf( fout, "'Toolpath Name = Profile %d\n", id );
	fprintf( fout, "'Tool Name   = %s\n", tool );
	fprintf( fout, "'----------------------------------------------------------------\n" );

}
#endif

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
#define	BUFLEN	100
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

	if( wlen == 0.0 ) {
		rc = fail( rc, "No edge length specified, use -l option" );
	} else if( wlen < 0.0 ) {
		rc = fail( rc, "Negative length edges are not supported" );
	} else if( wlen > MAX_EDGE_LENGTH ) {
		memset( buf, 0, BUFLEN );
		snprintf( buf,  BUFLEN, "Edges longer than %.3f inches are not supported", MAX_EDGE_LENGTH );
		rc = fail( rc, buf );
	}

	if( joints == 0 ) {
		rc = fail( rc, "Number of joint segments need to be specified, use -j option" );
	} else if( joints < 0 ) {
		rc = fail( rc, "Negative number of joint segments are not possible" );
	} else if( !(joints & 1)  ) {
		rc = fail( rc, "Should use a odd number of joint segments" );
	} 
	
	jointLen = wlen / joints;

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

	if( method == 0 ) {
		rc = fail( rc, "No cut method specified, use -m option" );
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
	} else if( cutDepth > 0.0 ) {
		rc = fail( rc, "Positive tool cutDepth are not effective" );
	} else if( diaOK == 1 ) {
		if( cutDepth >= (diameter / MAX_DEPTH_RATIO) ) {
			memset( buf, 0, BUFLEN );
			snprintf( buf,  BUFLEN, "Tool cut depths greater than the tool diameter / %.3f are not reccommended", MAX_DEPTH_RATIO );
			rc = fail( rc, buf );
		}	
	} else {
		rc = fail( rc, "No tool cut depth verification due to diameter parameter problems" );
	}

	if( diaOK == 1 ) { // avoid double error messages on one factor
		if( diameter >= jointLen ) {
			rc = fail( rc, "The tool diameter specified is too large for the joint" );
		}
	} else {
		rc = fail( rc, "No tool diameter depth, joint segment length  verification due to diameter parameter problems" );
	}

	if( diaOK == 1 ) { // avoid double error messages on one factor
		if( cutWidth == 0.0 ) {		// usr did not specify a cutWidth
			cutWidth = diameter * MAX_CUT_WIDTH;	//  assign one
		} else if( cutWidth >= MAX_CUT_WIDTH ) {	// usr did not specify a cutWidth
			rc = fail( rc, "Tool cut width is excessive!" );
		}
		if( cutWidth >= diameter ) {
			rc = fail( rc, "Cut width (-w) must be less than the tool diameter (-d)" );
		}
		
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
	
	minimumSegment = ( diameter + (2 * cutWidth));
	switch( method ) {
		default :	// unknown cut method
			rc = fail( rc, "Work piece thickness need to be specified, use -t option" );
			break;
		case METHOD_ONE :
			break;
		case METHOD_TWO :
			if( jointLen < minimumSegment  ) {
				memset( buf, 0, BUFLEN );
				snprintf( buf, BUFLEN, "The joint length (%.3f < %.3f) is too short based on the cut width(-w) and tool diameter(-d)", jointLen, minimumSegment );
				rc = fail( rc, buf );
			}
			break;
	}

	return rc;
}

int
finalCheck() {
#define	BUFLEN	100
	int	rc = 1;
	char	buf[ BUFLEN ];

// needs review...
	if(  (X1 <= 0.0) || (X2 <= 0.0) ) {
		rc = fail( rc, "There is a basic problem with the horizontal math of the joint, review the summary and try again." );
	}

	if(  (Y1 <= 0.0) || (Y2 >= 0.0) || (Y3 >= 0.0) || (Y4 <= 0.0) ) {
		rc = fail( rc, "There is a basic problem with the vertical math of the joint, review the summary and try again." );
	}

	if(  (X1 >= X2) ) {
		rc = fail( rc, "There is a basic problem with the horizontal points of the joint, review the summary and try again." );
	}

	if(  (Y1a >= Y1) || (Y1 >= Y3) || (Y1 >= Y4)  ) {
		rc = fail( rc, "There is a basic problem with the math of the joint, review the summary and try again." );
	}


	if( theCut >= halfDiameter ) {
		memset( buf, 0, BUFLEN );
		snprintf( buf, BUFLEN, "The cut width(%.3f) and the tool diameter(%.3f) aspects are not correct for this joint.\n", theCut, diameter );
		rc = fail( rc, buf );
		fprintf( stderr, "Change either -c or -d options, or both, to correct the problem.\n" );
	}

}

