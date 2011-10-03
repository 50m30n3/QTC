#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "databuffer.h"
#include "rangecode.h"

#include "qti.h"

#define FILEVERSION "QTI1"
#define VERSION 2

int read_qti( struct qti *image, char filename[] )
{
	FILE * qti;
	struct databuffer *compdata;
	struct rangecoder *coder;
	char header[4];
	int width, height;
	int minsize, maxdepth;
	int compress;
	unsigned char flags, version;
	unsigned int size;

	if( filename == NULL )
	{
		qti = stdin;
	}
	else
	{
		if( strcmp( filename, "-" ) == 0 )
		{
			qti = stdin;
		}
		else
		{
			qti = fopen( filename, "rb" );
		}
	}
	
	if( qti != NULL )
	{
		if( fread( header, 1, 4, qti ) != 4 )
		{
			fputs( "read_qti: Short read on header\n", stderr );
			if( qti != stdin )
				fclose( qti );
			return 0;
		}

		if( strncmp( header, FILEVERSION, 4 ) != 0 )
		{
			fputs( "read_qti: Invalid header\n", stderr );
			if( qti != stdin )
				fclose( qti );
			return 0;
		}

		if( ( fread( &version, sizeof( version ), 1, qti ) != 1 ) ||
			( fread( &width, sizeof( width ), 1, qti ) != 1 ) ||
			( fread( &height, sizeof( height ), 1, qti ) != 1 ) ||
			( fread( &flags, sizeof( flags ), 1, qti ) != 1 ) ||
			( fread( &minsize, sizeof( minsize ), 1, qti ) != 1 ) ||
			( fread( &maxdepth, sizeof( maxdepth ), 1, qti ) != 1 ) )
		{
			fputs( "read_qti: Short read on image header\n", stderr );
			if( qti != stdin )
				fclose( qti );
			return 0;
		}

		if( version != VERSION )
		{
			fputs( "read_qti: Wrong version\n", stderr );
			if( qti != stdin )
				fclose( qti );
			return 0;
		}

		image->width = width;
		image->height = height;
		image->minsize = minsize;
		image->maxdepth = maxdepth;
		image->transform = flags&0x03;
		compress = ( ( flags & (0x01<<2) ) >> 2 ) & 0x01;

		if( compress )
		{
			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "read_qti: Short read on compressed command data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			compdata = create_databuffer( size );
			if( compdata == NULL )
				return 0;

			compdata->size = size;

			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "read_qti: Short read on uncompressed command data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			if( fread( compdata->data, 1, compdata->size, qti ) != compdata->size )
			{
				fputs( "read_qti: Short read on compressed command data\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}
			
			image->commanddata = create_databuffer( size );
			if( image->commanddata == NULL )
				return 0;

			coder = create_rangecoder( 0 );

			rangecode_decompress( coder, compdata, image->commanddata, size );
			
			free_rangecoder( coder );
			free_databuffer( compdata );


			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "read_qti: Short read on compressed image data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			compdata = create_databuffer( size );
			if( compdata == NULL )
				return 0;

			compdata->size = size;

			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "read_qti: Short read on uncompressed image data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			if( fread( compdata->data, 1, compdata->size, qti ) != compdata->size )
			{
				fputs( "read_qti: Short read on compressed image data\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}
			
			image->imagedata = create_databuffer( size );
			if( image->imagedata == NULL )
				return 0;

			coder = create_rangecoder( 1 );

			rangecode_decompress( coder, compdata, image->imagedata, size );
			
			free_rangecoder( coder );
			free_databuffer( compdata );
		}
		else
		{
			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "read_qti: Short read on command data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			image->commanddata = create_databuffer( size );
			if( image->commanddata == NULL )
				return 0;

			image->commanddata->size = size;
			if( fread( image->commanddata->data, 1, image->commanddata->size, qti ) != image->commanddata->size )
			{
				fputs( "read_qti: Short read on command data\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}


			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "read_qti: Short read on image data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			image->imagedata = create_databuffer( size );
			if( image->imagedata == NULL )
				return 0;

			image->imagedata->size = size;
			if( fread( image->imagedata->data, 1, image->imagedata->size, qti ) != image->imagedata->size )
			{
				fputs( "read_qti: Short read on image data\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}
		}
		
		if( qti != stdin )
			fclose( qti );
		
		return 1;
	}
	else
	{
		perror( "read_qti" );
		return 0;
	}
}

int write_qti( struct qti *image, int compress, char filename[] )
{
	FILE * qti;
	struct databuffer *compdata;
	struct rangecoder *coder;
	unsigned char flags, version;
	int size;

	if( filename == NULL )
	{
		qti = stdout;
	}
	else
	{
		if( strcmp( filename, "-" ) == 0 )
		{
			qti = stdout;
		}
		else
		{
			qti = fopen( filename, "wb" );
		}
	}
	
	if( qti != NULL )
	{
		fwrite( FILEVERSION, 1, 4, qti );
		
		flags = 0;
		flags |= image->transform & 0x03;
		flags |= ( compress & 0x01 ) << 2;
		version = 1;
		
		fwrite( &(version), sizeof( version ), VERSION, qti );
		fwrite( &(image->width), sizeof( image->width ), 1, qti );
		fwrite( &(image->height), sizeof( image->height ), 1, qti );
		fwrite( &(flags), sizeof( flags ), 1, qti );
		fwrite( &(image->minsize), sizeof( image->minsize ), 1, qti );
		fwrite( &(image->maxdepth), sizeof( image->maxdepth ), 1, qti );

		pad_data( image->commanddata );
		pad_data( image->imagedata );
		
		size = 0;

		if( compress )
		{
			compdata = create_databuffer( image->commanddata->size );
			if( compdata == NULL )
				return 0;

			coder = create_rangecoder( 0 );
			if( coder == NULL )
				return 0;

			rangecode_compress( coder, image->commanddata, compdata );
			pad_data( compdata );

			fwrite( &(compdata->size), sizeof( compdata->size ), 1, qti );
			fwrite( &(image->commanddata->size), sizeof( image->commanddata->size ), 1, qti );
			fwrite( compdata->data, 1, compdata->size, qti );

			size += sizeof( compdata->size ) + sizeof( image->commanddata->size ) + compdata->size;

			free_rangecoder( coder );
			free_databuffer( compdata );


			compdata = create_databuffer( image->imagedata->size / 2 + 1 );
			if( compdata == NULL )
				return 0;

			coder = create_rangecoder( 1 );
			if( coder == NULL )
				return 0;

			rangecode_compress( coder, image->imagedata, compdata );
			pad_data( compdata );

			fwrite( &(compdata->size), sizeof( compdata->size ), 1, qti );
			fwrite( &(image->imagedata->size), sizeof( image->imagedata->size ), 1, qti );
			fwrite( compdata->data, 1, compdata->size, qti );

			size += sizeof( compdata->size ) + sizeof( image->imagedata->size ) + compdata->size;

			free_rangecoder( coder );
			free_databuffer( compdata );
		}
		else
		{
			fwrite( &(image->commanddata->size), sizeof( image->commanddata->size ), 1, qti );
			fwrite( image->commanddata->data, 1, image->commanddata->size, qti );
			
			size += sizeof( image->commanddata->size ) + image->commanddata->size;


			fwrite( &(image->imagedata->size), sizeof( image->imagedata->size ), 1, qti );
			fwrite( image->imagedata->data, 1, image->imagedata->size, qti );
			
			size += sizeof( image->imagedata->size ) + image->imagedata->size;
		}

		if( qti != stdout )
			fclose( qti );
		
		return size;
	}
	else
	{
		perror( "write_qti" );
		return 0;
	}
}

int create_qti( int width, int height, int minsize, int maxdepth, struct qti *image )
{
	image->width = width;
	image->height = height;
	image->minsize = minsize;
	image->maxdepth = maxdepth;
	image->transform = 0;

	image->imagedata = create_databuffer( 1024*512 );
	if( image->imagedata == NULL )
		return 0;

	image->commanddata = create_databuffer( 1204 );
	if( image->commanddata == NULL )
		return 0;
	
	return 1;
}

void free_qti( struct qti *image )
{
	free_databuffer( image->imagedata );
	image->imagedata = NULL;
	free_databuffer( image->commanddata );
	image->commanddata = NULL;
}

unsigned int qti_getsize( struct qti *image )
{
	unsigned int size=0;
	
	size += image->imagedata->size*8+image->imagedata->bits;
	size += image->commanddata->size*8+image->commanddata->bits;
	
	return size;
}

unsigned int qti_getcsize( struct qti *image )
{
	unsigned int size=0;
	
	size += image->commanddata->size*8+image->commanddata->bits;
	
	return size;
}

unsigned int qti_getdsize( struct qti *image )
{
	unsigned int size=0;
	
	size += image->imagedata->size*8+image->imagedata->bits;
	
	return size;
}

