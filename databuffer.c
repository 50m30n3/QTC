#include <stdlib.h>
#include <stdio.h>

#include "databuffer.h"

struct databuffer *create_databuffer( unsigned int size )
{
	struct databuffer *buffer = malloc( sizeof( struct databuffer ) );
	if( buffer == NULL )
	{
		perror( "create_databuffer: malloc" );
		return NULL;
	}

	if( size <= 0 )
		size = 256;

	buffer->size = 0;
	buffer->bits = 0;

	buffer->pos = 0;
	buffer->bitpos = 8;

	buffer->datasize = size;
	buffer->data = malloc( sizeof( unsigned char ) * buffer->datasize );
	if( buffer->data == NULL )
	{
		perror( "create_databuffer: malloc" );
		free( buffer );
		return NULL;
	}
	
	buffer->data[ 0 ] = 0;
	buffer->wbuffer = 0;
	buffer->rbuffer = 0;
	
	return buffer;
}

void free_databuffer( struct databuffer *buffer )
{
	free( buffer->data );
	free( buffer );
}

int add_data( unsigned int data, struct databuffer *buffer, int bits )
{
	int i;

	for( i=0; i<bits; i++ )
	{
		buffer->wbuffer |= (data&0x01)<<buffer->bits++;
		data >>= 1;

		if( buffer->bits >= 8 )
		{
			buffer->data[ buffer->size++ ] = buffer->wbuffer;
			if( buffer->size >= buffer->datasize )
			{
				buffer->datasize *= 2;
				buffer->data = realloc( buffer->data, buffer->datasize );
				if( buffer->data == NULL )
				{
					perror( "add_data: realloc" );
					return 0;
				}
			}
			buffer->wbuffer = 0;
			buffer->bits = 0;
		}
	}
	
	return 1;
}

int add_data_byte( unsigned char data, struct databuffer *buffer )
{
	if( buffer->bits != 0 )
	{
		buffer->data[ buffer->size++ ] = buffer->wbuffer;
		if( buffer->size >= buffer->datasize )
		{
			buffer->datasize *= 2;
			buffer->data = realloc( buffer->data, buffer->datasize );
			if( buffer->data == NULL )
			{
				perror( "add_data_byte: realloc" );
				return 0;
			}
		}
		buffer->wbuffer = 0;
		buffer->bits = 0;
	}


	buffer->data[ buffer->size++ ] = data;
	if( buffer->size >= buffer->datasize )
	{
		buffer->datasize *= 2;
		buffer->data = realloc( buffer->data, buffer->datasize );
		if( buffer->data == NULL )
		{
			fprintf( stderr, "%i\n", buffer->datasize );
			perror( "add_data_byte: realloc" );
			return 0;
		}
	}
	
	return 1;
}

int pad_data( struct databuffer *buffer )
{
	if( buffer->bits != 0 )
	{
		buffer->data[ buffer->size++ ] = buffer->wbuffer;
		if( buffer->size >= buffer->datasize )
		{
			buffer->datasize *= 2;
			buffer->data = realloc( buffer->data, buffer->datasize );
			if( buffer->data == NULL )
			{
				perror( "pad_data: realloc" );
				return 0;
			}
		}
		buffer->wbuffer = 0;
		buffer->bits = 0;
	}
	
	return 1;
}

unsigned int get_data( struct databuffer *buffer, int bits )
{
	unsigned int data = 0;
	int i;
	
	for( i=0; i<bits; i++ )
	{
		if( buffer->bitpos >= 8 )
		{
			buffer->rbuffer = buffer->data[ buffer->pos++ ];
			buffer->bitpos = 0;
		}

		data |= ((buffer->rbuffer>>buffer->bitpos++)&0x01)<<i;
	}

	return data;
}

unsigned char get_data_byte( struct databuffer *buffer )
{
	unsigned char data;

	if( buffer->bitpos != 0 )
		buffer->bitpos = 0;

	data = buffer->data[ buffer->pos++ ];

	return data;
}

