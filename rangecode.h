#ifndef RANGECODE_H
#define RANGECODE_H

struct rangecoder
{
	int order;
	int *freq;
};

extern struct rangecoder *rangecoder_create( int order );
extern void rangecoder_reset( struct rangecoder *coder );
extern void rangecoder_free( struct rangecoder *coder );
extern int rangecode_compress( struct rangecoder *coder, struct databuffer *in, struct databuffer *out );
extern int rangecode_decompress( struct rangecoder *coder, struct databuffer *in, struct databuffer *out, unsigned int length );

#endif

