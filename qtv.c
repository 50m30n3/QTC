#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "databuffer.h"
#include "rangecode.h"
#include "qti.h"

#include "qtv.h"

#define FILEVERSION "QTV1"
#define VERSION 2

int read_qtv_header( struct qtv *video, char filename[] )
{
	FILE * qtv;
	char header[4];
	int width, height, framerate;
	unsigned char version;

	if( filename == NULL )
	{
		qtv = stdin;
	}
	else
	{
		if( strcmp( filename, "-" ) == 0 )
		{
			qtv = stdin;
		}
		else
		{
			qtv = fopen( filename, "rb" );
		}
	}
	
	if( qtv != NULL )
	{
		video->file = qtv;

		if( fread( header, 1, 4, qtv ) != 4 )
		{
			fputs( "read_qtv_header: Short read on header\n", stderr );
			if( qtv != stdin )
				fclose( qtv );
			return 0;
		}

		if( strncmp( header, FILEVERSION, 4 ) != 0 )
		{
			fputs( "read_qtv_header: Invalid header\n", stderr );
			if( qtv != stdin )
				fclose( qtv );
			return 0;
		}

		if( ( fread( &version, sizeof( version ), 1, qtv ) != 1 ) ||
			( fread( &width, sizeof( width ), 1, qtv ) != 1 ) ||
			( fread( &height, sizeof( height ), 1, qtv ) != 1 ) ||
			( fread( &framerate, sizeof( framerate ), 1, qtv ) != 1 ) )
		{
			fputs( "read_qtv_header: Short read on image header\n", stderr );
			if( qtv != stdin )
				fclose( qtv );
			return 0;
		}

		if( version != VERSION )
		{
			fputs( "read_qtv_header: Wrong version\n", stderr );
			if( qtv != stdin )
				fclose( qtv );
			return 0;
		}

		video->width = width;
		video->height = height;
		video->framerate = framerate;

		video->cmdcoder = create_rangecoder( 0 );
		if( video->cmdcoder == NULL )
			return 0;

		video->imgcoder = create_rangecoder( 1 );
		if( video->imgcoder == NULL )
			return 0;
		
		return 1;
	}
	else
	{
		perror( "read_qtv_header" );
		return 0;
	}
}

int read_qtv_frame( struct qtv *video, struct qti *image, int *keyframe )
{
	FILE * qtv;
	struct databuffer *compdata;
	struct rangecoder *coder;
	int minsize, maxdepth;
	int compress;
	unsigned char flags;
	unsigned int size;

	qtv = video->file;
	
	if( qtv != NULL )
	{
		if( ( fread( &flags, sizeof( flags ), 1, qtv ) != 1 ) ||
			( fread( &minsize, sizeof( minsize ), 1, qtv ) != 1 ) ||
			( fread( &maxdepth, sizeof( maxdepth ), 1, qtv ) != 1 ) )
		{
			fputs( "read_qtv_frame: Short read on image header\n", stderr );
			if( qtv != stdin )
				fclose( qtv );
			return 0;
		}

		image->width = video->width;
		image->height = video->height;
		image->minsize = minsize;
		image->maxdepth = maxdepth;
		image->transform = flags&0x03;
		compress = ( ( flags & (0x01<<2) ) >> 2 ) & 0x01;
		*keyframe = ( ( flags & (0x01<<3) ) >> 3 ) & 0x01;

		if( compress )
		{
			if( *keyframe )
			{
				reset_model( video->cmdcoder );
				reset_model( video->imgcoder );
			}

			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "read_qtv_frame: Short read on compressed command data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			compdata = create_databuffer( size );
			if( compdata == NULL )
				return 0;

			compdata->size = size;

			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "read_qtv_frame: Short read on uncompressed command data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			if( fread( compdata->data, 1, compdata->size, qtv ) != compdata->size )
			{
				fputs( "read_qtv_frame: Short read on compressed command data\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}
			
			image->commanddata = create_databuffer( size );
			if( image->commanddata == NULL )
				return 0;

			coder = video->cmdcoder;

			rangecode_decompress( coder, compdata, image->commanddata, size );
			
			free_databuffer( compdata );


			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "read_qtv_frame: Short read on compressed image data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			compdata = create_databuffer( size );
			if( compdata == NULL )
				return 0;

			compdata->size = size;

			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "read_qtv_frame: Short read on uncompressed image data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			if( fread( compdata->data, 1, compdata->size, qtv ) != compdata->size )
			{
				fputs( "read_qtv_frame: Short read on compressed image data\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}
			
			image->imagedata = create_databuffer( size );
			if( image->imagedata == NULL )
				return 0;

			coder = video->imgcoder;

			rangecode_decompress( coder, compdata, image->imagedata, size );
			
			free_databuffer( compdata );
		}
		else
		{
			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "read_qtv_frame: Short read on command data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			image->commanddata = create_databuffer( size );
			if( image->commanddata == NULL )
				return 0;

			image->commanddata->size = size;
			if( fread( image->commanddata->data, 1, image->commanddata->size, qtv ) != image->commanddata->size )
			{
				fputs( "read_qtv_frame: Short read on command data\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}


			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "read_qtv_frame: Short read on image data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			image->imagedata = create_databuffer( size );
			if( image->imagedata == NULL )
				return 0;

			image->imagedata->size = size;
			if( fread( image->imagedata->data, 1, image->imagedata->size, qtv ) != image->imagedata->size )
			{
				fputs( "read_qtv_frame: Short read on image data\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}
		}
		
		return 1;
	}
	else
	{
		fputs( "read_qtv: no video opened\n", stderr );
		return 0;
	}
}

int can_read_frame( struct qtv *video )
{
	int tmp;

	tmp = getc( video->file );
	if( feof( video->file ) )
		return 0;
	else
		ungetc( tmp, video->file );

	return 1;
}

int write_qtv_header( struct qtv *video, char filename[] )
{
	FILE * qtv;
	unsigned char version;

	if( filename == NULL )
	{
		qtv = stdout;
	}
	else
	{
		if( strcmp( filename, "-" ) == 0 )
		{
			qtv = stdout;
		}
		else
		{
			qtv = fopen( filename, "wb" );
		}
	}
	
	if( qtv != NULL )
	{
		video->file = qtv;

		fwrite( FILEVERSION, 1, 4, qtv );

		version = VERSION;
		
		fwrite( &(version), sizeof( version ), 1, qtv );
		fwrite( &(video->width), sizeof( video->width ), 1, qtv );
		fwrite( &(video->height), sizeof( video->height ), 1, qtv );
		fwrite( &(video->framerate), sizeof( video->framerate ), 1, qtv );

		return 1;
	}
	else
	{
		perror( "write_qtv" );
		return 0;
	}
}

int write_qtv_frame( struct qtv *video, struct qti *image, int compress, int keyframe )
{
	FILE * qtv;
	struct databuffer *compdata;
	struct rangecoder *coder;
	unsigned char flags;
	int size;

	qtv = video->file;

	if( ( image->width != video->width ) || ( image->height != video->height ) )
	{
		fputs( "write_qtv: frame size mismatch\n", stderr );
		return 0;
	}

	if( qtv != NULL )
	{
		flags = 0;
		flags |= image->transform & 0x03;
		flags |= ( compress & 0x01 ) << 2;
		flags |= ( keyframe & 0x01 ) << 3;
		
		fwrite( &(flags), sizeof( flags ), 1, qtv );
		fwrite( &(image->minsize), sizeof( image->minsize ), 1, qtv );
		fwrite( &(image->maxdepth), sizeof( image->maxdepth ), 1, qtv );

		pad_data( image->commanddata );
		pad_data( image->imagedata );

		size = 0;

		if( compress )
		{
			if( keyframe )
			{
				reset_model( video->cmdcoder );
				reset_model( video->imgcoder );
			}

			compdata = create_databuffer( image->commanddata->size );
			if( compdata == NULL )
				return 0;

			coder = video->cmdcoder;

			rangecode_compress( coder, image->commanddata, compdata );
			pad_data( compdata );

			fwrite( &(compdata->size), sizeof( compdata->size ), 1, qtv );
			fwrite( &(image->commanddata->size), sizeof( image->commanddata->size ), 1, qtv );
			fwrite( compdata->data, 1, compdata->size, qtv );

			size += sizeof( compdata->size ) + sizeof( image->commanddata->size ) + compdata->size;
			
			free_databuffer( compdata );


			compdata = create_databuffer( image->imagedata->size / 2 + 1 );
			if( compdata == NULL )
				return 0;

			coder = video->imgcoder;
			rangecode_compress( coder, image->imagedata, compdata );
			pad_data( compdata );

			fwrite( &(compdata->size), sizeof( compdata->size ), 1, qtv );
			fwrite( &(image->imagedata->size), sizeof( image->imagedata->size ), 1, qtv );
			fwrite( compdata->data, 1, compdata->size, qtv );
			
			size += sizeof( compdata->size ) + sizeof( image->imagedata->size ) + compdata->size;
			
			free_databuffer( compdata );
		}
		else
		{
			fwrite( &(image->commanddata->size), sizeof( image->commanddata->size ), 1, qtv );
			fwrite( image->commanddata->data, 1, image->commanddata->size, qtv );

			size += sizeof( image->commanddata->size ) + image->commanddata->size;


			fwrite( &(image->imagedata->size), sizeof( image->imagedata->size ), 1, qtv );
			fwrite( image->imagedata->data, 1, image->imagedata->size, qtv );
			
			size += sizeof( image->imagedata->size ) + image->imagedata->size;
		}

		return size;
	}
	else
	{
		fputs( "write_qtv: no video opened\n", stderr );
		return 0;
	}
}

int create_qtv( int width, int height, int framerate, struct qtv *video )
{
	video->width = width;
	video->height = height;
	video->framerate = framerate;
	video->file = NULL;

	video->cmdcoder = create_rangecoder( 0 );
	if( video->cmdcoder == NULL )
		return 0;

	video->imgcoder = create_rangecoder( 1 );
	if( video->imgcoder == NULL )
		return 0;
	
	return 1;
}

void free_qtv( struct qtv *video )
{
	free_rangecoder( video->cmdcoder );
	video->cmdcoder = NULL;
	free_rangecoder( video->imgcoder );
	video->imgcoder = NULL;
	
	if( ( video->file ) && ( video->file != stdout ) )
		fclose( video->file );
}

