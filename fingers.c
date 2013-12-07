
//
// fingers - shopbot program generator for cutting finger joints
//
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

#define	TITLE_MAX 		50
#define	SIDE_A			0
#define	SIDE_B			1

void usage( void );
void summary( void );
void generate( int );
void header( char * );
void footer( void );
void toolPath( char * , int  );
int validate( void );
void seg( int, int );
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
//	-t	title

char  fn[ PATH_MAX ];
char  * title = "None";
int		side = SIDE_A;
float	wlen = 6.5;
float	wt = .75;
float	diameter = 0.0;
float	joints = 5.0;
float	jointLen = 0.0;

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

	pgm = argv[ 0 ];
	while((ch = getopt(argc,argv,"d:j:l:o:n:s:v")) != -1) {
		switch( ch ) {
      default:
        fprintf(stderr, "%s: invalid option -%c\n\n", pgm, ch);
        usage();
        exit(1);
        break;

      case 'd': // tool diameter
		diameter = atof( optarg );
        break;

      case 'l': // workpiece length
		wlen = atof( optarg );
        break;

      case 'n': // job title
          title = argv[ optind - 1 ];
        break;

      case 's': // edge side A/B
          s = argv[ optind -1 ];
		  si = *s;
		  switch( si ) {
		  	default : fprintf( stderr, "Invalid side selection: %c\n Use A/B\n", si );
			opterr = 1;
			return -3;
			break;
		  case 'A' :
		  case 'a' :
		  	side = 0;
			break;
		  case 'B' :
		  case 'b' :
		  	side = 0;
			break;
		  }
        break;

      case 't': // workpiece thickbess
		wt = atof( optarg );
        break;

      case 'j': // number of joints
		joints = atof( optarg );
        break;

      case 'v': // verbose
		verbose |= VERBOSE;
        break;

      case 'o': // output filename
// should delay file open until we know which side.....
          strncpy( fn, optarg, PATH_MAX);
		  fout = fopen( fn, "w" );
		  if( fout == NULL ) {
		  	fprintf( stderr, "Could not open %s\n", fn );
			exit( -1 );
		  }
        break;
		}
	}

	jointLen = wlen / joints;

	if( ! validate() ) {
		  	fprintf( stderr, "Validation error!!\n", fn );
			exit( -2 );
	}
	summary();

	header( title );

	generate( side );

	footer();

	fclose( fout );
	return rc;
}

void
usage() {
	fprintf( stderr, "%s - usage....\n", pgm );
}

void
summary() {
	fprintf( fout, "'\n'%s\n", pgm );
	fprintf( fout, "'verbose\t%d\n", verbose );
	fprintf( fout, "'filename\t%s\n", fn );
	fprintf( fout, "'workpiece length\t%f\n", wlen );
	fprintf( fout, "'workpiece thickness\t%f\n", wt );
	fprintf( fout, "'joints \t%f\n", joints );
	fprintf( fout, "'jointLen \t%f\n", jointLen );
	fprintf( fout, "'tool diameter \t%f\n", diameter );
	fprintf( fout, "\n'Assumes workpiece is homed with no offset....\n'\n" );
}

//
// generate all segments of the edge
void
generate( int si ) {
	int	x = 0;

	fprintf( fout, "'----------------------------------------------------------------\n" );
	fprintf( fout, "'Turning router ON\n" );
	fprintf( fout, "SO,1,1\n" );
	fprintf( fout, "PAUSE 2\n" );
	fprintf( fout, "'----------------------------------------------------------------\n" );
	fprintf( fout, "'\n" );

	for( x=1; x < joints; x++ ) {
		seg( si, x );
	}

}

// seg - cut out segments....
// A edges have odd numbered segments removed.
// B edges have even numbered segments removed.
void
seg( int sideSelect, int id ) {

	if( (sideSelect == SIDE_A) && !(id & 1) ) {
		return;
	} else if( (sideSelect == SIDE_B) && (id & 1) ) {
		return;
	}
	toolPath( tools[ toolSelect ].name, id );
	// compute parameters and add a subroutine call to cut out the slot
	// put parameters into ShopBot variables

	float baseX = 0.0;
	float baseY = 0.0;
	float step = diameter / 2.0;

	fprintf( fout, "&startx = %f	' left edge X\n", baseX - step );
	fprintf( fout, "&starty = %f	' left edge X\n", baseY + step );	// phoney numbers...
	fprintf( fout, "\tCALL	sub1	' left edge\n" );

	baseX = 1.0;
	baseY = 1.0;
	fprintf( fout, "&startx = %f	' left edge X\n", baseX - step );
	fprintf( fout, "&starty = %f	' left edge X\n", baseY + step );	// phoney numbers...
#if 0
	fprintf( fout, "\tCALL	sub2	' right edge\n" );
	fprintf( fout, "&startx = %f	' left edge X\n", -5.0 );
	fprintf( fout, "&starty = %f	' left edge X\n", 1.0 );	// phoney numbers...
	fprintf( fout, "\tCALL	sub3	' bottom edge\n" );
	fprintf( fout, "&startx = %f	' left edge X\n", -5.0 );
	fprintf( fout, "&starty = %f	' left edge X\n", 1.0 );	// phoney numbers...
	fprintf( fout, "\tCALL	sub4	' top edge\n" );
	fprintf( fout, "&startx = %f	' left edge X\n", -5.0 );
	fprintf( fout, "&starty = %f	' left edge X\n", 1.0 );	// phoney numbers...
	fprintf( fout, "\tCALL	sub5	' center\n" );
#endif

	fprintf( fout, "'----------------------------------------------------------------\n" );
}

void
header( char * t ) {

	fprintf( fout, "'%s\n", t );
	fprintf( fout, "'File created: blah blah\n" );
	fprintf( fout, "'By Bill\n" );
	fprintf( fout, "'\n" );
	fprintf( fout, "'SHOPBOT FILE IN INCHES\n" );
	fprintf( fout, "' 25 is UNIT, 0 or 1. asuming 0 means inches...\n" );
	fprintf( fout, "IF %(25)=1 THEN GOTO UNIT_ERROR	'check to see software is set to standard\n" );
	fprintf( fout, "C#,90				 	'Lookup offset values\n" );
	fprintf( fout, "'\n" );
	fprintf( fout, "TR,12000,1\n" );
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

void
subs() {

	fprintf( fout, "'\n'\n" );
	fprintf( fout, "'------------------------ subroutines -----------------------------\n" );
	fprintf( fout, "'\n'\n'\tsub1 - cut left vertical edge of slot'\n" );
	fprintf( fout, "'\nsub1:'\n" );
	fprintf( fout, "MS,0.08,0.05				' move speed: cut,plunge\n" );
	fprintf( fout, "JZ,0.950000					' raise tool\n" );
	fprintf( fout, "J2,0.000000,0.000000		' home tool\n" );
	fprintf( fout, "J3,&startx,&starty,0.200000		' position tool for cut\n" );
	fprintf( fout, "' delay to simulate cut\n" );
	fprintf( fout, "\tpause 2	' small delay\n" );


#if 0

	fprintf( fout, "M3,0.200000,4.650000,-0.187500\n" );
	fprintf( fout, "M3,1.520000,4.650000,-0.187500\n" );
	fprintf( fout, "J3,1.520000,4.650000,0.200000\n" );
	fprintf( fout, "J3,0.200000,4.650000,0.200000\n" );
	fprintf( fout, "M3,0.200000,4.650000,-0.375000\n" );
	fprintf( fout, "M3,1.520000,4.650000,-0.375000\n" );
	fprintf( fout, "J3,1.520000,4.650000,0.200000\n" );
	fprintf( fout, "J3,0.200000,4.650000,0.200000\n" );
	fprintf( fout, "M3,0.200000,4.650000,-0.562500\n" );
	fprintf( fout, "M3,1.520000,4.650000,-0.562500\n" );
	fprintf( fout, "J3,1.520000,4.650000,0.200000\n" );
	fprintf( fout, "J3,0.200000,4.650000,0.200000\n" );
	fprintf( fout, "M3,0.200000,4.650000,-0.750000\n" );
	fprintf( fout, "M3,1.520000,4.650000,-0.750000\n" );
	fprintf( fout, "J3,1.520000,4.650000,0.200000\n" );
#endif

	fprintf( fout, "'\n\tRETURN'\n'\n" );
	fprintf( fout, "'\n'\n'\tsub2 - cut right vertical edge of slot'\n" );
	fprintf( fout, "MS,0.08,0.05				' move speed: cut,plunge\n" );
	fprintf( fout, "JZ,0.950000					' raise tool\n" );
	fprintf( fout, "J2,0.000000,0.000000		' home tool\n" );
	fprintf( fout, "J3,&startx,&starty,0.200000		' position tool for cut\n" );
	fprintf( fout, "' delay to simulate cut\n" );
	fprintf( fout, "\tpause 2	' small delay\n" );
	fprintf( fout, "'\n\tRETURN'\n'\n" );
	fprintf( fout, "'\n'\n'\tsub3 - cut bottom edge of slot'\n" );
	fprintf( fout, "MS,0.08,0.05				' move speed: cut,plunge\n" );
	fprintf( fout, "JZ,0.950000					' raise tool\n" );
	fprintf( fout, "J2,0.000000,0.000000		' home tool\n" );
	fprintf( fout, "J3,&startx,&starty,0.200000		' position tool for cut\n" );
	fprintf( fout, "' delay to simulate cut\n" );
	fprintf( fout, "\tpause 2	' small delay\n" );
	fprintf( fout, "'\n\tRETURN'\n'\n" );
	fprintf( fout, "'\n'\n'\tsub4 - cut top edge of slot'\n" );
	fprintf( fout, "MS,0.08,0.05				' move speed: cut,plunge\n" );
	fprintf( fout, "JZ,0.950000					' raise tool\n" );
	fprintf( fout, "J2,0.000000,0.000000		' home tool\n" );
	fprintf( fout, "J3,&startx,&starty,0.200000		' position tool for cut\n" );
	fprintf( fout, "' delay to simulate cut\n" );
	fprintf( fout, "\tpause 2	' small delay\n" );
	fprintf( fout, "'\n\tRETURN'\n'\n" );
	fprintf( fout, "'\n'\n'\tsub5 - cut center of slot'\n" );
	fprintf( fout, "MS,0.08,0.05				' move speed: cut,plunge\n" );
	fprintf( fout, "JZ,0.950000					' raise tool\n" );
	fprintf( fout, "J2,0.000000,0.000000		' home tool\n" );
	fprintf( fout, "J3,&startx,&starty,0.200000		' position tool for cut\n" );
	fprintf( fout, "' delay to simulate cut\n" );
	fprintf( fout, "\tpause 2	' small delay\n" );
	fprintf( fout, "'\n\tRETURN'\n'\n" );
}

void
toolPath( char * tool, int id ) {

	fprintf( fout, "'Toolpath Name = Profile %d\n", id );
	fprintf( fout, "'Tool Name   = %s\n", tool );
	fprintf( fout, "'----------------------------------------------------------------\n" );

}

//
// validate job setup
//
// returns true if job looks OK.

int
validate() {
	int	rc = 1;

	return rc;
}

