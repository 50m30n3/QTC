#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "image.h"
#include "qti.h"
#include "qtc.h"
#include "qtv.h"
#include "ppm.h"

int interrupt;

void sig_exit( int sig )
{
	interrupt = 1;
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

	int opt, verbose;
	unsigned long int insize, bsize, outsize;
	int done, tmp, keyframe, framenum;
	int transform;
	int rangecomp, compress;
	int minsize;
	int maxdepth;
	int maxerror;
	int lazyness;
	int index;
	int framerate, keyrate, startframe, numframes;
	char mode;
	char *infile, *outfile;

	verbose = 0;
	transform = 0;
	rangecomp = 0;
	minsize = 2;
	maxdepth = 16;
	maxerror = 0;
	lazyness = 0;
	framerate = 25;
	keyrate = 0;
	index = 0;
	startframe = 0;
	numframes = -1;
	mode = 'c';
	infile = NULL;
	outfile = NULL;

	while( ( opt = getopt( argc, argv, "cvxf:n:t:s:d:l:e:r:k:m:i:o:" ) ) != -1 )
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

			case 'v':
				verbose = 1;
			break;

			case 'x':
				index = 1;
			break;

			case 's':
				if( sscanf( optarg, "%i", &minsize ) != 1 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 'f':
				if( sscanf( optarg, "%i", &startframe ) != 1 )
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

			case 'e':
				if( sscanf( optarg, "%i", &maxerror ) != 1 )
					fputs( "ERROR: Can not parse command line\n", stderr );
			break;

			case 'm':
				if( sscanf( optarg, "%c", &mode ) != 1 )
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

	if( mode == 'c' )
	{
		done = 0;
		framenum = 0;

		insize = 0;
		bsize = 0;
		outsize = 0;

		do
		{
			if( keyrate == 0 )
				keyframe = framenum == 0;
			else
				keyframe = framenum % ( keyrate * framerate ) == 0;

			if( ! ppm_read( &image, infile ) )
				return 0;

			if( framenum == 0 )
			{
				qtv_create( image.width, image.height, framerate, index, &video );
				qtv_write_header( &video, outfile );
				
				image_create( &refimage, image.width, image.height );
			}

			insize += ( image.width * image.height * 3 );

			if( transform == 1 )
				image_transform_fast( &image );
			else if( transform == 2 )
				image_transform( &image );

			if( keyframe )
			{
				if( ! qtc_compress( &image, NULL, &compimage, maxerror, minsize, maxdepth, lazyness ) )
					return 0;
			}
			else
			{
				if( ! qtc_compress( &image, &refimage, &compimage, maxerror, minsize, maxdepth, lazyness ) )
					return 0;
			}

			bsize += qti_getsize( &compimage );

			compimage.transform = transform;

			if( qti_getsize( &compimage ) <= 4 )
				compress = 0;
			else
				compress = rangecomp;

			if( ! ( outsize += qtv_write_frame( &video, &compimage, compress, keyframe ) ) )
				return 0;

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
				fprintf( stderr, "\r\033[2KFrame:%i In:%lukb/s Buff:%lukb/s,%f%% Out:%lukb/s,%f%%", framenum,
					insize/(framenum/framerate+1)/1024,
					bsize/8/(framenum/framerate+1)/1024, (bsize/8)*100.0/insize,
					outsize/(framenum/framerate+1)/1024, outsize*100.0/insize );
			}
			
			framenum++;
		}
		while( ( ! done ) && ( framenum != numframes ) );

		if( index )
			outsize += qtv_write_index( &video );

		image_free( &refimage );
		qtv_free( &video );

		if( verbose )
		{
			fprintf( stderr, "\nIn:%lumb Buff:%lumb,%f%% Out:%lumb,%f%%\n",
				insize/1024/1024,
				bsize/8/1024/1024, (bsize/8)*100.0/insize,
				outsize/1024/1024, outsize*100.0/insize );
		}
	}
	else if( mode == 'd' )
	{
		done = 0;
		framenum = 0;

		refimage.pixels = NULL;

		if( ! qtv_read_header( &video, infile ) )
			return 0;

		qtv_seek( &video, startframe );

		image_create( &refimage, video.width, video.height );

		do
		{
			if( ! qtv_read_frame( &video, &compimage, &keyframe ) )
				return 0;

			if( keyframe )
			{
				if( ! qtc_decompress( &compimage, NULL, &image ) )
					return 0;
			}
			else
			{
				if( ! qtc_decompress( &compimage, &refimage, &image ) )
					return 0;
			}

			image_copy( &image, &refimage );

			if( compimage.transform == 1 )
				image_transform_fast_rev( &image );
			else if( compimage.transform == 2 )
				image_transform_rev( &image );
	
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
	}
	else if( mode == 'a' )
	{
		done = 0;
		framenum = 0;

		refimage.pixels = NULL;

		if( ! qtv_read_header( &video, infile ) )
			return 0;

		qtv_seek( &video, startframe );

		image_create( &refimage, video.width, video.height );

		do
		{
			if( ! qtv_read_frame( &video, &compimage, &keyframe ) )
				return 0;

			if( ! qtc_decompress_ccode( &compimage, &image, !keyframe ) )
				return 0;

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

		qtv_free( &video );
	}
	else
	{
		fputs( "ERROR: Unknown mode\n", stderr );
		return 1;
	}

	free( infile );
	free( outfile );

	return 0;
}

