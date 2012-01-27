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

	int opt, verbose, qtw;
	unsigned long int insize, bsize, outsize, size;
	int done, tmp, keyframe, framenum;
	int transform, colordiff;
	int rangecomp, compress;
	int minsize;
	int maxdepth;
	int lazyness;
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
	lazyness = 0;
	framerate = 25;
	keyrate = 0;
	index = 0;
	numframes = -1;
	blockrate = 1024;
	qtw = 0;
	infile = NULL;
	outfile = NULL;

	while( ( opt = getopt( argc, argv, "cvxwy:n:t:s:d:l:r:k:b:i:o:" ) ) != -1 )
	{
		switch( opt )
		{
			case 't':
				if( sscanf( optarg, "%i", &transform ) != 1 )
					fputs( "main: Can not parse command line: -t\n", stderr );
			break;

			case 'c':
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
	
	signal( SIGINT, sig_exit );
	signal( SIGTERM, sig_exit );

	done = 0;
	framenum = 0;
	numblocks = 0;
	
	blocksize = 0;

	insize = 0;
	bsize = 0;
	outsize = 0;

	fps = 0;
	start = get_time();

	do
	{
		frame_start = get_time();

		if( keyrate == 0 )
			keyframe = framenum == 0;
		else
			keyframe = framenum % ( keyrate * framerate ) == 0;

		if( ( qtw ) && ( blocksize >= blockrate*1024 ) )
		{
			qtv_write_block( &video );
			numblocks++;
			blocksize = 0;
		}

		if( ! ppm_read( &image, infile ) )
			return 0;

		if( framenum == 0 )
		{
			qtv_create( image.width, image.height, framerate, index, qtw, &video );
			qtv_write_header( &video, outfile );
			
			image_create( &refimage, image.width, image.height );
		}

		insize += ( image.width * image.height * 3 );

		if( colordiff >= 1 )
			image_color_diff( &image );

		if( transform == 1 )
			image_transform_fast( &image );
		else if( transform == 2 )
			image_transform( &image );

		if( keyframe )
		{
			if( colordiff >= 2 )
			{
				if( ! qtc_compress_color_diff( &image, NULL, &compimage, minsize, maxdepth, lazyness ) )
					return 0;
			}
			else
			{
				if( ! qtc_compress( &image, NULL, &compimage, minsize, maxdepth, lazyness, 0, 0 ) )
					return 0;
			}
		}
		else
		{
			if( colordiff >= 2 )
			{
				if( ! qtc_compress_color_diff( &image, &refimage, &compimage, minsize, maxdepth, lazyness ) )
					return 0;
			}
			else
			{
				if( ! qtc_compress( &image, &refimage, &compimage, minsize, maxdepth, lazyness, 0, 0 ) )
					return 0;
			}
		}

		bsize += qti_getsize( &compimage );

		compimage.transform = transform;
		compimage.colordiff = colordiff;

		if( qti_getsize( &compimage ) <= 4 )
			compress = 0;
		else
			compress = rangecomp;

		if( ! ( size = qtv_write_frame( &video, &compimage, compress, keyframe ) ) )
			return 0;

		outsize += size;
		blocksize += size;

		image_copy( &image, &refimage );

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
			if( qtw )
			{
				fprintf( stderr, "Frame:%i Block:%i FPS:%.2f In:%lukb/s Buff:%lukb/s,%f%% Out:%lukb/s,%f%% Curr:%lukb/s\n", framenum, numblocks, fps,
					(insize*8)/(framenum+1)*framerate/1000,
					bsize/(framenum/framerate+1)/1000, bsize*100.0/(insize*8),
					(outsize*8)/(framenum+1)*framerate/1000, outsize*100.0/insize,
					(size*8)*framerate/1000 );
			}
			else
			{
				fprintf( stderr, "Frame:%i FPS:%.2f In:%lukb/s Buff:%lukb/s,%f%% Out:%lukb/s,%f%% Curr:%lukb/s\n", framenum, fps,
					(insize*8)/(framenum+1)*framerate/1000,
					bsize/(framenum/framerate+1)/1000, bsize*100.0/(insize*8),
					(outsize*8)/(framenum+1)*framerate/1000, outsize*100.0/insize,
					(size*8)*framerate/1000 );
			}
		}
		
		framenum++;
		
		fps = fps*0.75 + 0.25*(1000000.0/(get_time()-frame_start));
	}
	while( ( ! done ) && ( framenum != numframes ) );

	fps = 1000000.0/((get_time()-start)/framenum);

	if( index )
		outsize += qtv_write_index( &video );

	image_free( &refimage );
	qtv_free( &video );

	if( verbose )
	{
		fprintf( stderr, "In:%lumiB Buff:%lumiB,%f%% Out:%lumiB,%f%% FPS:%.2f\n",
			insize/1024/1024,
			bsize/8/1024/1024, (bsize/8)*100.0/insize,
			outsize/1024/1024, outsize*100.0/insize,
			fps );
	}

	free( infile );
	free( outfile );

	return 0;
}

