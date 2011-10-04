#ifndef DATABUFFER_H
#define DATABUFFER_H

struct databuffer
{
	unsigned char *data;
	unsigned char wbuffer, rbuffer;
	unsigned int datasize;
	unsigned int size, bits;
	unsigned int pos, bitpos;
};

extern struct databuffer *databuffer_create( unsigned int size );
extern void databuffer_free( struct databuffer *buffer );
extern int databuffer_pad( struct databuffer *buffer );
extern int databuffer_add_bits( unsigned int data, struct databuffer *buffer, int bits );
extern int databuffer_add_byte( unsigned char data, struct databuffer *buffer );
extern unsigned int databuffer_get_bits( struct databuffer *buffer, int bits );
extern unsigned char databuffer_get_byte( struct databuffer *buffer );

#endif

