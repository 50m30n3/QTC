#ifndef QTI_H
#define QTI_H

/*******************************************************************************
* Structure to hold all the data associated with a qti                         *
*                                                                              *
* width and height are the dimension of the image                              *
* transform indicates what kind of transform is used on the image              *
*  0 - No transform                                                            *
*  1 - Simplified Paeth transform                                              *
*  2 - Full Paeth transform                                                    *
* colordiff indicates wether the image uses the fakeyuv color mode and comp.   *
*  0 - Normal RGB or BGR color                                                 *
*  1 - Fakeyuv color                                                           *
*  2 - Fakeyuv color and seperate compression of luma and chroma channels      *
* minsize is the minimal block size used during compression                    *
* maxdepth is the maximum recursion depth used during compression              *
* imagedata contains the compressed color data of the image                    *
* commanddata contains the data nessecary for reconstructing the quad tree     *
*******************************************************************************/
struct qti
{
	int width, height;
	int transform, colordiff;
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
