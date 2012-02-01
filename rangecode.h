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

