#include <stdlib.h>
#include <stdio.h>

#include "databuffer.h"

/*******************************************************************************
* Function to create a new databuffer                                          *
*                                                                              *
* size is the initial size of the databuffer in bytes                          *
*                                                                              *
* Returns a new databuffer or NULL on failure                                  *
*******************************************************************************/
struct databuffer *databuffer_create( unsigned int size )
{
	struct databuffer *buffer = malloc( sizeof( struct databuffer ) );
	if( buffer == NULL )
	{
		perror( "databuffer_create: malloc" );
		return NULL;
	}

	buffer->size = 0;
	buffer->bits = 0;

	buffer->pos = 0;
	buffer->bitpos = 8;

	buffer->datasize = size;
	buffer->data = malloc( sizeof( unsigned char ) * buffer->datasize );
	if( buffer->data == NULL )
	{
		perror( "databuffer_create: malloc" );
		free( buffer );
		return NULL;
	}
	
	buffer->data[ 0 ] = 0;
	buffer->wbuffer = 0;
	buffer->rbuffer = 0;
	
	return buffer;
}

/*******************************************************************************
* Function to free the internal structures of a databuffer                     *
*                                                                              *
* buffer is the databuffer to free                                             *
*                                                                              *
* Modifies databuffer                                                          *
*******************************************************************************/
void databuffer_free( struct databuffer *buffer )
{
	free( buffer->data );
	free( buffer );
}

/*******************************************************************************
* Function to add a number of bits to a databuffer                             *
*                                                                              *
* data is the data to be added                                                 *
* buffer is the databuffer to add to                                           *
* bits is the number of bits to take from data and add                         *
*                                                                              *
* Modifies databuffer                                                          *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int databuffer_add_bits( unsigned int data, struct databuffer *buffer, int bits )
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
					perror( "databuffer_add_bits: realloc" );
					return 0;
				}
			}
			buffer->wbuffer = 0;
			buffer->bits = 0;
		}
	}
	
	return 1;
}

/*******************************************************************************
* Function to add a single byte to a databuffer                                *
*                                                                              *
* data is the data to be added                                                 *
* buffer is the databuffer to add to                                           *
*                                                                              *
* Modifies databuffer                                                          *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int databuffer_add_byte( unsigned char data, struct databuffer *buffer )
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
				perror( "databuffer_add_byte: realloc" );
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
			perror( "databuffer_add_byte: realloc" );
			return 0;
		}
	}
	
	return 1;
}

/*******************************************************************************
* Function to pad the data in a databuffer to a full byte                      *
*                                                                              *
* buffer is the databuffer to add to                                           *
*                                                                              *
* Modifies databuffer                                                          *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int databuffer_pad( struct databuffer *buffer )
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
				perror( "databuffer_pad: realloc" );
				return 0;
			}
		}
		buffer->wbuffer = 0;
		buffer->bits = 0;
	}
	
	return 1;
}

/*******************************************************************************
* Function to get a number of bits from a databuffer                           *
*                                                                              *
* buffer is the databuffer to add to                                           *
*                                                                              *
* Modifies databuffer                                                          *
*                                                                              *
* Returns the bits in an unsigned int                                          *
*******************************************************************************/
unsigned int databuffer_get_bits( struct databuffer *buffer, int bits )
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

/*******************************************************************************
* Function to get a single byte from a databuffer                              *
*                                                                              *
* buffer is the databuffer to add to                                           *
*                                                                              *
* Modifies databuffer                                                          *
*                                                                              *
* Returns the byte as unsigned char                                            *
*******************************************************************************/
unsigned char databuffer_get_byte( struct databuffer *buffer )
{
	unsigned char data;

	if( buffer->bitpos != 0 )
	{
		buffer->rbuffer = buffer->data[ buffer->pos++ ];
	}

	data = buffer->rbuffer;
	buffer->bitpos = 8;

	return data;
}

