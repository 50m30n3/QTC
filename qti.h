#ifndef QTI_H
#define QTI_H

struct qti
{
	int width, height;
	int transform;
	int minsize, maxdepth;

	struct databuffer *imagedata;
	struct databuffer *commanddata;
};

extern int qti_create( int width, int height, int minsize, int maxdepth, struct qti *image );
extern int qti_read( struct qti *image, char filename[] );
extern int qti_write( struct qti *image, int compress, char filename[] );
extern void qti_free( struct qti *image );
extern unsigned int qti_getsize( struct qti *image );

#endif
