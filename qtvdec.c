#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "utils.h"
#include "image.h"
#include "qti.h"
#include "qtc.h"
#include "qtv.h"
#include "ppm.h"

int interrupt;

void sig_exit( int sig )
{
	if( ( sig == SIGINT ) || ( sig == SIGTERM ) )
		interrupt = 1;
}

int main( int argc, char *argv[] )
{
	struct image image, refimage;
	struct qti compimage;
	struct qtv video;

	int opt, analyze, qtw;
	int done, keyframe, framenum;
	int startframe, numframes;
	char *infile, *outfile;

	analyze = 0;
	startframe = 0;
	numframes = -1;
	qtw = 0;
	infile = NULL;
	outfile = NULL;

	while( ( opt = getopt( argc, argv, "a:wf:n:i:o:" ) ) != -1 )
	{
		switch( opt )
		{
			case 'w':
				qtw = 1;
			break;

			case 'a':
				if( sscanf( optarg, "%i", &analyze ) != 1 )
					fputs( "main: Can not parse command line: -a\n", stderr );
			break;

			case 'f':
				if( sscanf( optarg, "%i", &startframe ) != 1 )
					fputs( "main: Can not parse command line: -f\n", stderr );
			break;

			case 'n':
				if( sscanf( optarg, "%i", &numframes ) != 1 )
					fputs( "main: Can not parse command line: -n\n", stderr );
			break;

			case 'i':
				infile = strdup( optarg );
			break;

			case 'o':
				outfile = strdup( optarg );
			break;

			default:
			case '?':
				fputs( "main: Can not parse command line: unknown option\n", stderr );
				return 1;
			break;
		}
	}

	if( ( analyze < 0 ) || ( analyze > 2 ) )
	{
		fputs( "main: Analysis mode out of range\n", stderr );
		return 1;
	}

	if( startframe < 0 )
	{
		fputs( "main: Start frame out of range\n", stderr );
		return 1;
	}

	if( numframes < -1 )
	{
		fputs( "main: Number of frames out of range\n", stderr );
		return 1;
	}

	interrupt = 0;
	
	signal( SIGINT, sig_exit );
	signal( SIGTERM, sig_exit );

	done = 0;
	framenum = 0;

	if( ! qtv_read_header( &video, qtw, infile ) )
		return 0;

	if( startframe != 0 )
		qtv_seek( &video, startframe );

	image_create( &refimage, video.width, video.height );

	do
	{
		if( ! qtv_read_frame( &video, &compimage, &keyframe ) )
			return 0;

		if( analyze == 0 )
		{
			if( keyframe )
			{
				if( ! qtc_decompress( &compimage, NULL, &image, 0, compimage.colordiff >= 2 ) )
					return 0;
			}
			else
			{
				if( ! qtc_decompress( &compimage, &refimage, &image, 0, compimage.colordiff >= 2 ) )
					return 0;
			}

			image_copy( &image, &refimage );

			if( compimage.transform == 1 )
				image_transform_fast_rev( &image );
			else if( compimage.transform == 2 )
				image_transform_rev( &image );

			if( compimage.colordiff >= 1 )
				image_color_diff_rev( &image );

		}
		else
		{
			if( ! qtc_decompress_ccode( &compimage, &image, !keyframe, 0, compimage.colordiff >= 2, analyze-1 ) )
				return 0;	
		}

		if( ! ppm_write( &image, outfile ) )
			return 0;

		image_free( &image );
		qti_free( &compimage );

		if( ( outfile != NULL ) && ( strcmp( outfile, "-" ) != 0 ) )
		{
			if( !inc_filename( outfile ) )
				done = 1;
		}

		if( interrupt )
			done = 1;

		if( ! qtv_can_read_frame( &video ) )
			done = 1;

		framenum++;
	}
	while( ( ! done ) && ( framenum != numframes ) );

	image_free( &refimage );
	qtv_free( &video );

	free( infile );
	free( outfile );

	return 0;
}

