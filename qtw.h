#ifndef QTW_H
#define QTW_H

struct qtw_index
{
	int frame;
	int block;
	long int offset;
};

struct qtw
{
	int width, height;
	int framerate;
	int numframes, framenum;
	int numblocks, blocknum;
	
	FILE *file, *blockfile;
	
	char *basename;

	struct rangecoder *cmdcoder;
	struct rangecoder *imgcoder;
	
	struct qtw_index *index;
	int idx_size, idx_datasize;
};

extern int qtw_create( int width, int height, int framerate, struct qtw *video );
extern int qtw_write_header( struct qtw *video, char filename[] );
extern int qtw_write_frame( struct qtw *video, struct qti *image, int compress, int keyframe );
extern int qtw_write_block( struct qtw *video );
extern int qtw_read_header( struct qtw *video, char filename[] );
extern int qtw_read_frame( struct qtw *video, struct qti *image, int *keyframe );
extern int qtw_can_read_frame( struct qtw *video );
extern int qtw_seek( struct qtw *video, int frame );
extern int qtw_write_index( struct qtw *video );
extern void qtw_free( struct qtw *video );

#endif

