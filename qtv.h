/*
*    QTC: qtv.h (c) 2011, 2012 50m30n3
*
*    This file is part of QTC.
*
*    QTC is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    QTC is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with QTC.  If not, see <http://www.gnu.org/licenses/>.
*/

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
	
	struct tilecache *tilecache;
	struct rangecoder *idxcoder;
};

extern int qtv_create( struct qtv *video, int width, int height, int framerate, struct tilecache *cache, int index, int is_qtw );
extern int qtv_write_header( struct qtv *video, char filename[] );
extern int qtv_write_frame( struct qtv *video, struct qti *image, int compress );
extern int qtv_write_block( struct qtv *video );
extern int qtv_read_header( struct qtv *video, int is_qtw, char filename[] );
extern int qtv_read_frame( struct qtv *video, struct qti *image );
extern int qtv_can_read_frame( struct qtv *video );
extern int qtv_seek( struct qtv *video, int frame );
extern int qtv_write_index( struct qtv *video );
extern void qtv_free( struct qtv *video );

#endif

