#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "databuffer.h"
#include "rangecode.h"
#include "qti.h"

#include "qtw.h"

#define FILEVERSION "QTW1"
#define VERSION 2

int qtw_read_header( struct qtw *video, char filename[] )
{
	FILE *qtw, *block;
	int i;
	char header[4];
	int width, height, framerate;
	unsigned char version, flags;
	int numframes, idx_size, numblocks, frame, blocknum;
	long int offset;
	char blockname[256];

	if( filename == NULL )
	{
		fputs( "qtw_read_header: Cannot read QTW from stdin\n", stderr );
		return 0;
	}
	else
	{
		if( strcmp( filename, "-" ) == 0 )
		{
			fputs( "qtw_read_header: Cannot read QTW from stdin\n", stderr );
			return 0;
		}
		else
		{
			qtw = fopen( filename, "rb" );
		}
	}
	
	if( qtw != NULL )
	{
		video->file = qtw;

		if( fread( header, 1, 4, qtw ) != 4 )
		{
			fputs( "qtw_read_header: Short read on header\n", stderr );
			fclose( qtw );
			return 0;
		}

		if( strncmp( header, FILEVERSION, 4 ) != 0 )
		{
			fputs( "qtw_read_header: Invalid header\n", stderr );
			fclose( qtw );
			return 0;
		}

		if( ( fread( &version, sizeof( version ), 1, qtw ) != 1 ) ||
			( fread( &width, sizeof( width ), 1, qtw ) != 1 ) ||
			( fread( &height, sizeof( height ), 1, qtw ) != 1 ) ||
			( fread( &framerate, sizeof( framerate ), 1, qtw ) != 1 ) ||
			( fread( &flags, sizeof( flags ), 1, qtw ) != 1 ) )
		{
			fputs( "qtw_read_header: Short read on image header\n", stderr );
			fclose( qtw );
			return 0;
		}

		if( version != VERSION )
		{
			fputs( "qtw_read_header: Wrong version\n", stderr );
			fclose( qtw );
			return 0;
		}

		video->framenum = 0;
		video->numframes = 0;
		video->blocknum = 0;
		video->numblocks = 0;
		video->width = width;
		video->height = height;
		video->framerate = framerate;

		if( ( fread( &numframes, sizeof( numframes ), 1, qtw ) != 1 ) ||
			( fread( &numblocks, sizeof( numblocks ), 1, qtw ) != 1 ) ||
			( fread( &idx_size, sizeof( idx_size ), 1, qtw ) != 1 ) )
		{
			fputs( "qtw_read_header: Cannot read index header\n", stderr );
			fclose( qtw );
			return 0;
		}

		video->numframes = numframes;
		video->numblocks = numblocks;
		video->idx_size = idx_size;

		video->idx_datasize = video->idx_size;
		video->index = malloc( sizeof( *video->index ) * video->idx_datasize );
		if( video->index == NULL )
		{
			perror( "qtw_read_header: malloc" );
			fclose( qtw );
			return 0;
		}

		for( i=0; i<video->idx_size; i++ )
		{
			if( ( fread( &frame, sizeof( frame ), 1, qtw ) != 1 ) ||
				( fread( &blocknum, sizeof( blocknum ), 1, qtw ) != 1 ) ||
				( fread( &offset, sizeof( offset ), 1, qtw ) != 1 ) )
			{
				fputs( "qtw_read_header: Cannot read index entry\n", stderr );
				fclose( qtw );
				return 0;
			}

			video->index[i].frame = frame;
			video->index[i].block = blocknum;
			video->index[i].offset = offset;
		}

		video->cmdcoder = rangecoder_create( 0 );
		if( video->cmdcoder == NULL )
			return 0;

		video->imgcoder = rangecoder_create( 1 );
		if( video->imgcoder == NULL )
			return 0;

		video->basename = strdup( filename );
		
		snprintf( blockname, 256, "%s.%06i", video->basename, video->blocknum );
		
		block = fopen( blockname, "rb" );
		if( block == NULL )
		{
			perror( "qtw_read_header: fopen" );
			return 0;
		}

		video->blockfile = block;

		return 1;
	}
	else
	{
		perror( "qtw_read_header" );
		return 0;
	}
}

int qtw_read_frame( struct qtw *video, struct qti *image, int *keyframe )
{
	FILE * qtw;
	struct databuffer *compdata;
	struct rangecoder *coder;
	int minsize, maxdepth;
	int compress;
	int tmp;
	unsigned char flags;
	unsigned int size;
	char blockname[256];

	qtw = video->blockfile;
	
	if( qtw != NULL )
	{
		if( ( fread( &flags, sizeof( flags ), 1, qtw ) != 1 ) ||
			( fread( &minsize, sizeof( minsize ), 1, qtw ) != 1 ) ||
			( fread( &maxdepth, sizeof( maxdepth ), 1, qtw ) != 1 ) )
		{
			fputs( "qtw_read_frame: Short read on image header\n", stderr );
			if( qtw != stdin )
				fclose( qtw );
			return 0;
		}

		image->width = video->width;
		image->height = video->height;
		image->minsize = minsize;
		image->maxdepth = maxdepth;
		image->transform = flags&0x03;
		compress = ( ( flags & (0x01<<2) ) >> 2 ) & 0x01;
		*keyframe = ( ( flags & (0x01<<7) ) >> 7 ) & 0x01;

		if( compress )
		{
			if( *keyframe )
			{
				rangecoder_reset( video->cmdcoder );
				rangecoder_reset( video->imgcoder );
			}

			if( fread( &size, sizeof( size ), 1, qtw ) != 1 )
			{
				fputs( "qtw_read_frame: Short read on compressed command data size\n", stderr );
				if( qtw != stdin )
					fclose( qtw );
				return 0;
			}

			compdata = databuffer_create( size );
			if( compdata == NULL )
				return 0;

			compdata->size = size;

			if( fread( &size, sizeof( size ), 1, qtw ) != 1 )
			{
				fputs( "qtw_read_frame: Short read on uncompressed command data size\n", stderr );
				if( qtw != stdin )
					fclose( qtw );
				return 0;
			}

			if( fread( compdata->data, 1, compdata->size, qtw ) != compdata->size )
			{
				fputs( "qtw_read_frame: Short read on compressed command data\n", stderr );
				if( qtw != stdin )
					fclose( qtw );
				return 0;
			}
			
			image->commanddata = databuffer_create( size );
			if( image->commanddata == NULL )
				return 0;

			coder = video->cmdcoder;

			rangecode_decompress( coder, compdata, image->commanddata, size );
			
			databuffer_free( compdata );


			if( fread( &size, sizeof( size ), 1, qtw ) != 1 )
			{
				fputs( "qtw_read_frame: Short read on compressed image data size\n", stderr );
				if( qtw != stdin )
					fclose( qtw );
				return 0;
			}

			compdata = databuffer_create( size );
			if( compdata == NULL )
				return 0;

			compdata->size = size;

			if( fread( &size, sizeof( size ), 1, qtw ) != 1 )
			{
				fputs( "qtw_read_frame: Short read on uncompressed image data size\n", stderr );
				if( qtw != stdin )
					fclose( qtw );
				return 0;
			}

			if( fread( compdata->data, 1, compdata->size, qtw ) != compdata->size )
			{
				fputs( "qtw_read_frame: Short read on compressed image data\n", stderr );
				if( qtw != stdin )
					fclose( qtw );
				return 0;
			}
			
			image->imagedata = databuffer_create( size );
			if( image->imagedata == NULL )
				return 0;

			coder = video->imgcoder;

			rangecode_decompress( coder, compdata, image->imagedata, size );
			
			databuffer_free( compdata );
		}
		else
		{
			if( fread( &size, sizeof( size ), 1, qtw ) != 1 )
			{
				fputs( "qtw_read_frame: Short read on command data size\n", stderr );
				if( qtw != stdin )
					fclose( qtw );
				return 0;
			}

			image->commanddata = databuffer_create( size );
			if( image->commanddata == NULL )
				return 0;

			image->commanddata->size = size;
			if( fread( image->commanddata->data, 1, image->commanddata->size, qtw ) != image->commanddata->size )
			{
				fputs( "qtw_read_frame: Short read on command data\n", stderr );
				if( qtw != stdin )
					fclose( qtw );
				return 0;
			}


			if( fread( &size, sizeof( size ), 1, qtw ) != 1 )
			{
				fputs( "qtw_read_frame: Short read on image data size\n", stderr );
				if( qtw != stdin )
					fclose( qtw );
				return 0;
			}

			image->imagedata = databuffer_create( size );
			if( image->imagedata == NULL )
				return 0;

			image->imagedata->size = size;
			if( fread( image->imagedata->data, 1, image->imagedata->size, qtw ) != image->imagedata->size )
			{
				fputs( "qtw_read_frame: Short read on image data\n", stderr );
				if( qtw != stdin )
					fclose( qtw );
				return 0;
			}
		}

		video->framenum++;

		tmp = getc( qtw );
		if( feof( qtw ) )
		{
			fclose( qtw );
			
			video->blocknum++;
			
			snprintf( blockname, 256, "%s.%06i", video->basename, video->blocknum );
		
			qtw = fopen( blockname, "rb" );
			if( qtw == NULL )
			{
				perror( "qtw_read_frame: fopen" );
				return 0;
			}

			video->blockfile = qtw;
		}
		else
		{
			ungetc( tmp, qtw );
		}

		return 1;
	}
	else
	{
		fputs( "read_qtw: no video opened\n", stderr );
		return 0;
	}
}

int qtw_can_read_frame( struct qtw *video )
{
	return (video->framenum < video->numframes);
}

int qtw_write_header( struct qtw *video, char filename[] )
{
	FILE *qtw, *block;
	unsigned char version, flags;
	char blockname[256];

	if( filename == NULL )
	{
		fputs( "qtw_write_header: Cannot write QTW to stdout\n", stderr );
		return 0;
	}
	else
	{
		if( strcmp( filename, "-" ) == 0 )
		{
			fputs( "qtw_write_header: Cannot write QTW to stdout\n", stderr );
			return 0;
		}
		else
		{
			qtw = fopen( filename, "wb" );
		}
	}
	
	if( qtw != NULL )
	{
		video->file = qtw;

		fwrite( FILEVERSION, 1, 4, qtw );

		version = VERSION;
		
		flags = 0;
		
		fwrite( &(version), sizeof( version ), 1, qtw );
		fwrite( &(video->width), sizeof( video->width ), 1, qtw );
		fwrite( &(video->height), sizeof( video->height ), 1, qtw );
		fwrite( &(video->framerate), sizeof( video->framerate ), 1, qtw );
		fwrite( &flags, sizeof( flags ), 1, qtw );

		video->basename = strdup( filename );
		
		snprintf( blockname, 256, "%s.%06i", video->basename, video->blocknum );
		
		block = fopen( blockname, "wb" );
		if( block == NULL )
		{
			perror( "qtw_write_header: fopen" );
			return 0;
		}

		video->blockfile = block;

		return 1;
	}
	else
	{
		perror( "write_qtw" );
		return 0;
	}
}

int qtw_write_frame( struct qtw *video, struct qti *image, int compress, int keyframe )
{
	FILE * qtw;
	struct databuffer *compdata;
	struct rangecoder *coder;
	unsigned char flags;
	unsigned int size, offset;

	qtw = video->blockfile;

	if( ( image->width != video->width ) || ( image->height != video->height ) )
	{
		fputs( "write_qtw: frame size mismatch\n", stderr );
		return 0;
	}

	if( qtw != NULL )
	{
		offset = ftell( qtw );

		flags = 0;
		flags |= image->transform & 0x03;
		flags |= ( compress & 0x01 ) << 2;
		flags |= ( keyframe & 0x01 ) << 7;
		
		fwrite( &(flags), sizeof( flags ), 1, qtw );
		fwrite( &(image->minsize), sizeof( image->minsize ), 1, qtw );
		fwrite( &(image->maxdepth), sizeof( image->maxdepth ), 1, qtw );

		databuffer_pad( image->commanddata );
		databuffer_pad( image->imagedata );

		size = 0;

		if( compress )
		{
			if( keyframe )
			{
				rangecoder_reset( video->cmdcoder );
				rangecoder_reset( video->imgcoder );
			}

			compdata = databuffer_create( image->commanddata->size );
			if( compdata == NULL )
				return 0;

			coder = video->cmdcoder;

			rangecode_compress( coder, image->commanddata, compdata );
			databuffer_pad( compdata );

			fwrite( &(compdata->size), sizeof( compdata->size ), 1, qtw );
			fwrite( &(image->commanddata->size), sizeof( image->commanddata->size ), 1, qtw );
			fwrite( compdata->data, 1, compdata->size, qtw );

			size += sizeof( compdata->size ) + sizeof( image->commanddata->size ) + compdata->size;
			
			databuffer_free( compdata );


			compdata = databuffer_create( image->imagedata->size / 2 + 1 );
			if( compdata == NULL )
				return 0;

			coder = video->imgcoder;
			rangecode_compress( coder, image->imagedata, compdata );
			databuffer_pad( compdata );

			fwrite( &(compdata->size), sizeof( compdata->size ), 1, qtw );
			fwrite( &(image->imagedata->size), sizeof( image->imagedata->size ), 1, qtw );
			fwrite( compdata->data, 1, compdata->size, qtw );
			
			size += sizeof( compdata->size ) + sizeof( image->imagedata->size ) + compdata->size;
			
			databuffer_free( compdata );
		}
		else
		{
			fwrite( &(image->commanddata->size), sizeof( image->commanddata->size ), 1, qtw );
			fwrite( image->commanddata->data, 1, image->commanddata->size, qtw );

			size += sizeof( image->commanddata->size ) + image->commanddata->size;


			fwrite( &(image->imagedata->size), sizeof( image->imagedata->size ), 1, qtw );
			fwrite( image->imagedata->data, 1, image->imagedata->size, qtw );
			
			size += sizeof( image->imagedata->size ) + image->imagedata->size;
		}

		if( keyframe )
		{
			video->index[video->idx_size].frame = video->numframes;
			video->index[video->idx_size].block = video->blocknum;
			video->index[video->idx_size].offset = offset;
			video->idx_size++;
			if( video->idx_size > video->idx_datasize )
			{
				video->idx_datasize *= 2;
				video->index = realloc( video->index, sizeof( *video->index ) * video->idx_datasize );
				if( video->index == NULL )
				{
					perror( "qtw_write_frame: realloc" );
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
		fputs( "write_qtw: no video opened\n", stderr );
		return 0;
	}
}

int qtw_write_block( struct qtw *video )
{
	FILE *block;
	char blockname[256];

	fclose( video->blockfile );

	video->blocknum++;
	video->numblocks++;

	snprintf( blockname, 256, "%s.%06i", video->basename, video->blocknum );
		
	block = fopen( blockname, "wb" );
	if( block == NULL )
	{
		perror( "qtw_block: fopen" );
		return 0;
	}

	video->blockfile = block;

	return 1;
}

int qtw_create( int width, int height, int framerate, struct qtw *video )
{
	video->width = width;
	video->height = height;
	video->framerate = framerate;
	video->file = NULL;
	video->numframes = 0;
	video->framenum = 0;
	video->numblocks = 0;
	video->blocknum = 0;

	video->idx_size = 0;
	video->idx_datasize = 256;
	video->index = malloc( sizeof( *video->index ) * video->idx_datasize );
	if( video->index == NULL )
	{
		perror( "qtw_create: malloc" );
		return 0;
	}

	video->cmdcoder = rangecoder_create( 0 );
	if( video->cmdcoder == NULL )
		return 0;

	video->imgcoder = rangecoder_create( 1 );
	if( video->imgcoder == NULL )
		return 0;
	
	return 1;
}

int qtw_write_index( struct qtw *video )
{
	FILE *qtw;
	int i;
	long int size;

	qtw = video->file;

	video->numblocks++;

	size = 0;
	
	fwrite( &(video->numframes), sizeof( video->numframes ), 1, qtw );
	fwrite( &(video->numblocks), sizeof( video->numblocks ), 1, qtw );
	fwrite( &(video->idx_size), sizeof( video->idx_size ), 1, qtw );
	
	size += sizeof( video->numframes ) + sizeof( video->numblocks ) + sizeof( video->idx_size );
	
	for( i=0; i<video->idx_size; i++ )
	{
		fwrite( &(video->index[i].frame), sizeof( video->index[i].frame ), 1, qtw );
		fwrite( &(video->index[i].block), sizeof( video->index[i].block ), 1, qtw );
		fwrite( &(video->index[i].offset), sizeof( video->index[i].offset ), 1, qtw );
		size += sizeof( video->index[i].frame ) + sizeof( video->index[i].block ) + sizeof( video->index[i].offset );
	}
	
	return size;
}

int qtw_seek( struct qtw *video, int frame )
{
	int i;
	char blockname[256];
	
	for( i=0; i<video->idx_size; i++ )
	{
		if( video->index[i].frame > frame )
			break;
	}

	i--;

	if( video->blocknum != video->index[i].block )
	{
		fclose( video->blockfile );

		video->blocknum = video->index[i].block;

		snprintf( blockname, 256, "%s.%06i", video->basename, video->blocknum );

		video->blockfile = fopen( blockname, "rb" );
		if( video->blockfile == NULL )
		{
			perror( "qtw_seek: fopen" );
			return 0;
		}
	}

	video->framenum = video->index[i].frame;
	if( fseek( video->blockfile, video->index[i].offset, SEEK_SET ) == -1 )
	{
		perror( "qtw_seek: fseek" );
		return 0;
	}
	
	return 1;
}

void qtw_free( struct qtw *video )
{
	rangecoder_free( video->cmdcoder );
	video->cmdcoder = NULL;
	rangecoder_free( video->imgcoder );
	video->imgcoder = NULL;
	
	free( video->index );
	
	if( ( video->file ) && ( video->file != stdout ) )
		fclose( video->file );

	if( video->blockfile )
		fclose( video->blockfile );
	
	free( video->basename );
}

