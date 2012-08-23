/*
*    QTC: qtvenc.c (c) 2011, 2012 50m30n3
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
#include "tilecache.h"

/*******************************************************************************
* This is the reference qtv encoder.                                           *
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
	puts( "qtvenc (c) 50m30n3 2011, 2012" );
	puts( "USAGE: qtvenc [options] -i infile -o outfile" );
	puts( "\t-h\t\t-\tPrint help" );
	puts( "\t-t [0..2]\t-\tUse image transforms (0)" );
	puts( "\t-e\t\t-\tCompress output data" );
	puts( "\t-w\t\t-\tCreate QTW file" );
	puts( "\t-y [0..2]\t-\tUse fakeyuv transform (0)" );
	puts( "\t-v\t\t-\tBe verbose" );
	puts( "\t-x\t\t-\tCreate index (Needs key frames)" );
	puts( "\t-s [1..]\t-\tMinimal block size (2)" );
	puts( "\t-n [1..]\t-\tLimit number of frames to encode" );
	puts( "\t-r [1..]\t-\tFrame rate (25)" );
	puts( "\t-k [1..]\t-\tPlace key frames every X seconds" );
	puts( "\t-b [1..]\t-\tMaximum size of one QTW block in KiB (1024)" );
	puts( "\t-d [0..]\t-\tMaximum recursion depth (16)" );
	puts( "\t-c [0..]\t-\tCache size in kilo tiles (0)" );
	puts( "\t-l [0..]\t-\tLaziness" );
	puts( "\t-i filename\t-\tInput file (-)" );
	puts( "\t-o filename\t-\tOutput file (-)" );
}

int main( int argc, char *argv[] )
{
	struct image image, refimage;
	struct qti compimage;
	struct qtv video;
	struct tilecache *cache;

	int opt, verbose, qtw;
	unsigned long int insize, bsize, outsize, size;
	unsigned long int cacheblocks, cachehits;
	int done, tmp, keyframe, framenum;
	int transform, colordiff;
	int rangecomp, compress;
	int minsize;
	int maxdepth;
	int lazyness;
	int cachesize;
	int index;
	int framerate, keyrate, numframes;
	int blockrate, numblocks, blocksize;
	long int start, frame_start;
	double fps;
	char *infile, *outfile;

	verbose = 0;
	transform = 0;
	colordiff = 0;
	rangecomp = 0;
	minsize = 2;
	maxdepth = 16;
	cachesize = 0;
	lazyness = 0;
	framerate = 25;
	keyrate = 0;
	index = 0;
	numframes = -1;
	blockrate = 1024;
	qtw = 0;
	infile = NULL;
	outfile = NULL;

	while( ( opt = getopt( argc, argv, "hevxwy:n:t:s:d:c:l:r:k:b:i:o:" ) ) != -1 )
	{
		switch( opt )
		{
			case 'h':
				print_help();
				return 0;
			break;

			case 't':
				if( sscanf( optarg, "%i", &transform ) != 1 )
					fputs( "main: Can not parse command line: -t\n", stderr );
			break;

			case 'e':
				rangecomp = 1;
			break;

			case 'w':
				qtw = 1;
			break;

			case 'y':
				if( sscanf( optarg, "%i", &colordiff ) != 1 )
					fputs( "main: Can not parse command line: -y\n", stderr );
			break;

			case 'v':
				verbose = 1;
			break;

			case 'x':
				index = 1;
			break;

			case 's':
				if( sscanf( optarg, "%i", &minsize ) != 1 )
					fputs( "main: Can not parse command line: -s\n", stderr );
			break;

			case 'n':
				if( sscanf( optarg, "%i", &numframes ) != 1 )
					fputs( "main: Can not parse command line: -n\n", stderr );
			break;

			case 'r':
				if( sscanf( optarg, "%i", &framerate ) != 1 )
					fputs( "main: Can not parse command line: -r\n", stderr );
			break;

			case 'k':
				if( sscanf( optarg, "%i", &keyrate ) != 1 )
					fputs( "main: Can not parse command line: -k\n", stderr );
			break;

			case 'b':
				if( sscanf( optarg, "%i", &blockrate ) != 1 )
					fputs( "main: Can not parse command line: -b\n", stderr );
			break;

			case 'd':
				if( sscanf( optarg, "%i", &maxdepth ) != 1 )
					fputs( "main: Can not parse command line: -d\n", stderr );
			break;

			case 'c':
				if( sscanf( optarg, "%i", &cachesize ) != 1 )
					fputs( "main: Can not parse command line: -s\n", stderr );
			break;

			case 'l':
				if( sscanf( optarg, "%i", &lazyness ) != 1 )
					fputs( "main: Can not parse command line: -l\n", stderr );
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

	if( ( transform < 0 ) || ( transform > 2 ) )
	{
		fputs( "main: Transform mode out of range\n", stderr );
		return 1;
	}

	if( ( colordiff < 0 ) || ( colordiff > 2 ) )
	{
		fputs( "main: Fakeyuv mode out of range\n", stderr );
		return 1;
	}

	if( minsize < 0 )
	{
		fputs( "main: Mininmal block size out of range\n", stderr );
		return 1;
	}

	if( maxdepth < 0 )
	{
		fputs( "main: Maximum recursion depth out of range\n", stderr );
		return 1;
	}

	if( cachesize < 0 )
	{
		fputs( "main: Cache size out of range\n", stderr );
		return 1;
	}

	if( lazyness < 0 )
	{
		fputs( "main: Lazyness recursion depth out of range\n", stderr );
		return 1;
	}

	if( numframes < -1 )
	{
		fputs( "main: Number of frames out of range\n", stderr );
		return 1;
	}

	if( framerate < 0 )
	{
		fputs( "main: Frame rate out of range\n", stderr );
		return 1;
	}

	if( keyrate < 0 )
	{
		fputs( "main: Keyframe rate out of range\n", stderr );
		return 1;
	}

	if( blockrate < 0 )
	{
		fputs( "main: Maximum block size out of range\n", stderr );
		return 1;
	}

	interrupt = 0;

	done = 0;
	framenum = 0;
	numblocks = 0;
	
	blocksize = 0;

	insize = 0;
	bsize = 0;
	outsize = 0;

	if( cachesize > 0 )
		cache = tilecache_create( cachesize*1024, minsize );
	else
		cache = NULL;

	fps = 0;
	start = get_time();

	do
	{
		frame_start = get_time();

		if( keyrate == 0 )		// Check if we need a keyframe
			keyframe = framenum == 0;
		else
			keyframe = framenum % ( keyrate * framerate ) == 0;

		if( ( qtw ) && ( blocksize >= blockrate*1024 ) )		// Check if we need to start a new qtw block
		{
			if( ! qtv_write_block( &video ) )		// Start new block
				return 2;

			numblocks++;
			blocksize = 0;
		}

		if( ! ppm_read( &image, infile ) )		// Read input frame
			return 2;

		if( framenum == 0 )		// Initialize video stream after reading the first frame
		{
			signal( SIGINT, sig_exit );
			signal( SIGTERM, sig_exit );

			if( ! qtv_create( &video, image.width, image.height, framerate, cache, index, qtw ) )		// Initialize video
				return 2;

			if( ! qtv_write_header( &video, outfile ) )		// Write video header to file
				return 2;

			if( ! image_create( &refimage, image.width, image.height, 0 ) )		// Create reference image
				return 2;
		}

		if( ( image.width != video.width ) || ( image.height != video.height ) )
		{
			fprintf( stderr, "main: %s: Image size does not match video size.\n", infile );
			return 2;
		}

		insize += ( image.width * image.height * 3 );

		if( colordiff >= 1 )		// Apply fakeyuv transform
			image_color_diff( &image );

		if( transform == 1 )		// Apply image transforms
			image_transform_fast( &image );
		else if( transform == 2 )
			image_transform( &image );

		if( ! qti_create( &compimage, image.width, image.height, minsize, maxdepth, cache ) )
			return 2;

		if( keyframe )		// Compress frame
		{
			if( ! qtc_compress( &image, NULL, &compimage, lazyness, colordiff == 2 ) )
				return 2;
		}
		else
		{
			if( ! qtc_compress( &image, &refimage, &compimage, lazyness, colordiff == 2 ) )
				return 2;
		}

		bsize += qti_getsize( &compimage );

		if( qti_getsize( &compimage ) <= 4 )		// Apply entropy coding only to big frames 
			compress = 0;
		else
			compress = rangecomp;

		if( ! ( size = qtv_write_frame( &video, &compimage, compress ) ) )		// Write compressed frame to video stream
			return 2;

		outsize += size;
		blocksize += size;

		image_copy( &image, &refimage );		// Copy current frame to reference image

		image_free( &image );
		qti_free( &compimage );
		
		if( ( infile == NULL ) || ( strcmp( infile, "-" ) == 0 ) )
		{
			tmp = getc( stdin );
			if( feof( stdin ) )
				done = 1;
			else
				ungetc( tmp, stdin );
		}
		else
		{
			if( !inc_filename( infile ) )
				done = 1;
			
			if( access( infile, R_OK ) == -1 )
				done = 1;
		}

		if( interrupt )
			done = 1;

		if( verbose )
		{
			if( cache != NULL )
			{
				cacheblocks = cache->numblocks;
				cachehits = cache->hits;
			}
			else
			{
				cacheblocks = 0;
				cachehits = 0;
			}
	
			if( qtw )
			{
				fprintf( stderr, "Frame:%i Block:%i FPS:%.2f In:%lukb/s Buff:%lukb/s,%f%% Cache:%lu/%lu,%f%% Out:%lukb/s,%f%% Curr:%lukb/s\n", framenum, numblocks, fps,
				         (insize*8)/(framenum+1)*framerate/1000,
				         bsize/(framenum+1)*framerate/1000, bsize*100.0/(insize*8),
				         cachehits, cacheblocks, cachehits*100.0/cacheblocks,
				         (outsize*8)/(framenum+1)*framerate/1000, outsize*100.0/insize,
				         (size*8)*framerate/1000 );
			}
			else
			{
				fprintf( stderr, "Frame:%i FPS:%.2f In:%lukb/s Buff:%lukb/s,%f%% Cache:%lu/%lu,%f%% Out:%lukb/s,%f%% Curr:%lukb/s\n", framenum, fps,
				         (insize*8)/(framenum+1)*framerate/1000,
				         bsize/(framenum+1)*framerate/1000, bsize*100.0/(insize*8),
				         cachehits, cacheblocks, cachehits*100.0/cacheblocks,
				         (outsize*8)/(framenum+1)*framerate/1000, outsize*100.0/insize,
				         (size*8)*framerate/1000 );
			}
		}
		
		framenum++;
		
		fps = fps*0.75 + 0.25*(1000000.0/(get_time()-frame_start));
	}
	while( ( ! done ) && ( framenum != numframes ) );

	fps = 1000000.0/((get_time()-start)/framenum);

	if( index )		// Write video index to file
		outsize += qtv_write_index( &video );

	image_free( &refimage );
	qtv_free( &video );

	if( cache != NULL )
	{
		cacheblocks = cache->numblocks;
		cachehits = cache->hits;
		tilecache_free( cache );
	}
	else
	{
		cacheblocks = 0;
		cachehits = 0;
	}

	if( verbose )
	{
		fprintf( stderr, "In:%lumiB Buff:%lumiB,%f%% Cache:%lu/%lu,%f%% Out:%lumiB,%f%% FPS:%.2f\n",
		         insize/1024/1024,
		         bsize/8/1024/1024, (bsize/8)*100.0/insize,
		         cachehits, cacheblocks, cachehits*100.0/cacheblocks,
		         outsize/1024/1024, outsize*100.0/insize,
		         fps );
	}

	free( infile );
	free( outfile );

	return 0;
}

