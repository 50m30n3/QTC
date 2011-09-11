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

extern int create_qti( int width, int height, int minsize, int maxdepth, struct qti *image );
extern int read_qti( struct qti *image, char filename[] );
extern int write_qti( struct qti *image, int compress, char filename[] );
extern void free_qti( struct qti *image );
extern unsigned int qti_getsize( struct qti *image );
extern unsigned int qti_getcsize( struct qti *image );
extern unsigned int qti_getdsize( struct qti *image );

#endif
