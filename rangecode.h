/*
*    QTC: rangecode.h (c) 2011, 2012 50m30n3
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

#ifndef RANGECODE_H
#define RANGECODE_H

/*******************************************************************************
* Structure that holds all the data associated with a range coder              *
*                                                                              *
* order is the order of the markov chain used for prediction                   *
* bits is the bits per symbol used by the range coder                          *
* freqs and totals contain the model data                                      *
*******************************************************************************/
struct rangecoder
{
	int order;
	int bits;
	int *freqs;
	int *totals;
};

extern struct rangecoder *rangecoder_create( int order, int bits );
extern void rangecoder_reset( struct rangecoder *coder );
extern void rangecoder_free( struct rangecoder *coder );
extern int rangecode_compress( struct rangecoder *coder, struct databuffer *in, struct databuffer *out );
extern int rangecode_decompress( struct rangecoder *coder, struct databuffer *in, struct databuffer *out, unsigned int length );

#endif

