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

extern int qtv_create( int width, int height, int framerate, int index, struct qtv *video );
extern int qtv_write_header( struct qtv *video, char filename[] );
extern int qtv_write_frame( struct qtv *video, struct qti *image, int compress, int keyframe );
extern int qtv_read_header( struct qtv *video, char filename[] );
extern int qtv_read_frame( struct qtv *video, struct qti *image, int *keyframe );
extern int qtv_can_read_frame( struct qtv *video );
extern int qtv_seek( struct qtv *video, int frame );
extern int qtv_write_index( struct qtv *video );
extern void qtv_free( struct qtv *video );

#endif

