/*
*    QTC: qti.c (c) 2011, 2012 50m30n3
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
#include <string.h>

#include "databuffer.h"
#include "rangecode.h"
#include "tilecache.h"

#include "qti.h"

#define FILEVERSION "QTI1"
#define VERSION 4

/*******************************************************************************
* Function to load and decompress a qti file                                   *
*                                                                              *
* image is the uninitialized qti structure to load into                        *
* filename is the file name of the qti file                                    *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qti_read( struct qti *image, char filename[] )
{
	FILE * qti;
	struct databuffer *compdata;
	struct rangecoder *coder;
	char header[4];
	int width, height;
	int minsize, maxdepth, cachesize, tilesize;
	int compress, cache;
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
			fputs( "qti_read: Short read on header\n", stderr );
			if( qti != stdin )
				fclose( qti );
			return 0;
		}

		if( strncmp( header, FILEVERSION, 4 ) != 0 )
		{
			fputs( "qti_read: Invalid header\n", stderr );
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
			fputs( "qti_read: Short read on image header\n", stderr );
			if( qti != stdin )
				fclose( qti );
			return 0;
		}

		if( version != VERSION )
		{
			fputs( "qti_read: Wrong version\n", stderr );
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
		image->colordiff = ( ( flags & (0x03<<3) ) >> 3 ) & 0x03;
		cache = ( ( flags & (0x01<<5) ) >> 5 ) & 0x01;
		image->keyframe = 1;

		if( cache )
		{
			if( ( fread( &cachesize, sizeof( cachesize ), 1, qti ) != 1 ) ||
			    ( fread( &tilesize, sizeof( tilesize ), 1, qti ) != 1 ) )
			{
				fputs( "qti_read: Short read on cache info\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			image->tilecache = tilecache_create( cachesize, tilesize );
			if( image->tilecache == NULL )
				return 0;
		}
		else
		{
			image->tilecache = NULL;
		}

		if( compress )
		{
			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "qti_read: Short read on compressed command data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			compdata = databuffer_create( size );
			if( compdata == NULL )
				return 0;

			compdata->size = size;

			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "qti_read: Short read on uncompressed command data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			if( fread( compdata->data, 1, compdata->size, qti ) != compdata->size )
			{
				fputs( "qti_read: Short read on compressed command data\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}
			
			image->commanddata = databuffer_create( size );
			if( image->commanddata == NULL )
				return 0;

			coder = rangecoder_create( 8, 1 );

			rangecode_decompress( coder, compdata, image->commanddata, size );
			
			rangecoder_free( coder );
			databuffer_free( compdata );


			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "qti_read: Short read on compressed image data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			compdata = databuffer_create( size );
			if( compdata == NULL )
				return 0;

			compdata->size = size;

			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "qti_read: Short read on uncompressed image data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			if( fread( compdata->data, 1, compdata->size, qti ) != compdata->size )
			{
				fputs( "qti_read: Short read on compressed image data\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}
			
			image->imagedata = databuffer_create( size );
			if( image->imagedata == NULL )
				return 0;

			coder = rangecoder_create( 2, 8 );

			rangecode_decompress( coder, compdata, image->imagedata, size );
			
			rangecoder_free( coder );
			databuffer_free( compdata );


			if( cache )
			{
				if( fread( &size, sizeof( size ), 1, qti ) != 1 )
				{
					fputs( "qti_read: Short read on compressed index data size\n", stderr );
					if( qti != stdin )
						fclose( qti );
					return 0;
				}

				compdata = databuffer_create( size );
				if( compdata == NULL )
					return 0;

				compdata->size = size;

				if( fread( &size, sizeof( size ), 1, qti ) != 1 )
				{
					fputs( "qti_read: Short read on uncompressed index data size\n", stderr );
					if( qti != stdin )
						fclose( qti );
					return 0;
				}

				if( fread( compdata->data, 1, compdata->size, qti ) != compdata->size )
				{
					fputs( "qti_read: Short read on compressed index data\n", stderr );
					if( qti != stdin )
						fclose( qti );
					return 0;
				}
			
				image->indexdata = databuffer_create( size );
				if( image->indexdata == NULL )
					return 0;

				coder = rangecoder_create( 2, 8 );

				rangecode_decompress( coder, compdata, image->indexdata, size );
			
				rangecoder_free( coder );
				databuffer_free( compdata );
			}
		}
		else
		{
			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "qti_read: Short read on command data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			image->commanddata = databuffer_create( size );
			if( image->commanddata == NULL )
				return 0;

			image->commanddata->size = size;
			if( fread( image->commanddata->data, 1, image->commanddata->size, qti ) != image->commanddata->size )
			{
				fputs( "qti_read: Short read on command data\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}


			if( fread( &size, sizeof( size ), 1, qti ) != 1 )
			{
				fputs( "qti_read: Short read on image data size\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}

			image->imagedata = databuffer_create( size );
			if( image->imagedata == NULL )
				return 0;

			image->imagedata->size = size;
			if( fread( image->imagedata->data, 1, image->imagedata->size, qti ) != image->imagedata->size )
			{
				fputs( "qti_read: Short read on image data\n", stderr );
				if( qti != stdin )
					fclose( qti );
				return 0;
			}


			if( cache )
			{
				if( fread( &size, sizeof( size ), 1, qti ) != 1 )
				{
					fputs( "qti_read: Short read on index data size\n", stderr );
					if( qti != stdin )
						fclose( qti );
					return 0;
				}

				image->indexdata = databuffer_create( size );
				if( image->indexdata == NULL )
					return 0;

				image->indexdata->size = size;
				if( fread( image->indexdata->data, 1, image->indexdata->size, qti ) != image->indexdata->size )
				{
					fputs( "qti_read: Short read on index data\n", stderr );
					if( qti != stdin )
						fclose( qti );
					return 0;
				}
			}
		}
		
		if( qti != stdin )
			fclose( qti );
		
		return 1;
	}
	else
	{
		perror( "qti_read" );
		return 0;
	}
}

/*******************************************************************************
* Function to compress a qti and write it to a file                            *
*                                                                              *
* image is the image to be written                                             *
* filename is the file name of the new qti file                                *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qti_write( struct qti *image, int compress, char filename[] )
{
	FILE * qti;
	struct databuffer *compdata;
	struct rangecoder *coder;
	unsigned char flags, version;
	unsigned int size;

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
		flags |= ( image->colordiff & 0x03 ) << 3;
		flags |= (image->tilecache!=NULL) << 5;
		version = VERSION;
		
		fwrite( &(version), sizeof( version ), 1, qti );
		fwrite( &(image->width), sizeof( image->width ), 1, qti );
		fwrite( &(image->height), sizeof( image->height ), 1, qti );
		fwrite( &(flags), sizeof( flags ), 1, qti );
		fwrite( &(image->minsize), sizeof( image->minsize ), 1, qti );
		fwrite( &(image->maxdepth), sizeof( image->maxdepth ), 1, qti );
		
		if( image->tilecache != NULL )
		{
			fwrite( &(image->tilecache->size), sizeof( image->tilecache->size ), 1, qti );
			fwrite( &(image->tilecache->blocksize), sizeof( image->tilecache->blocksize ), 1, qti );
		}

		databuffer_pad( image->commanddata );
		databuffer_pad( image->imagedata );

		if( image->tilecache != NULL )
			databuffer_pad( image->indexdata );
		
		size = 0;

		if( compress )
		{
			compdata = databuffer_create( image->commanddata->size );
			if( compdata == NULL )
				return 0;

			coder = rangecoder_create( 8, 1 );
			if( coder == NULL )
				return 0;

			rangecode_compress( coder, image->commanddata, compdata );
			databuffer_pad( compdata );

			fwrite( &(compdata->size), sizeof( compdata->size ), 1, qti );
			fwrite( &(image->commanddata->size), sizeof( image->commanddata->size ), 1, qti );
			fwrite( compdata->data, 1, compdata->size, qti );

			size += sizeof( compdata->size ) + sizeof( image->commanddata->size ) + compdata->size;

			rangecoder_free( coder );
			databuffer_free( compdata );


			compdata = databuffer_create( image->imagedata->size / 2 + 1 );
			if( compdata == NULL )
				return 0;

			coder = rangecoder_create( 2, 8 );
			if( coder == NULL )
				return 0;

			rangecode_compress( coder, image->imagedata, compdata );
			databuffer_pad( compdata );

			fwrite( &(compdata->size), sizeof( compdata->size ), 1, qti );
			fwrite( &(image->imagedata->size), sizeof( image->imagedata->size ), 1, qti );
			fwrite( compdata->data, 1, compdata->size, qti );

			size += sizeof( compdata->size ) + sizeof( image->imagedata->size ) + compdata->size;

			rangecoder_free( coder );
			databuffer_free( compdata );

			if( image->tilecache != NULL )
			{
				compdata = databuffer_create( image->indexdata->size / 2 + 1 );
				if( compdata == NULL )
					return 0;

				coder = rangecoder_create( 2, 8 );
				if( coder == NULL )
					return 0;

				rangecode_compress( coder, image->indexdata, compdata );
				databuffer_pad( compdata );

				fwrite( &(compdata->size), sizeof( compdata->size ), 1, qti );
				fwrite( &(image->indexdata->size), sizeof( image->indexdata->size ), 1, qti );
				fwrite( compdata->data, 1, compdata->size, qti );

				size += sizeof( compdata->size ) + sizeof( image->indexdata->size ) + compdata->size;

				rangecoder_free( coder );
				databuffer_free( compdata );
			}
		}
		else
		{
			fwrite( &(image->commanddata->size), sizeof( image->commanddata->size ), 1, qti );
			fwrite( image->commanddata->data, 1, image->commanddata->size, qti );
			
			size += sizeof( image->commanddata->size ) + image->commanddata->size;


			fwrite( &(image->imagedata->size), sizeof( image->imagedata->size ), 1, qti );
			fwrite( image->imagedata->data, 1, image->imagedata->size, qti );
			
			size += sizeof( image->imagedata->size ) + image->imagedata->size;

			if( image->tilecache != NULL )
			{
				fwrite( &(image->indexdata->size), sizeof( image->indexdata->size ), 1, qti );
				fwrite( image->indexdata->data, 1, image->indexdata->size, qti );
			
				size += sizeof( image->indexdata->size ) + image->indexdata->size;
			}
		}

		if( qti != stdout )
			fclose( qti );
		
		return size;
	}
	else
	{
		perror( "qti_write" );
		return 0;
	}
}

/*******************************************************************************
* Function to initialize a qti structure                                       *
*                                                                              *
* image is a pointer to an qti struct to hold the information                  *
* with is the width of the image                                               *
* height is the height of the image                                            *
* minsize is the minimal block size used during compression                    *
* maxdepth is the maximum recursion depth used during compression              *
*                                                                              *
* Modifies the qti structure                                                   *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qti_create( struct qti *image, int width, int height, int minsize, int maxdepth, struct tilecache *cache )
{
	image->width = width;
	image->height = height;
	image->minsize = minsize;
	image->maxdepth = maxdepth;
	image->transform = 0;
	image->colordiff = 0;
	image->keyframe = 0;

	image->tilecache = cache;

	image->imagedata = databuffer_create( 1024*512 );
	if( image->imagedata == NULL )
		return 0;

	image->commanddata = databuffer_create( 1204 );
	if( image->commanddata == NULL )
		return 0;

	if( cache != NULL )
	{
		image->indexdata = databuffer_create( 1024*64 );
		if( image->indexdata == NULL )
			return 0;
	}

	return 1;
}

/*******************************************************************************
* Function to free the internal structures of a qti                            *
*                                                                              *
* image is the qti to free                                                     *
*                                                                              *
* Modifies image                                                               *
*******************************************************************************/
void qti_free( struct qti *image )
{
	databuffer_free( image->imagedata );
	image->imagedata = NULL;
	databuffer_free( image->commanddata );
	image->commanddata = NULL;

	if( image->tilecache != NULL )
	{
		databuffer_free( image->indexdata );
		image->indexdata = NULL;
	}
}

/*******************************************************************************
* Function to free the uncompressed size of a qti image in bits                *
*                                                                              *
* image is the qti to return the size of                                       *
*******************************************************************************/
unsigned int qti_getsize( struct qti *image )
{
	unsigned int size=0;
	
	size += image->imagedata->size*8+image->imagedata->bits;
	size += image->commanddata->size*8+image->commanddata->bits;
	
	if( image->tilecache != NULL )
		size += image->indexdata->size*8+image->indexdata->bits;
	
	return size;
}

