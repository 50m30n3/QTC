/*
*    QTC: image.h (c) 2011, 2012 50m30n3
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


#ifndef IMAGE_H
#define IMAGE_H

/*******************************************************************************
* Structure to hold a single pixel                                             *
* Depending on the pixelformat used the fields can have diferrent meanings     *
*                                                                              *
* For RGBA:                                                                    *
* x - Red channel                                                              *
* y - Green channel                                                            *
* z - Blue channel                                                             *
* a - Alpha channel                                                            *
*                                                                              *
* For BGRA:                                                                    *
* x - Blue channel                                                             *
* y - Green channel                                                            *
* z - Red channel                                                              *
* a - Alpha channel                                                            *
*                                                                              *
* For color transformed (fakeyuv) RGBA:                                        *
* x - dR channel                                                               *
* y - Luma channel                                                             *
* z - dB channel                                                               *
* a - Alpha channel                                                            *
*                                                                              *
* For color transformed (fakeyuv) BGRA:                                        *
* x - dB channel                                                               *
* y - Luma channel                                                             *
* z - dR channel                                                               *
* a - Alpha channel                                                            *
*******************************************************************************/
struct pixel
{
	unsigned char x;
	unsigned char y;
	unsigned char z;
	unsigned char a;
};

/*******************************************************************************
* Structure to hold all the data associated with an image                      *
*                                                                              *
* width and height are the dimension of the image                              *
* transform indicates that the paeth transform was applied to the image        *
* colordiff indicates that the pixel data is in colordiff mode                 *
* bgra indicates that the pixel data is in bgra ordering                       *
* pixels points to the image data                                              *
*******************************************************************************/
struct image
{
	int width, height;
	
	int transform, colordiff;
	int bgra;
	
	struct pixel *pixels;
};

extern int image_create( struct image *image, int width, int height, int bgra );
extern void image_free( struct image *image );
extern void image_copy( struct image *in, struct image *out );

extern void image_color_diff( struct image *image );
extern void image_color_diff_rev( struct image *image );

extern void image_transform_fast( struct image *image );
extern void image_transform_fast_rev( struct image *image );
extern void image_transform( struct image *image );
extern void image_transform_rev( struct image *image );

#endif

