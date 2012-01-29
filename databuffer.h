#ifndef DATABUFFER_H
#define DATABUFFER_H

/*******************************************************************************
* Structure to hold all the data associated with a databuffer                  *
*                                                                              *
* data is the data held by the databuffer                                      *
* wbuffer and rbuffer hold incomplete bytes                                    *
* datasize is the amount of data allocated for the databuffer                  *
* size is the current amount of data in the databuffer                         *
* bits is the current number of bits in wbuffer that have not yet been added   *
* pos is the current read position of the buffer                               *
* bitpos is the current number of bits in rbuffer                              *
*                                                                              *
* rbuffer and wbuffer buffer the current incomplete byte. A call to            *
* databuffer_pad or databuffer_add_byte flushes the wbuffer. A call to         *
* databuffer_get_byte discards the bits in the rbuffer.                        *
*******************************************************************************/
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

