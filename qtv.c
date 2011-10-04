#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "databuffer.h"
#include "rangecode.h"
#include "qti.h"

#include "qtv.h"

#define FILEVERSION "QTV1"
#define VERSION 3

int qtv_read_header( struct qtv *video, char filename[] )
{
	FILE * qtv;
	int i;
	char header[4];
	int width, height, framerate;
	unsigned char version, flags;
	int numframes, idx_size, frame;
	unsigned int offset, idx_offset;

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
			fputs( "qtv_read_header: Short read on header\n", stderr );
			if( qtv != stdin )
				fclose( qtv );
			return 0;
		}

		if( strncmp( header, FILEVERSION, 4 ) != 0 )
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
		video->width = width;
		video->height = height;
		video->framerate = framerate;
		video->has_index = flags&0x01;
		
		if( video->has_index )
		{
			if( qtv == stdin )
			{
				fputs( "qtv_read_header: Cannot read indexed video from stdin\n", stderr );
				return 0;
			}

			if( fseek( qtv, -sizeof( idx_offset ), SEEK_END ) == -1 )
			{
				fputs( "qtv_read_header: fseek:\n", stderr );
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
				fputs( "qtv_read_header: fseek:\n", stderr );
				fclose( qtv );
				return 0;
			}

			if( ( fread( &numframes, sizeof( numframes ), 1, qtv ) != 1 ) ||
				( fread( &idx_size, sizeof( idx_size ), 1, qtv ) != 1 ) )
			{
				fputs( "qtv_read_header: Cannot read index header\n", stderr );
				fclose( qtv );
				return 0;
			}

			video->numframes = numframes;
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
				if( ( fread( &frame, sizeof( frame ), 1, qtv ) != 1 ) ||
					( fread( &offset, sizeof( offset ), 1, qtv ) != 1 ) )
				{
					fputs( "qtv_read_header: Cannot read index entry\n", stderr );
					fclose( qtv );
					return 0;
				}

				video->index[i].frame = frame;
				video->index[i].offset = offset;
			}

			if( fseek( qtv, 18, SEEK_SET ) == -1 )
			{
				fputs( "qtv_read_header: fseek:\n", stderr );
				fclose( qtv );
				return 0;
			}
		}

		video->cmdcoder = rangecoder_create( 0 );
		if( video->cmdcoder == NULL )
			return 0;

		video->imgcoder = rangecoder_create( 1 );
		if( video->imgcoder == NULL )
			return 0;
		
		return 1;
	}
	else
	{
		perror( "qtv_read_header" );
		return 0;
	}
}

int qtv_read_frame( struct qtv *video, struct qti *image, int *keyframe )
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
		*keyframe = ( ( flags & (0x01<<3) ) >> 3 ) & 0x01;

		if( compress )
		{
			if( *keyframe )
			{
				rangecoder_reset( video->cmdcoder );
				rangecoder_reset( video->imgcoder );
			}

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

int qtv_write_header( struct qtv *video, char filename[] )
{
	FILE * qtv;
	unsigned char version, flags;

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
		
		flags = video->has_index&0x01;
		
		fwrite( &(version), sizeof( version ), 1, qtv );
		fwrite( &(video->width), sizeof( video->width ), 1, qtv );
		fwrite( &(video->height), sizeof( video->height ), 1, qtv );
		fwrite( &(video->framerate), sizeof( video->framerate ), 1, qtv );
		fwrite( &flags, sizeof( flags ), 1, qtv );

		return 1;
	}
	else
	{
		perror( "write_qtv" );
		return 0;
	}
}

int qtv_write_frame( struct qtv *video, struct qti *image, int compress, int keyframe )
{
	FILE * qtv;
	struct databuffer *compdata;
	struct rangecoder *coder;
	unsigned char flags;
	unsigned int size, offset;

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
		flags |= ( keyframe & 0x01 ) << 3;
		
		fwrite( &(flags), sizeof( flags ), 1, qtv );
		fwrite( &(image->minsize), sizeof( image->minsize ), 1, qtv );
		fwrite( &(image->maxdepth), sizeof( image->maxdepth ), 1, qtv );

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

		if( ( video->has_index ) && ( keyframe ) )
		{
			video->index[video->idx_size].frame = video->numframes;
			video->index[video->idx_size].offset = offset;
			video->idx_size++;
			if( video->idx_size > video->idx_datasize )
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

int qtv_create( int width, int height, int framerate, int index, struct qtv *video )
{
	video->width = width;
	video->height = height;
	video->framerate = framerate;
	video->file = NULL;
	video->has_index = index;
	video->numframes = 0;
	video->framenum = 0;

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

	video->cmdcoder = rangecoder_create( 0 );
	if( video->cmdcoder == NULL )
		return 0;

	video->imgcoder = rangecoder_create( 1 );
	if( video->imgcoder == NULL )
		return 0;
	
	return 1;
}

int qtv_write_index( struct qtv *video )
{
	FILE *qtv;
	int i;
	unsigned int size;

	if( ! video->has_index )
		return 0;

	qtv = video->file;

	size = 0;
	
	fwrite( &(video->numframes), sizeof( video->numframes ), 1, qtv );
	fwrite( &(video->idx_size), sizeof( video->idx_size ), 1, qtv );
	
	size += sizeof( video->numframes ) + sizeof( video->idx_size );
	
	for( i=0; i<video->idx_size; i++ )
	{
		fwrite( &(video->index[i].frame), sizeof( video->index[i].frame ), 1, qtv );
		fwrite( &(video->index[i].offset), sizeof( video->index[i].offset ), 1, qtv );
		size += sizeof( video->index[i].frame ) + sizeof( video->index[i].offset );
	}
	
	size += sizeof( size );

	fwrite( &size, sizeof( size ), 1, qtv );
	
	return size;
}

void qtv_seek( struct qtv *video, int frame )
{
	int i;
	
	if( video->has_index )
	{
		for( i=0; i<video->idx_size; i++ )
		{
			if( video->index[i].frame >= frame )
				break;
			
			video->framenum = i;
			fseek( video->file, video->index[i].offset, SEEK_SET );
		}
	}
	else
	{
		video->framenum = 0;
		fseek( video->file, 18, SEEK_SET );
	}
}

void qtv_free( struct qtv *video )
{
	rangecoder_free( video->cmdcoder );
	video->cmdcoder = NULL;
	rangecoder_free( video->imgcoder );
	video->imgcoder = NULL;
	
	if( video->has_index )
		free( video->index );
	
	if( ( video->file ) && ( video->file != stdout ) )
		fclose( video->file );
}

