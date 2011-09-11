#ifndef DATABUFFER_H
#define DATABUFFER_H

struct databuffer
{
	unsigned char *data;
	unsigned int datasize;
	unsigned int size, pos, bits;
};

extern struct databuffer *create_databuffer( unsigned int size );
extern void free_databuffer( struct databuffer *buffer );
extern int pad_data( struct databuffer *buffer );
extern int add_data( unsigned int data, struct databuffer *buffer, int bits );
extern int add_data_byte( unsigned char data, struct databuffer *buffer );
extern unsigned int get_data( struct databuffer *buffer, int bits );
extern unsigned char get_data_byte( struct databuffer *buffer );

#endif

