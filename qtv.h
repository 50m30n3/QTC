#ifndef QTV_H
#define QTV_H

struct qtv_index
{
	int frame;
	int offset;
};

struct qtv
{
	int width, height;
	int framerate;
	int numframes, framenum;
	
	FILE *file;

	struct rangecoder *cmdcoder;
	struct rangecoder *imgcoder;
	
	int has_index;
	struct qtv_index *index;
	int idx_size, idx_datasize;
};

extern int create_qtv( int width, int height, int framerate, int index, struct qtv *video );
extern int write_qtv_header( struct qtv *video, char filename[] );
extern int write_qtv_frame( struct qtv *video, struct qti *image, int compress, int keyframe );
extern int read_qtv_header( struct qtv *video, char filename[] );
extern int read_qtv_frame( struct qtv *video, struct qti *image, int *keyframe );
extern int can_read_frame( struct qtv *video );
extern void seek_qtv( struct qtv *video, int frame );
extern int write_qtv_index( struct qtv *video );
extern void free_qtv( struct qtv *video );

#endif

