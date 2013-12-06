
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

void usage( void );
void summary( void );
void generate( void );
void header( char * );
void footer( void );
void toolPath( char * , int  );
int validate( void );
void seg( int id );

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
float	wlen = 6.5;
float	wt = .75;
float	joints = 5.0;
float	jointLen = 0.0;

int		verbose = 0;
#define	VERBOSE 	1
#define	PROLOGUE	2
#define	DEBUG 		4

char * pgm = NULL;

int
main( int argc, char * argv[] )
{
	int	rc = 0;
	int	ch;

	fout = stdout;

	pgm = argv[ 0 ];
	while((ch = getopt(argc,argv,"j:l:o:n:v")) != -1) {
		switch( ch ) {
      default:
        fprintf(stderr, "%s: invalid option -%c\n\n", pgm, ch);
        usage();
        exit(1);
        break;

      case 'd': // debug
		verbose |= DEBUG;
        break;

      case 'l': // workpiece length
		wlen = atof( optarg );
        break;

      case 'n': // job title
          title = argv[ optind ];
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

	generate();

	header( title );

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
	fprintf( stdout, "\n%s\n", pgm );
	fprintf( stdout, "verbose\t%d\n", verbose );
	fprintf( stdout, "filename\t%s\n", fn );
	fprintf( stdout, "workpiece length\t%f\n", wlen );
	fprintf( stdout, "workpiece thickness\t%f\n", wt );
	fprintf( stdout, "joints \t%f\n", joints );
	fprintf( stdout, "jointLen \t%f\n", jointLen );
}

//
// generate all segments of the edge
void
generate() {
	int	x = 0;

	fprintf( fout, "'----------------------------------------------------------------\n" );
	fprintf( fout, "'Turning router ON\n" );
	fprintf( fout, "SO,1,1\n" );
	fprintf( fout, "PAUSE 2\n" );
	fprintf( fout, "'----------------------------------------------------------------\n" );
	fprintf( fout, "'\n" );
	for( x=0; x < joints; x++ ) {
		seg( x );
	}

}

void
seg( int id ) {
	toolPath( tools[ toolSelect ].name, id );
	fprintf( fout, "'Tool Name   = End Mill (0.5 inch)\n" );
	fprintf( fout, "MS,0.08,0.05				' move speed: cut,plunge\n" );
	fprintf( fout, "JZ,0.950000\n" );
	fprintf( fout, "J2,0.000000,0.000000\n" );
	fprintf( fout, "J3,0.200000,4.650000,0.200000\n" );
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

