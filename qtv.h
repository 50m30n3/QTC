#ifndef QTV_H
#define QTV_H

/*******************************************************************************
* Structure to hold all the data associated with a qtv index entry             *
*                                                                              *
* frame is the frame number of the keyframe                                    *
* block is the block the keyframe resides in (only qtw)                        *
* offset is the frame offset from the begging of the file                      *
*******************************************************************************/
struct qtv_index
{
	int frame;
	int block;
	long int offset;
};

/*******************************************************************************
* Structure to hold all the data associated with a qtv                         *
*                                                                              *
* width and height are the dimension of the video                              *
* framerate is the frame rate if the video                                     *
* numframes is the number of frames the video has                              *
* framenum is the current frame number                                         *
* numblocks is the number of blocks the video is split into (only qtw)         *
* blocknum is the current block number (only qtw)                              *
* is_qtw indicates wether the video is a qtw                                   *
* file is the file object to read/write                                        *
* streamfile is the file object for the current block (only qtw)               *
* filename is the file name of the video                                       *
* cmdcoder is the range coder used to compress the command data                *
* imgcoder is the range coder used to compress the image data                  *
* has_index indicates wether the video has an index or not                     *
* index contains the video index                                               *
* idx_size is the number of entries in the index                               *
* idx_datasize is the amount of space allocated for the index entries          *
*******************************************************************************/
struct qtv
{
	int width, height;
	int framerate;
	int numframes, framenum;
	int numblocks, blocknum;

	int is_qtw;

	FILE *file, *streamfile;
	char *filename;

	struct rangecoder *cmdcoder;
	struct rangecoder *imgcoder;
	
	int has_index;
	struct qtv_index *index;
	int idx_size, idx_datasize;
};

extern int qtv_create( int width, int height, int framerate, int index, int is_qtw, struct qtv *video );
extern int qtv_write_header( struct qtv *video, char filename[] );
extern int qtv_write_frame( struct qtv *video, struct qti *image, int compress, int keyframe );
extern int qtv_write_block( struct qtv *video );
extern int qtv_read_header( struct qtv *video, int is_qtw, char filename[] );
extern int qtv_read_frame( struct qtv *video, struct qti *image, int *keyframe );
extern int qtv_can_read_frame( struct qtv *video );
extern int qtv_seek( struct qtv *video, int frame );
extern int qtv_write_index( struct qtv *video );
extern void qtv_free( struct qtv *video );

#endif

