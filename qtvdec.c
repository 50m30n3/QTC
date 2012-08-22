/*
*    QTC: qtvdec.c (c) 2011, 2012 50m30n3
*
*    This file is part of QTC.
*
*    QTC is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    QTC is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with QTC.  If not, see <http://www.gnu.org/licenses/>.
*/

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

/*******************************************************************************
* This is the reference qtv decoder.                                           *
*                                                                              *
* It implements all the features currently supported by the qtv codec.         *
*******************************************************************************/

volatile int interrupt;

void sig_exit( int sig )
{
	if( ( sig == SIGINT ) || ( sig == SIGTERM ) )
		interrupt = 1;
}

void print_help( void )
{
	puts( "qtvdec (c) 50m30n3 2011, 2012" );
	puts( "USAGE: qtvdec [options] -i infile -o outfile" );
	puts( "\t-h\t\t-\tPrint help" );
	puts( "\t-w\t\t-\tRead QTW file" );
	puts( "\t-a [0..2]\t-\tAnalysis mode" );
	puts( "\t-f [1..]\t-\tBegin decoding at specific frame (Needs index)" );
	puts( "\t-n [1..]\t-\tLimit number of frames to decode" );
	puts( "\t-i filename\t-\tInput file (-)" );
	puts( "\t-o filename\t-\tOutput file (-)" );
}

int main( int argc, char *argv[] )
{
	struct image image, refimage;
	struct qti compimage;
	struct qtv video;

	int opt, analyze, qtw;
	int done, framenum, skipframes;
	int startframe, numframes;
	char *infile, *outfile;

	analyze = 0;
	startframe = 0;
	skipframes = 0;
	numframes = -1;
	qtw = 0;
	infile = NULL;
	outfile = NULL;

	while( ( opt = getopt( argc, argv, "ha:wf:n:i:o:" ) ) != -1 )
	{
		switch( opt )
		{
			case 'h':
				print_help();
				return 0;
			break;

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

	done = 0;
	framenum = 0;

	if( ! qtv_read_header( &video, qtw, infile ) )		// Read video header
		return 2;

	signal( SIGINT, sig_exit );
	signal( SIGTERM, sig_exit );

	if( startframe != 0 )
	{
		if( video.has_index )
			qtv_seek( &video, startframe );

		skipframes = startframe - video.framenum;
	}

	image_create( &refimage, video.width, video.height );		// Create reference image

	do
	{
		if( ! qtv_read_frame( &video, &compimage ) )		// Read frame from stream
			return 2;

		if( analyze == 0 )
		{
			if( ! qtc_decompress( &compimage, &refimage, &image, 0 ) )		// Decompress frame
				return 2;

			image_copy( &image, &refimage );		// Copy frame to reference image


			if( skipframes <= 0 )
			{
				if( image.transform == 1 )		// Apply image transforms
					image_transform_fast_rev( &image );
				else if( image.transform == 2 )
					image_transform_rev( &image );

				if( image.colordiff )		// Apply fakeyuv transform
					image_color_diff_rev( &image );
			}

		}
		else
		{
			if( skipframes <= 0 )
			{
				if( ! qtc_decompress_ccode( &compimage, &image, 0, analyze-1 ) )		// Create analysis image
					return 2;
			}
		}

		if( skipframes <= 0 )
		{
			if( ! ppm_write( &image, outfile ) )		// Write decompressed frame to file
				return 2;

			if( ( outfile != NULL ) && ( strcmp( outfile, "-" ) != 0 ) )
			{
				if( !inc_filename( outfile ) )
					done = 1;
			}
		}

		image_free( &image );
		qti_free( &compimage );

		if( interrupt )
			done = 1;

		if( ! qtv_can_read_frame( &video ) )
			done = 1;

		if( skipframes <= 0 )
			framenum++;
		else
			skipframes--;
	}
	while( ( ! done ) && ( framenum != numframes ) );

	image_free( &refimage );
	qtv_free( &video );

	free( infile );
	free( outfile );

	return 0;
}

