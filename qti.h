/*
*    QTC: qti.h (c) 2011, 2012 50m30n3
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
* keyframes indicates wether the image makes use of a reference image or not   *
* imagedata contains the compressed color data of the image                    *
* commanddata contains the data nessecary for reconstructing the quad tree     *
*******************************************************************************/
struct qti
{
	int width, height;
	int transform, colordiff;
	int minsize, maxdepth;
	int keyframe;

	struct databuffer *imagedata;
	struct databuffer *indexdata;
	struct databuffer *commanddata;
	
	struct tilecache *tilecache;
};

extern int qti_create( struct qti *image, int width, int height, int minsize, int maxdepth, struct tilecache *cache );
extern int qti_read( struct qti *image, char filename[] );
extern int qti_write( struct qti *image, int compress, char filename[] );
extern void qti_free( struct qti *image );
extern unsigned int qti_getsize( struct qti *image );

#endif
