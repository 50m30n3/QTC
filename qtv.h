#ifndef QTV_H
#define QTV_H

struct qtv
{
	int width, height;
	int framerate;
	
	FILE *file;

	struct rangecoder *cmdcoder;
	struct rangecoder *imgcoder;
};

extern int create_qtv( int width, int height, int framerate, struct qtv *video );
extern int write_qtv_header( struct qtv *video, char filename[] );
extern int write_qtv_frame( struct qtv *video, struct qti *image, int compress, int keyframe );
extern int read_qtv_header( struct qtv *video, char filename[] );
extern int read_qtv_frame( struct qtv *video, struct qti *image, int *keyframe );
extern int can_read_frame( struct qtv *video );
extern void free_qtv( struct qtv *video );

#endif

