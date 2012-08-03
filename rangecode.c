/*
*    QTC: rangecode.c (c) 2011, 2012 50m30n3
*
*    The range coder in this file is based on the carry-less range coder
*    by Dmitry Subbotin and the imlementation found here:
*    http://www.sachingarg.com/compression/entropy_coding/64bit/
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

#include "databuffer.h"

#include "rangecode.h"


/* implement division with floating point instructions on architectures that
 * don't have an integer division instruction (such as ARMv6).  */

#ifdef SLOW_DIV
#  define DIV(x,y) (typeof(x))((double)(x) / (double)(y))
#else
#  define DIV(x,y) ((x)/(y))
#endif

unsigned const int maxrange = 0xFFFFFFFF;
unsigned const int top = 0x01<<24;
unsigned const int bottom = 0x01<<16;


/*******************************************************************************
* This function creates a new range coder using a markov chain model           *
*                                                                              *
* order specifies the order of the markov chain model used for prediciton      *
* bits specifies the number of bits per symbol                                 *
*                                                                              *
* Returns a new range coder struct                                             *
*******************************************************************************/
struct rangecoder *rangecoder_create( int order, int bits )
{
	struct rangecoder *coder;
	int i;
	int symbols, fsize, tsize;

	if( order < 0 )
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
	coder->bits = bits;
	
	symbols = 1<<bits;
	fsize = 1<<(bits*(order+1));
	tsize = 1<<(bits*order);

	coder->freqs = malloc( sizeof( *coder->freqs ) * fsize );
	if( coder->freqs == NULL )
	{
		perror( "rangecoder_create: malloc" );
		return NULL;
	}

	coder->totals = malloc( sizeof( *coder->freqs ) * tsize );
	if( coder->freqs == NULL )
	{
		perror( "rangecoder_create: malloc" );
		return NULL;
	}

	for( i=0; i<fsize; i++ )
		coder->freqs[i] = 1;
	
	for( i=0; i<tsize; i++ )
		coder->totals[i] = symbols;

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
	int i;
	int symbols, fsize, tsize;

	symbols = 1<<coder->bits;
	fsize = 1<<(coder->bits*(coder->order+1));
	tsize = 1<<(coder->bits*coder->order);

	for( i=0; i<fsize; i++ )
		coder->freqs[i] = 1;
	
	for( i=0; i<tsize; i++ )
		coder->totals[i] = symbols;
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
	free( coder->freqs );
	free( coder->totals );
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
	int *freqs, *totals;
	unsigned int count;
	int symbol;
	int i;
	int bits, symbols, idx, mask;
	int start, size, total;

	unsigned int low = 0x00;
	unsigned int range = maxrange;

	freqs = coder->freqs;
	totals = coder->totals;
	bits = coder->bits;
	symbols = 1<<bits;

	mask = ~((~0x00)<<(bits*(coder->order+1)));

	idx = 0x00;

	for( count=0; count<in->size*8; count+=bits )
	{
		if( bits == 8 )
			symbol = databuffer_get_byte( in );
		else
			symbol = databuffer_get_bits( in, bits );

		start = 0;
		for( i=0; i<symbol; i++ )
			start += freqs[idx+i];

		size = freqs[idx+symbol];
		total = totals[idx>>bits];

		range = DIV(range,total);
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

		freqs[idx+symbol] += 32;
		totals[idx>>bits] += 32;

		if( totals[idx>>bits] >= 0xFFFF )
		{
			totals[idx>>bits] = 0;
			for( i=0; i<symbols; i++ )
			{
				freqs[idx+i] /= 2;
				if( freqs[idx+i] == 0 )
					freqs[idx+i] = 1;
				totals[idx>>bits] += freqs[idx+i];
			}
		}

		idx = ((idx+symbol)<<bits)&mask;
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
	int *freqs, *totals;
	unsigned int count;
	int symbol;
	int i;
	int start, size, total, value;
	int bits, symbols, idx, mask;

	unsigned int low = 0x00;
	unsigned int range = maxrange;
	unsigned int code = 0x00;

	freqs = coder->freqs;
	totals = coder->totals;
	bits = coder->bits;
	symbols = 1<<bits;

	for( i=0; i<4; i++ )
	{
		code <<= 8;
		code |= databuffer_get_byte( in ) & 0xFF;
	}

	mask = ~((~0x00)<<(bits*(coder->order+1)));

	idx = 0x00;

	for( count=0; count<length*8; count+=bits )
	{
		total = totals[idx>>bits];

		range = DIV(range,total);

		value = (code - low) / range;

		i = 0;
		while( ( value >= 0 ) && ( i < symbols ) )
		{
			value -= freqs[idx+i];
			i++;
		}

		if( value >= 0 )
		{
			fputs( "rangecode_decompress: decompression error\n", stderr );
			return 0;
		}

		symbol = i-1;

		if( bits == 8 )
			databuffer_add_byte( symbol, out );
		else
			databuffer_add_bits( symbol, out, bits );

		start = 0;
		for( i=0; i<symbol; i++ )
			start += freqs[idx+i];

		size = freqs[idx+symbol];

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

		freqs[idx+symbol] += 32;
		totals[idx>>bits] += 32;

		if( totals[idx>>bits] >= 0xFFFF )
		{
			totals[idx>>bits] = 0;
			for( i=0; i<symbols; i++ )
			{
				freqs[idx+i] /= 2;
				if( freqs[idx+i] == 0 )
					freqs[idx+i] = 1;
				totals[idx>>bits] += freqs[idx+i];
			}
		}

		idx = ((idx+symbol)<<bits)&mask;
	}

	return 1;
}

