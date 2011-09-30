#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "image.h"
#include "qti.h"
#include "qtc.h"
#include "qtv.h"
#include "ppm.h"

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
	int rangecomp;
	int minsize;
	int maxdepth;
	int maxerror;
	int lazyness;
	int framerate, keyrate;
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
	mode = 'c';
	infile = NULL;
	outfile = NULL;

	while( ( opt = getopt( argc, argv, "cvt:s:d:l:e:r:k:m:i:o:" ) ) != -1 )
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

			case 's':
				if( sscanf( optarg, "%i", &minsize ) != 1 )
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
				keyframe = framenum  == 0;
			else
				keyframe = framenum % ( keyrate * framerate ) == 0;

			if( ! read_ppm( &image, infile ) )
				return 0;

			if( framenum == 0 )
			{
				create_qtv( image.width, image.height, framerate, &video );
				write_qtv_header( &video, outfile );
				
				create_image( &refimage, image.width, image.height );
			}

			insize += ( image.width * image.height * 3 );

			if( transform == 1 )
				transform_image_fast( &image );
			else if( transform == 2 )
				transform_image( &image );

			if( keyframe )
			{
				if( ! compress( &image, NULL, &compimage, maxerror, minsize, maxdepth, lazyness ) )
					return 0;
			}
			else
			{
				if( ! compress( &image, &refimage, &compimage, maxerror, minsize, maxdepth, lazyness ) )
					return 0;
			}

			bsize += qti_getsize( &compimage );

			compimage.transform = transform;

			if( ! ( outsize += write_qtv_frame( &video, &compimage, rangecomp, keyframe ) ) )
				return 0;

			copy_image( &image, &refimage );

			free_image( &image );
			free_qti( &compimage );
			
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

			if( verbose )
			{
				fprintf( stderr, "\r\033[2KFrame:%i In:%lukb/s Buff:%lukb/s,%f%% Out:%lukb/s,%f%%", framenum,
					insize/(framenum/framerate+1)/1024,
					bsize/8/(framenum/framerate+1)/1024, (bsize/8)*100.0/insize,
					outsize/(framenum/framerate+1)/1024, outsize*100.0/insize );
			}
			
			framenum++;
		}
		while( ! done );

		free_image( &refimage );
		free_qtv( &video );

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

		if( ! read_qtv_header( &video, infile ) )
			return 0;

		create_image( &refimage, video.width, video.height );

		do
		{
			if( ! read_qtv_frame( &video, &compimage, &keyframe ) )
				return 0;

			if( keyframe )
			{
				if( ! decompress( &compimage, NULL, &image ) )
					return 0;
			}
			else
			{
				if( ! decompress( &compimage, &refimage, &image ) )
					return 0;
			}

			copy_image( &image, &refimage );

			if( compimage.transform == 1 )
				transform_image_fast_rev( &image );
			else if( compimage.transform == 2 )
				transform_image_rev( &image );
	
			if( ! write_ppm( &image, outfile ) )
				return 0;

			free_image( &image );
			free_qti( &compimage );

			if( ( outfile != NULL ) && ( strcmp( outfile, "-" ) != 0 ) )
			{
				if( !inc_filename( outfile ) )
					done = 1;
			}

			if( ! can_read_frame( &video ) )
				done = 1;

			framenum++;
		}
		while( !done );

		free_image( &refimage );
		free_qtv( &video );
	}
	else if( mode == 'a' )
	{
		done = 0;
		framenum = 0;

		refimage.pixels = NULL;

		if( ! read_qtv_header( &video, infile ) )
			return 0;

		create_image( &refimage, video.width, video.height );

		do
		{
			if( ! read_qtv_frame( &video, &compimage, &keyframe ) )
				return 0;

			if( ! decompress_ccode( &compimage, &image, !keyframe ) )
				return 0;

			if( ! write_ppm( &image, outfile ) )
				return 0;

			free_image( &image );
			free_qti( &compimage );

			if( ( outfile != NULL ) && ( strcmp( outfile, "-" ) != 0 ) )
			{
				if( !inc_filename( outfile ) )
					done = 1;
			}

			if( ! can_read_frame( &video ) )
				done = 1;

			framenum++;
		}
		while( !done );

		free_qtv( &video );
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

