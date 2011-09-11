#ifndef RANGECODE_H
#define RANGECODE_H

struct rangecoder
{
	int order;
	int *freq;
};

extern struct rangecoder *create_rangecoder( int order );
extern void reset_model( struct rangecoder *coder );
extern void free_rangecoder( struct rangecoder *coder );
extern int rangecode_compress( struct rangecoder *coder, struct databuffer *in, struct databuffer *out );
extern int rangecode_decompress( struct rangecoder *coder, struct databuffer *in, struct databuffer *out, unsigned int length );

#endif

