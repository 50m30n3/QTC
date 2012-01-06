#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include "image.h"
#include "x11grab.h"
#include "qti.h"
#include "qtc.h"
#include "qtv.h"

int interrupt;

void sig_exit( int sig )
{
	if( ( sig == SIGINT ) || ( sig == SIGTERM ) )
		interrupt = 1;
}

unsigned long int get_time( void )
{
	struct timeval tv;

	gettimeofday( &tv, NULL );

	return tv.tv_sec * 1000000lu + tv.tv_usec;
}

int inc_filename( char *name )
{
	int i, carry, done;
	
	carry = 1;
	done = 0;

	for( i=strlen( name )-1; i>=0; i-- )
	{
		if( ( name[i] >= '0' ) && ( name[i] <= '9' ) )
		{
			done = 1;
			if( carry )
			{
				if( name[i] < '9' )
				{
					name[i]++;
					carry = 0;
				}
				else
				{
					name[i] = '0';
					carry = 1;
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			if( done )
				break;
		}
	}
	
	return !carry;
}

int main( int argc, char *argv[] )
{
	struct image image, refimage;
	struct qti compimage;
	struct qtv video;
	struct x11grabber grabber;

	int opt, verbose, x, y, w, h, mouse;
	unsigned long int insize, bsize, outsize, size;
	int done, keyframe, framenum;
	int transform, colordiff;
	int rangecomp, compress;
	int minsize;
	int maxdepth;
	int lazyness;
	int index;
	int framerate, keyrate, numframes;
	long int delay, start;
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
	x = y = 0;
	w = h = -1;
	mouse = 0;
	infile = NULL;
	outfile = NULL;

	while( ( opt = getopt( argc, argv, "cvxmg:y:f:n:t:s:d:l:r:k:i:o:" ) ) != -1 )
	{
		switch( opt )
		{
			case 't':
				if( sscanf( optarg, "%i", &transform ) != 1 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 'c':
				rangecomp = 1;
			break;

			case 'y':
				if( sscanf( optarg, "%i", &colordiff ) != 1 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 'v':
				verbose = 1;
			break;

			case 'x':
				index = 1;
			break;

			case 'm':
				mouse = 1;
			break;

			case 'g':
				if( sscanf( optarg, "%ix%i+%i,%i", &w, &h, &x, &y ) != 4 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 's':
				if( sscanf( optarg, "%i", &minsize ) != 1 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 'n':
				if( sscanf( optarg, "%i", &numframes ) != 1 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 'r':
				if( sscanf( optarg, "%i", &framerate ) != 1 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 'k':
				if( sscanf( optarg, "%i", &keyrate ) != 1 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 'd':
				if( sscanf( optarg, "%i", &maxdepth ) != 1 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 'l':
				if( sscanf( optarg, "%i", &lazyness ) != 1 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 'i':
				infile = strdup( optarg );
			break;

			case 'o':
				outfile = strdup( optarg );
			break;

			default:
			case '?':
				fputs( "ERROR: Can not parse command line\n", stderr );
				return 1;
			break;
		}
	}

	interrupt = 0;
	
	signal( SIGINT, sig_exit );
	signal( SIGTERM, sig_exit );

	done = 0;
	framenum = 0;

	if( ! x11grabber_create( &grabber, infile, x, y, w, h, mouse ) )
		return 1;

	insize = 0;
	bsize = 0;
	outsize = 0;
	
	delay = 0;
	fps = 0;

	do
	{
		start = get_time();

		if( keyrate == 0 )
			keyframe = framenum == 0;
		else
			keyframe = framenum % ( keyrate * framerate ) == 0;

		if( ! x11grabber_grab_frame( &image, &grabber ) )
			return 1;

		if( framenum == 0 )
		{
			qtv_create( image.width, image.height, framerate, index, &video );
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
		else if( transform == 3 )
			image_transform_multi( &image );

		if( keyframe )
		{
			if( colordiff >= 2 )
			{
				if( ! qtc_compress_color_diff( &image, NULL, &compimage, minsize, maxdepth, lazyness ) )
					return 0;
			}
			else
			{
				if( ! qtc_compress( &image, NULL, &compimage, minsize, maxdepth, lazyness ) )
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
				if( ! qtc_compress( &image, &refimage, &compimage, minsize, maxdepth, lazyness ) )
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

		image_copy( &image, &refimage );

		image_free( &image );
		qti_free( &compimage );
		
		if( interrupt )
			done = 1;

		if( verbose )
		{
			fprintf( stderr, "Frame:%i FPS:%.2f In:%lukb/s Buff:%lukb/s,%f%% Out:%lukb/s,%f%% Curr:%lukb/s\n", framenum, fps,
				(insize*8)/(framenum+1)*framerate/1000,
				bsize/(framenum/framerate+1)/1000, bsize*100.0/(insize*8),
				(outsize*8)/(framenum+1)*framerate/1000, outsize*100.0/insize,
				(size*8)*framerate/1000 );
		}
		
		framenum++;

		delay = (1000000l/(long int)framerate)-(get_time()-start);

		if( delay > 0 )
			usleep( delay );
		
		fps = fps*0.75 + 0.25*(1000000l/(get_time()-start));
	}
	while( ( ! done ) && ( framenum != numframes ) );

	if( index )
		outsize += qtv_write_index( &video );

	x11grabber_free( &grabber );

	image_free( &refimage );
	qtv_free( &video );

	if( verbose )
	{
		fprintf( stderr, "In:%lumiB Buff:%lumiB,%f%% Out:%lumiB,%f%%\n",
			insize/1024/1024,
			bsize/8/1024/1024, (bsize/8)*100.0/insize,
			outsize/1024/1024, outsize*100.0/insize );
	}

	free( infile );
	free( outfile );

	return 0;
}

