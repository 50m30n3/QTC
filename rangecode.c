#include <stdlib.h>
#include <stdio.h>

#include "databuffer.h"

#include "rangecode.h"

/*******************************************************************************
* The range coder in this file is based on the carry-less range coder          *
* by Dmitry Subbotin and the imlementation found here:                         *
* http://www.sachingarg.com/compression/entropy_coding/64bit/                  *
*******************************************************************************/

unsigned const int maxrange = 0xFFFFFFFF;
unsigned const int top = 0x01<<24;
unsigned const int bottom = 0x01<<16;


/*******************************************************************************
* This function creates a new range coder using a zero or first order model    *
*                                                                              *
* order specifies the order of the markov chain model used for prediciton      *
*                                                                              *
* Returns a new range coder struct                                             *
*******************************************************************************/
struct rangecoder *rangecoder_create( int order )
{
	struct rangecoder *coder;
	int i, j;

	if( ( order < 0 ) || ( order > 1 ) )
	{
		perror( "rangecoder_create: out of order" );
		return NULL;
	}

	coder = malloc( sizeof( *coder ) );
	if( coder == NULL )
	{
		perror( "rangecoder_create: malloc" );
		return NULL;
	}

	coder->order = order;

	if( order == 0 )
	{
		coder->freq = malloc( sizeof( *coder->freq ) * 257 );
		if( coder->freq == NULL )
		{
			perror( "rangecoder_create: malloc" );
			return NULL;
		}

		coder->freq[0] = 256;
		for( i=0; i<256; i++ )
			coder->freq[i+1] = 1;
	}
	else
	{
		coder->freq = malloc( sizeof( *coder->freq ) * 257 * 256 );
		if( coder->freq == NULL )
		{
			perror( "rangecoder_create: malloc" );
			return NULL;
		}

		for( j=0; j<256; j++ )
		{
			coder->freq[j*257] = 256;

			for( i=0; i<256; i++ )
				coder->freq[i+1+j*257] = 1;
		}
	}

	return coder;
}

/*******************************************************************************
* This function resets the model of a range coder                              *
*                                                                              *
* coder is the range coder that contains the model to be reset                 *
*                                                                              *
* Modifies coder                                                               *
*******************************************************************************/
void rangecoder_reset( struct rangecoder *coder )
{
	int i, j;

	if( coder->order == 0 )
	{
		coder->freq[0] = 256;
		for( i=0; i<256; i++ )
			coder->freq[i+1] = 1;
	}
	else
	{
		for( j=0; j<256; j++ )
		{
			coder->freq[j*257] = 256;

			for( i=0; i<256; i++ )
				coder->freq[i+1+j*257] = 1;
		}
	}
}

/*******************************************************************************
* This function frees a range coder and its internal structures                *
*                                                                              *
* coder is the range coder to be freed                                         *
*                                                                              *
* Modifies coder                                                               *
*******************************************************************************/
void rangecoder_free( struct rangecoder *coder )
{
	free( coder->freq );
	free( coder );
}

/*******************************************************************************
* This function compresses a databuffer using a range coder                    *
*                                                                              *
* coder is the range coder to be used during compression                       *
* in contains the data to be compressed                                        *
* out is the databuffer that the compressed data will be written to            *
*                                                                              *
* Modifies coder, in and out                                                   *
*******************************************************************************/
int rangecode_compress( struct rangecoder *coder, struct databuffer *in, struct databuffer *out )
{
	int *freq;
	unsigned int count;
	int i, symbol, lastsym, idx;
	int start, size, total;

	unsigned int low = 0x00;
	unsigned int range = maxrange;

	freq = coder->freq;

	lastsym = 0x00;
	idx = 0x00;

	for( count=0; count<in->size; count++ )
	{
		symbol = databuffer_get_byte( in );

		start = 0;
		for( i=0; i<symbol; i++ )
			start += freq[i+1+idx];

		size = freq[symbol+1+idx];
		total = freq[idx];

		range /= total;
		low += start * range;
		range *= size;

		while( ( (low^(low+range)) < top ) || ( range < bottom ) )
		{
			if( ( range < bottom ) && ( ( (low^(low+range)) >= top ) ) )
				range = (-low)&(bottom-1);

			databuffer_add_byte( ( low >> 24 ) & 0xFF, out );
			low <<= 8;
			range <<= 8;
		}

		freq[symbol+1+idx] += 32;
		freq[idx] += 32;

		if( freq[idx] >= 0xFFFF )
		{
			freq[idx] = 0;
			for( i=0; i<256; i++ )
			{
				freq[i+1+idx] /= 2;
				if( freq[i+1+idx] == 0 )
					freq[i+1+idx] = 1;
				freq[idx] += freq[i+1+idx];
			}
		}

		if( coder->order > 0 )
		{
			lastsym = symbol;
			idx = lastsym*257;
		}
	}

	for( i=0; i<4; i++ )
	{
		databuffer_add_byte( ( low >> 24 ) & 0xFF, out );
		low <<= 8;
	}

	return 1;
}

/*******************************************************************************
* This function decompresses a databuffer using a range coder                  *
*                                                                              *
* coder is the range coder to be used during compression                       *
* in contains the data to be decompressed                                      *
* out is the databuffer that the decompressed data will be written to          *
* length is the uncompressed data length                                       *
*                                                                              *
* Modifies coder, in and out                                                   *
*******************************************************************************/

int rangecode_decompress( struct rangecoder *coder, struct databuffer *in, struct databuffer *out, unsigned int length )
{
	int *freq;
	unsigned int count;
	int i, symbol, lastsym, idx;
	int start, size, total, value;

	unsigned int low = 0x00;
	unsigned int range = maxrange;
	unsigned int code = 0x00;

	freq = coder->freq;

	for( i=0; i<4; i++ )
	{
		code <<= 8;
		code |= databuffer_get_byte( in ) & 0xFF;
	}

	lastsym = 0x00;
	idx = 0x00;

	for( count=0; count<length; count++ )
	{
		total = freq[idx];

		value = ( code - low ) / ( range / total );

		i = 0;
		while( ( value >= 0 ) && ( i < 256 ) )
		{
			value -= freq[i+1+idx];
			i++;
		}

		if( value >= 0 )
		{
			fputs( "rangecode_decompress: decompression error\n", stderr );
			return 0;
		}

		symbol = i-1;

		databuffer_add_byte( symbol & 0xFF, out );

		start = 0;
		for( i=0; i<symbol; i++ )
			start += freq[i+1+idx];

		size = freq[symbol+1+idx];

		range /= total;
		low += start * range;
		range *= size;


		while( ( (low^(low+range)) < top ) || ( range < bottom ) )
		{
			if( ( range < bottom ) && ( ( (low^(low+range)) >= top ) ) )
				range = (-low)&(bottom-1);

			code <<= 8;
			code |= databuffer_get_byte( in ) & 0xFF;

			low <<= 8;
			range <<= 8;
		}

		freq[symbol+1+idx] += 32;
		freq[idx] += 32;

		if( freq[idx] >= 0xFFFF )
		{
			freq[idx] = 0;
			for( i=0; i<256; i++ )
			{
				freq[i+1+idx] /= 2;
				if( freq[i+1+idx] == 0 )
					freq[i+1+idx] = 1;
				freq[idx] += freq[i+1+idx];
			}
		}

		if( coder->order > 0 )
		{
			lastsym = symbol;
			idx = lastsym*257;
		}
	}

	return 1;
}

