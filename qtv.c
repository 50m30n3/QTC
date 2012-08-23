/*
*    QTC: qtv.c (c) 2011, 2012 50m30n3
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

#include "qtv.h"

#define QTV_MAGIC "QTV1"
#define QTW_MAGIC "QTW1"
#define VERSION 6

/*******************************************************************************
* Function to read a qtv file header and initialize a qtv struct from it       *
*                                                                              *
* video is the uninitialized qtv structure to load into                        *
* is_qtw indicates wether the file to load is a qtw file                       *
* filename is the file name of the qtv file                                    *
*                                                                              *
* Modifies video                                                               *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtv_read_header( struct qtv *video, int is_qtw, char filename[] )
{
	FILE *qtv, *block;
	int i;
	char header[4], *magic;
	int width, height, framerate;
	int cache, cachesize, tilesize;
	unsigned char version, flags;
	int numframes, idx_size, numblocks, frame, blocknum;
	long int offset, idx_offset;
	char blockname[256];

	if( filename == NULL )
	{
		if( is_qtw )
		{
			fputs( "qtv_read_header: Cannot read QTW from stdin\n", stderr );
			return 0;
		}
		else
		{
			qtv = stdin;
		}
	}
	else
	{
		if( strcmp( filename, "-" ) == 0 )
		{
			if( is_qtw )
			{
				fputs( "qtv_read_header: Cannot read QTW from stdin\n", stderr );
				return 0;
			}
			else
			{
				qtv = stdin;
			}
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
			fputs( "qtv_read_header: Short read on header\n", stderr );
			if( qtv != stdin )
				fclose( qtv );
			return 0;
		}

		if( is_qtw )
			magic = QTW_MAGIC;
		else
			magic = QTV_MAGIC;

		if( strncmp( header, magic, 4 ) != 0 )
		{
			fputs( "qtv_read_header: Invalid header\n", stderr );
			if( qtv != stdin )
				fclose( qtv );
			return 0;
		}

		if( ( fread( &version, sizeof( version ), 1, qtv ) != 1 ) ||
		    ( fread( &width, sizeof( width ), 1, qtv ) != 1 ) ||
		    ( fread( &height, sizeof( height ), 1, qtv ) != 1 ) ||
		    ( fread( &framerate, sizeof( framerate ), 1, qtv ) != 1 ) ||
		    ( fread( &flags, sizeof( flags ), 1, qtv ) != 1 ) )
		{
			fputs( "qtv_read_header: Short read on image header\n", stderr );
			if( qtv != stdin )
				fclose( qtv );
			return 0;
		}

		if( version != VERSION )
		{
			fputs( "qtv_read_header: Wrong version\n", stderr );
			if( qtv != stdin )
				fclose( qtv );
			return 0;
		}

		video->framenum = 0;
		video->numframes = 0;
		video->blocknum = 0;
		video->numblocks = 0;
		video->width = width;
		video->height = height;
		video->framerate = framerate;
		video->is_qtw = is_qtw;
		video->has_index = flags&0x01;
		cache = (flags&0x02)!=0;

		if( qtv == stdin )
			video->has_index = 0;

		if( ( is_qtw ) && ( ! video->has_index ) )
		{
			fputs( "qtv_read_header: Cannot read QTW files without index\n", stderr );
			return 0;
		}

		if( video->has_index )
		{
			if( !is_qtw )
			{
				if( fseek( qtv, -sizeof( idx_offset ), SEEK_END ) == -1 )
				{
					perror( "qtv_read_header: fseek" );
					fclose( qtv );
					return 0;
				}
			
				if( fread( &idx_offset, sizeof( idx_offset ), 1, qtv ) != 1 )
				{
					fputs( "qtv_read_header: Cannot read index offset\n", stderr );
					fclose( qtv );
					return 0;
				}

				if( fseek( qtv, -idx_offset, SEEK_END ) == -1 )
				{
					perror( "qtv_read_header: fseek" );
					fclose( qtv );
					return 0;
				}
			}

			if( is_qtw )
			{
				if( ( fread( &numframes, sizeof( numframes ), 1, qtv ) != 1 ) ||
				    ( fread( &numblocks, sizeof( numblocks ), 1, qtv ) != 1 ) ||
				    ( fread( &idx_size, sizeof( idx_size ), 1, qtv ) != 1 ) )
				{
					fputs( "qtv_read_header: Cannot read index header\n", stderr );
					fclose( qtv );
					return 0;
				}
			}
			else
			{
				numblocks = 0;

				if( ( fread( &numframes, sizeof( numframes ), 1, qtv ) != 1 ) ||
				    ( fread( &idx_size, sizeof( idx_size ), 1, qtv ) != 1 ) )
				{
					fputs( "qtv_read_header: Cannot read index header\n", stderr );
					fclose( qtv );
					return 0;
				}
			}

			video->numframes = numframes;
			video->numblocks = numblocks;
			video->idx_size = idx_size;

			video->idx_datasize = video->idx_size;
			video->index = malloc( sizeof( *video->index ) * video->idx_datasize );
			if( video->index == NULL )
			{
				perror( "qtv_read_header: malloc" );
				fclose( qtv );
				return 0;
			}

			for( i=0; i<video->idx_size; i++ )
			{
				if( is_qtw )
				{
					if( ( fread( &frame, sizeof( frame ), 1, qtv ) != 1 ) ||
					    ( fread( &blocknum, sizeof( blocknum ), 1, qtv ) != 1 ) ||
					    ( fread( &offset, sizeof( offset ), 1, qtv ) != 1 ) )
					{
						fputs( "qtv_read_header: Cannot read index entry\n", stderr );
						fclose( qtv );
						return 0;
					}
				}
				else
				{
					blocknum = 0;

					if( ( fread( &frame, sizeof( frame ), 1, qtv ) != 1 ) ||
					    ( fread( &offset, sizeof( offset ), 1, qtv ) != 1 ) )
					{
						fputs( "qtv_read_header: Cannot read index entry\n", stderr );
						fclose( qtv );
						return 0;
					}
				}

				video->index[i].frame = frame;
				video->index[i].block = blocknum;
				video->index[i].offset = offset;
			}

			if( ! is_qtw )
			{
				if( fseek( qtv, 18, SEEK_SET ) == -1 )
				{
					perror( "qtv_read_header: fseek" );
					fclose( qtv );
					return 0;
				}
			}
		}

		if( cache )
		{
			if( ( fread( &cachesize, sizeof( cachesize ), 1, qtv ) != 1 ) ||
			    ( fread( &tilesize, sizeof( tilesize ), 1, qtv ) != 1 ) )
			{
				fputs( "qtv_read: Short read on cache info\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			video->tilecache = tilecache_create( cachesize, tilesize );
			if( video->tilecache == NULL )
				return 0;
		}
		else
		{
			video->tilecache = NULL;
		}

		video->cmdcoder = rangecoder_create( 8, 1 );
		if( video->cmdcoder == NULL )
			return 0;

		video->imgcoder = rangecoder_create( 2, 8 );
		if( video->imgcoder == NULL )
			return 0;

		if( cache )
		{
			video->idxcoder = rangecoder_create( 2, 8 );
			if( video->idxcoder == NULL )
				return 0;
		}

		if( filename )
			video->filename = strdup( filename );
		else
			video->filename = NULL;

		if( is_qtw )
		{
			snprintf( blockname, 256, "%s.%06i", video->filename, video->blocknum );
		
			block = fopen( blockname, "rb" );
			if( block == NULL )
			{
				perror( "qtv_read_header: fopen" );
				return 0;
			}
			
			video->streamfile = block;
		}
		else
		{
			video->streamfile = NULL;
		}

		return 1;
	}
	else
	{
		perror( "qtv_read_header" );
		return 0;
	}
}

/*******************************************************************************
* Function to read a single frame from an opened qtv file                      *
*                                                                              *
* video is a qtv structure as returned from qtv_read_header                    *
* image is a pointer to a qti image where the frame will be stored             *
*                                                                              *
* Modifies video                                                               *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtv_read_frame( struct qtv *video, struct qti *image )
{
	FILE *qtv;
	struct databuffer *compdata;
	struct rangecoder *coder;
	int minsize, maxdepth;
	int compress;
	int tmp;
	unsigned char flags;
	unsigned int size;
	char blockname[256];

	if( video->is_qtw )
		qtv = video->streamfile;
	else
		qtv = video->file;
	
	if( qtv != NULL )
	{
		if( video->is_qtw )
		{
			tmp = getc( qtv );
			if( feof( qtv ) )
			{
				fclose( qtv );

				video->blocknum++;

				snprintf( blockname, 256, "%s.%06i", video->filename, video->blocknum );

				qtv = fopen( blockname, "rb" );
				if( qtv == NULL )
				{
					perror( "qtv_read_frame: fopen" );
					return 0;
				}

				video->streamfile = qtv;
			}
			else
			{
				ungetc( tmp, qtv );
			}
		}

		if( ( fread( &flags, sizeof( flags ), 1, qtv ) != 1 ) ||
		    ( fread( &minsize, sizeof( minsize ), 1, qtv ) != 1 ) ||
		    ( fread( &maxdepth, sizeof( maxdepth ), 1, qtv ) != 1 ) )
		{
			fputs( "qtv_read_frame: Short read on image header\n", stderr );
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
		image->colordiff = ( ( flags & (0x03<<3) ) >> 3 ) & 0x03;
		image->keyframe = ( ( flags & (0x01<<7) ) >> 7 ) & 0x01;

		image->tilecache = video->tilecache;

		if( image->keyframe )
		{
			rangecoder_reset( video->cmdcoder );
			rangecoder_reset( video->imgcoder );
		}

		if( compress )
		{
			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "qtv_read_frame: Short read on compressed command data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			compdata = databuffer_create( size );
			if( compdata == NULL )
				return 0;

			compdata->size = size;

			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "qtv_read_frame: Short read on uncompressed command data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			if( fread( compdata->data, 1, compdata->size, qtv ) != compdata->size )
			{
				fputs( "qtv_read_frame: Short read on compressed command data\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}
			
			image->commanddata = databuffer_create( size );
			if( image->commanddata == NULL )
				return 0;

			coder = video->cmdcoder;

			rangecode_decompress( coder, compdata, image->commanddata, size );
			
			databuffer_free( compdata );


			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "qtv_read_frame: Short read on compressed image data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			compdata = databuffer_create( size );
			if( compdata == NULL )
				return 0;

			compdata->size = size;

			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "qtv_read_frame: Short read on uncompressed image data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			if( fread( compdata->data, 1, compdata->size, qtv ) != compdata->size )
			{
				fputs( "qtv_read_frame: Short read on compressed image data\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}
			
			image->imagedata = databuffer_create( size );
			if( image->imagedata == NULL )
				return 0;

			coder = video->imgcoder;

			rangecode_decompress( coder, compdata, image->imagedata, size );
			
			databuffer_free( compdata );
			
			
			if( image->tilecache != NULL )
			{
				if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
				{
					fputs( "qtv_read_frame: Short read on compressed index data size\n", stderr );
					if( qtv != stdin )
						fclose( qtv );
					return 0;
				}

				compdata = databuffer_create( size );
				if( compdata == NULL )
					return 0;

				compdata->size = size;

				if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
				{
					fputs( "qtv_read_frame: Short read on uncompressed index data size\n", stderr );
					if( qtv != stdin )
						fclose( qtv );
					return 0;
				}

				if( fread( compdata->data, 1, compdata->size, qtv ) != compdata->size )
				{
					fputs( "qtv_read_frame: Short read on compressed index data\n", stderr );
					if( qtv != stdin )
						fclose( qtv );
					return 0;
				}
			
				image->indexdata = databuffer_create( size );
				if( image->indexdata == NULL )
					return 0;

				coder = video->idxcoder;

				rangecode_decompress( coder, compdata, image->indexdata, size );
			
				databuffer_free( compdata );
			}
		}
		else
		{
			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "qtv_read_frame: Short read on command data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			image->commanddata = databuffer_create( size );
			if( image->commanddata == NULL )
				return 0;

			image->commanddata->size = size;
			if( fread( image->commanddata->data, 1, image->commanddata->size, qtv ) != image->commanddata->size )
			{
				fputs( "qtv_read_frame: Short read on command data\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}


			if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
			{
				fputs( "qtv_read_frame: Short read on image data size\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}

			image->imagedata = databuffer_create( size );
			if( image->imagedata == NULL )
				return 0;

			image->imagedata->size = size;
			if( fread( image->imagedata->data, 1, image->imagedata->size, qtv ) != image->imagedata->size )
			{
				fputs( "qtv_read_frame: Short read on image data\n", stderr );
				if( qtv != stdin )
					fclose( qtv );
				return 0;
			}
			
			
			if( image->tilecache != NULL )
			{
				if( fread( &size, sizeof( size ), 1, qtv ) != 1 )
				{
					fputs( "qtv_read_frame: Short read on index data size\n", stderr );
					if( qtv != stdin )
						fclose( qtv );
					return 0;
				}

				image->indexdata = databuffer_create( size );
				if( image->indexdata == NULL )
					return 0;

				image->indexdata->size = size;
				if( fread( image->indexdata->data, 1, image->indexdata->size, qtv ) != image->indexdata->size )
				{
					fputs( "qtv_read_frame: Short read on index data\n", stderr );
					if( qtv != stdin )
						fclose( qtv );
					return 0;
				}
			}
		}

		video->framenum++;

		return 1;
	}
	else
	{
		fputs( "read_qtv: no video opened\n", stderr );
		return 0;
	}
}

/*******************************************************************************
* Function to check wether another frame can be read from a qtv file           *
*                                                                              *
* video is a qtv structure as returned from qtv_read_header                    *
*                                                                              *
* Returns 0 on file end, 1 when more frames are available                      *
*******************************************************************************/
int qtv_can_read_frame( struct qtv *video )
{
	int tmp;

	if( video->has_index )
	{
		return (video->framenum < video->numframes);
	}
	else
	{
		tmp = getc( video->file );
		if( feof( video->file ) )
			return 0;
		else
			ungetc( tmp, video->file );

		return 1;
	}
}

/*******************************************************************************
* Function to write a qtv struct to a file                                     *
*                                                                              *
* video is a qtv structure as returned from qtv_create                         *
* filename is the file name of the new qtv file                                *
*                                                                              *
* Modifies video                                                               *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtv_write_header( struct qtv *video, char filename[] )
{
	FILE *qtv, *block;
	unsigned char version, flags;
	char blockname[256];

	if( filename == NULL )
	{
		if( video->is_qtw )
		{
			fputs( "qtv_write_header: Cannot write QTW to stdout\n", stderr );
			return 0;
		}
		else
		{
			qtv = stdout;
		}
	}
	else
	{
		if( strcmp( filename, "-" ) == 0 )
		{
			if( video->is_qtw )
			{
				fputs( "qtv_write_header: Cannot write QTW to stdout\n", stderr );
				return 0;
			}
			else
			{
				qtv = stdout;
			}
		}
		else
		{
			qtv = fopen( filename, "wb" );
		}
	}
	
	if( qtv != NULL )
	{
		video->file = qtv;

		if( video->is_qtw )
			fwrite( QTW_MAGIC, 1, 4, qtv );
		else
			fwrite( QTV_MAGIC, 1, 4, qtv );

		version = VERSION;
		
		flags = 0;
		flags |= video->has_index&0x01;
		flags |= (video->tilecache!=NULL) << 1;
		
		fwrite( &(version), sizeof( version ), 1, qtv );
		fwrite( &(video->width), sizeof( video->width ), 1, qtv );
		fwrite( &(video->height), sizeof( video->height ), 1, qtv );
		fwrite( &(video->framerate), sizeof( video->framerate ), 1, qtv );
		fwrite( &flags, sizeof( flags ), 1, qtv );

		if( video->tilecache != NULL )
		{
			fwrite( &(video->tilecache->size), sizeof( video->tilecache->size ), 1, qtv );
			fwrite( &(video->tilecache->blocksize), sizeof( video->tilecache->blocksize ), 1, qtv );
		}

		if( filename )
			video->filename = strdup( filename );
		else
			video->filename = NULL;

		if( video->is_qtw )
		{
			snprintf( blockname, 256, "%s.%06i", video->filename, video->blocknum );
		
			block = fopen( blockname, "wb" );
			if( block == NULL )
			{
				perror( "qtv_write_header: fopen" );
				return 0;
			}

			video->streamfile = block;
		}
		else
		{
			video->streamfile = NULL;
		}

		return 1;
	}
	else
	{
		perror( "write_qtv" );
		return 0;
	}
}

/*******************************************************************************
* Function to write a single frame to a qtv file                               *
*                                                                              *
* video is a qtv structure as returned from qtv_create                         *
* image is the frame to be written                                             *
* compress indicates wether the frame data should be compressed                *
*                                                                              *
* Modifies video                                                               *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtv_write_frame( struct qtv *video, struct qti *image, int compress )
{
	FILE * qtv;
	struct databuffer *compdata;
	struct rangecoder *coder;
	unsigned char flags;
	unsigned int size, offset;

	if( video->is_qtw )
		qtv = video->streamfile;
	else
		qtv = video->file;

	if( ( image->width != video->width ) || ( image->height != video->height ) )
	{
		fputs( "write_qtv: frame size mismatch\n", stderr );
		return 0;
	}

	if( qtv != NULL )
	{
		offset = ftell( qtv );

		flags = 0;
		flags |= image->transform & 0x03;
		flags |= ( compress & 0x01 ) << 2;
		flags |= ( image->colordiff & 0x03 ) << 3;
		flags |= ( image->keyframe & 0x01 ) << 7;
		
		fwrite( &(flags), sizeof( flags ), 1, qtv );
		fwrite( &(image->minsize), sizeof( image->minsize ), 1, qtv );
		fwrite( &(image->maxdepth), sizeof( image->maxdepth ), 1, qtv );

		databuffer_pad( image->commanddata );
		databuffer_pad( image->imagedata );

		size = 0;

		if( image->keyframe )
		{
			rangecoder_reset( video->cmdcoder );
			rangecoder_reset( video->imgcoder );
			
			if( image->tilecache != NULL )
				rangecoder_reset( video->idxcoder );
		}

		if( compress )
		{
			compdata = databuffer_create( image->commanddata->size );
			if( compdata == NULL )
				return 0;

			coder = video->cmdcoder;

			rangecode_compress( coder, image->commanddata, compdata );
			databuffer_pad( compdata );

			fwrite( &(compdata->size), sizeof( compdata->size ), 1, qtv );
			fwrite( &(image->commanddata->size), sizeof( image->commanddata->size ), 1, qtv );
			fwrite( compdata->data, 1, compdata->size, qtv );

			size += sizeof( compdata->size ) + sizeof( image->commanddata->size ) + compdata->size;
			
			databuffer_free( compdata );


			compdata = databuffer_create( image->imagedata->size / 2 + 1 );
			if( compdata == NULL )
				return 0;

			coder = video->imgcoder;
			rangecode_compress( coder, image->imagedata, compdata );
			databuffer_pad( compdata );

			fwrite( &(compdata->size), sizeof( compdata->size ), 1, qtv );
			fwrite( &(image->imagedata->size), sizeof( image->imagedata->size ), 1, qtv );
			fwrite( compdata->data, 1, compdata->size, qtv );
			
			size += sizeof( compdata->size ) + sizeof( image->imagedata->size ) + compdata->size;
			
			databuffer_free( compdata );
			
			if( image->tilecache != NULL )
			{
				compdata = databuffer_create( image->indexdata->size / 2 + 1 );
				if( compdata == NULL )
					return 0;

				coder = video->idxcoder;
				rangecode_compress( coder, image->indexdata, compdata );
				databuffer_pad( compdata );

				fwrite( &(compdata->size), sizeof( compdata->size ), 1, qtv );
				fwrite( &(image->indexdata->size), sizeof( image->indexdata->size ), 1, qtv );
				fwrite( compdata->data, 1, compdata->size, qtv );
			
				size += sizeof( compdata->size ) + sizeof( image->indexdata->size ) + compdata->size;
			
				databuffer_free( compdata );
			}
		}
		else
		{
			fwrite( &(image->commanddata->size), sizeof( image->commanddata->size ), 1, qtv );
			fwrite( image->commanddata->data, 1, image->commanddata->size, qtv );

			size += sizeof( image->commanddata->size ) + image->commanddata->size;


			fwrite( &(image->imagedata->size), sizeof( image->imagedata->size ), 1, qtv );
			fwrite( image->imagedata->data, 1, image->imagedata->size, qtv );
			
			size += sizeof( image->imagedata->size ) + image->imagedata->size;
			
			if( image->tilecache != NULL )
			{
				fwrite( &(image->indexdata->size), sizeof( image->indexdata->size ), 1, qtv );
				fwrite( image->indexdata->data, 1, image->indexdata->size, qtv );
			
				size += sizeof( image->indexdata->size ) + image->indexdata->size;
			}
		}

		if( ( video->has_index ) && ( image->keyframe ) )
		{
			video->index[video->idx_size].frame = video->numframes;
			video->index[video->idx_size].block = video->blocknum;
			video->index[video->idx_size].offset = offset;
			video->idx_size++;
			if( video->idx_size >= video->idx_datasize )
			{
				video->idx_datasize *= 2;
				video->index = realloc( video->index, sizeof( *video->index ) * video->idx_datasize );
				if( video->index == NULL )
				{
					perror( "qtv_write_frame: realloc" );
					return 0;
				}
			}
		}

		video->numframes++;
		video->framenum++;

		return size;
	}
	else
	{
		fputs( "write_qtv: no video opened\n", stderr );
		return 0;
	}
}

/*******************************************************************************
* Function to finish a block in a qtw file and start a new one                 *
*                                                                              *
* video is a qtv structure as returned from qtv_create                         *
*                                                                              *
* Modifies video                                                               *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtv_write_block( struct qtv *video )
{
	FILE *block;
	char blockname[256];

	if( !video->is_qtw )
	{
		fputs( "qtv_write_block: cannot switch blocks on non QTW video\n", stderr );
		return 0;
	}

	fclose( video->streamfile );

	video->blocknum++;
	video->numblocks++;

	snprintf( blockname, 256, "%s.%06i", video->filename, video->blocknum );
		
	block = fopen( blockname, "wb" );
	if( block == NULL )
	{
		perror( "qtv_write_block: fopen" );
		return 0;
	}

	video->streamfile = block;

	return 1;
}

/*******************************************************************************
* Function to create a new qtv structure for writing                           *
*                                                                              *
* video is a pointer to an uninitialized qtv struct                            *
* width is the width of the video                                              *
* height is the height of the video                                            *
* framerate is the frame rate of the video                                     *
* index indicates wether the video should have and index (1) or not (0)        *
* is_qtw indicates wether the video should be a qtw (1) or qtv(0) video        *
*                                                                              *
* Modifies video                                                               *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtv_create( struct qtv *video, int width, int height, int framerate, struct tilecache *cache, int index, int is_qtw )
{
	video->width = width;
	video->height = height;
	video->framerate = framerate;
	video->is_qtw = is_qtw;
	video->file = NULL;
	video->streamfile = NULL;
	video->filename = NULL;
	video->has_index = index || is_qtw;
	video->numframes = 0;
	video->framenum = 0;
	video->numblocks = 0;
	video->blocknum = 0;
	
	video->tilecache = cache;

	if( index )
	{
		video->idx_size = 0;
		video->idx_datasize = 256;
		video->index = malloc( sizeof( *video->index ) * video->idx_datasize );
		if( video->index == NULL )
		{
			perror( "qtv_create: malloc" );
			return 0;
		}
	}

	video->cmdcoder = rangecoder_create( 8, 1 );
	if( video->cmdcoder == NULL )
		return 0;

	video->imgcoder = rangecoder_create( 2, 8 );
	if( video->imgcoder == NULL )
		return 0;

	if( cache != NULL )
	{
		video->idxcoder = rangecoder_create( 2, 8 );
		if( video->idxcoder == NULL )
			return 0;
	}

	return 1;
}

/*******************************************************************************
* Function to write the index for a qtv file                                   *
*                                                                              *
* video is a qtv structure as returned from qtv_create                         *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtv_write_index( struct qtv *video )
{
	FILE *qtv;
	int i;
	long int size;

	if( ! video->has_index )
	{
		fputs( "qtv_write_index: video has no index\n", stderr );
		return 0;
	}

	qtv = video->file;

	if( video->is_qtw )
		video->numblocks++;

	fwrite( &(video->numframes), sizeof( video->numframes ), 1, qtv );
	if( video->is_qtw )
		fwrite( &(video->numblocks), sizeof( video->numblocks ), 1, qtv );
	fwrite( &(video->idx_size), sizeof( video->idx_size ), 1, qtv );
	
	size = sizeof( video->numframes ) + sizeof( video->idx_size );
	if( video->is_qtw )
		size += sizeof( video->numblocks );
	
	for( i=0; i<video->idx_size; i++ )
	{
		fwrite( &(video->index[i].frame), sizeof( video->index[i].frame ), 1, qtv );
		if( video->is_qtw )
			fwrite( &(video->index[i].block), sizeof( video->index[i].block ), 1, qtv );
		fwrite( &(video->index[i].offset), sizeof( video->index[i].offset ), 1, qtv );

		size += sizeof( video->index[i].frame ) + sizeof( video->index[i].offset );
		if( video->is_qtw )
			size += sizeof( video->index[i].block );
	}

	if( !video->is_qtw )
	{
		size += sizeof( size );
		fwrite( &size, sizeof( size ), 1, qtv );
	}
	
	return size;
}

/*******************************************************************************
* Function to seek in a qtv file with index                                    *
*                                                                              *
* video is a qtv structure as returned from qtv_read_header                    *
*                                                                              *
* Modifies video                                                               *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtv_seek( struct qtv *video, int frame )
{
	int i;
	char blockname[256];

	if( frame < 0 )
		frame = 0;

	if( video->has_index )
	{
		for( i=0; i<video->idx_size; i++ )
		{
			if( video->index[i].frame > frame )
				break;
		}

		i--;

		if( video->is_qtw )
		{
			if( video->blocknum != video->index[i].block )
			{
				fclose( video->streamfile );

				video->blocknum = video->index[i].block;

				snprintf( blockname, 256, "%s.%06i", video->filename, video->blocknum );

				video->streamfile = fopen( blockname, "rb" );
				if( video->streamfile == NULL )
				{
					perror( "qtv_seek: fopen" );
					return 0;
				}
			}

			video->framenum = video->index[i].frame;
			if( fseek( video->streamfile, video->index[i].offset, SEEK_SET ) == -1 )
			{
				perror( "qtv_seek: fseek" );
				return 0;
			}
		}
		else
		{
			video->framenum = video->index[i].frame;
			if( fseek( video->file, video->index[i].offset, SEEK_SET ) == -1 )
			{
				perror( "qtv_seek: fseek" );
				return 0;
			}
		}
	}
	else
	{
		fputs( "qtv_seek: video has no index\n", stderr );
		return 0;
	}
	
	return 1;
}

/*******************************************************************************
* Function to free the internal structures of a qtv struct                     *
*                                                                              *
* video is the qtv structure to be freed                                       *
*                                                                              *
* Modifies video                                                               *
*******************************************************************************/
void qtv_free( struct qtv *video )
{
	rangecoder_free( video->cmdcoder );
	video->cmdcoder = NULL;
	rangecoder_free( video->imgcoder );
	video->imgcoder = NULL;
	
	if( video->has_index )
		free( video->index );
	
	if( ( video->file ) && ( video->file != stdout ) )
	{
		fclose( video->file );
		video->file = NULL;
	}

	if( ( video->streamfile ) && ( video->streamfile != stdout ) )
	{
		fclose( video->streamfile );
		video->streamfile = NULL;
	}
	
	if( video->filename )
		free( video->filename );
}

