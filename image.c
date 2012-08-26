/*
*    QTC: image.c (c) 2011, 2012 50m30n3
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "image.h"

/*******************************************************************************
* Function to initialize an image structure                                    *
*                                                                              *
* image is a pointer to an image struct to hold the information                *
* with is the width of the image                                               *
* height is the height of the image                                            *
* bgra indicates that the pixel data is in bgra ordering                       *
*                                                                              *
* Modifies the image struct                                                    *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int image_create( struct image *image, int width, int height, int bgra )
{
	image->width = width;
	image->height = height;
	
	image->colordiff = 0;
	image->transform = 0;
	
	image->bgra = bgra;

	image->pixels = malloc( sizeof( *image->pixels ) * width * height );

	if( image->pixels == NULL )
	{
		perror( "image_create" );
		return 0;
	}
	else
	{
		return 1;
	}
}

/*******************************************************************************
* Function to free the internal structures of an image                         *
*                                                                              *
* image is the image to free                                                   *
*                                                                              *
* Modifies image                                                               *
*******************************************************************************/
void image_free( struct image *image )
{
	if( image->pixels != NULL )
	{
		free( image->pixels );
		image->pixels = NULL;
	}
}

/*******************************************************************************
* Function to copy the content of one image into another                       *
* Both images need to have the same dimensions                                 *
*                                                                              *
* in is the source image                                                       *
* out is the destinatin image                                                  *
*                                                                              *
* Modifies out                                                                 *
*******************************************************************************/
void image_copy( struct image *in, struct image *out )
{
	memcpy( out->pixels, in->pixels, in->width*in->height*4 );
}

/*******************************************************************************
* Function to apply the fakeyuf transform to an image                          *
*                                                                              *
* image is the image be processed                                              *
*                                                                              *
* Modifies image                                                               *
*******************************************************************************/
void image_color_diff( struct image *image )
{
	int i;
	struct pixel *pixels;

	image->colordiff = 1;

	pixels = image->pixels;

	for( i=0; i<image->width*image->height; i++ )
	{
		pixels[ i ].x -= pixels[ i ].y;
		pixels[ i ].z -= pixels[ i ].y;
	}
}

/*******************************************************************************
* Function to apply the reverse fakeyuf transform to an image                  *
*                                                                              *
* image is the image be processed                                              *
*                                                                              *
* Modifies image                                                               *
*******************************************************************************/
void image_color_diff_rev( struct image *image )
{
	int i;
	struct pixel *pixels;

	image->colordiff = 0;

	pixels = image->pixels;

	for( i=0; i<image->width*image->height; i++ )
	{
		pixels[ i ].x += pixels[ i ].y;
		pixels[ i ].z += pixels[ i ].y;
	}
}

/*******************************************************************************
* Function to apply the simplified Paeth transform to an image                 *
*                                                                              *
* image is the image be processed                                              *
*                                                                              *
* Modifies image                                                               *
*******************************************************************************/
void image_transform_fast( struct image *image )
{
	int x, y, i, width, height;
	struct pixel pa, pb, pc;
	struct pixel *pixels;

	image->transform = 1;

	width = image->width;
	height = image->height;
	pixels = image->pixels;

	i = width-1+(height-1)*width;

	for( y=height-1; y>0; y-- )
	{
		for( x=width-1; x>0; x-- )
		{
			pa = pixels[ i-1 ];
			pb = pixels[ i-width ];
			pc = pixels[ i-1-width ];

			pixels[ i ].x -= pa.x + pb.x - pc.x;
			pixels[ i ].y -= pa.y + pb.y - pc.y;
			pixels[ i ].z -= pa.z + pb.z - pc.z;
			
			i--;
		}
		
		pb = pixels[ i-width ];

		pixels[ i ].x -= pb.x;
		pixels[ i ].y -= pb.y;
		pixels[ i ].z -= pb.z;
		
		i--;
	}

	for( x=width-1; x>0; x-- )
	{
		i = x;

		pa = pixels[ i-1 ];

		pixels[ i ].x -= pa.x;
		pixels[ i ].y -= pa.y;
		pixels[ i ].z -= pa.z;
	}
}

/*******************************************************************************
* Function to apply the reversed simplified Paeth transform to an image        *
*                                                                              *
* image is the image be processed                                              *
*                                                                              *
* Modifies image                                                               *
*******************************************************************************/
void image_transform_fast_rev( struct image *image )
{
	int x, y, i, width, height;
	struct pixel pa, pb, pc;
	struct pixel *pixels;

	image->transform = 0;

	width = image->width;
	height = image->height;
	pixels = image->pixels;

	for( x=1; x<width; x++ )
	{
		i = x;

		pa = pixels[ i-1 ];

		pixels[ i ].x += pa.x;
		pixels[ i ].y += pa.y;
		pixels[ i ].z += pa.z;
	}

	i = width;
	for( y=1; y<height; y++ )
	{
		pb = pixels[ i-width ];

		pixels[ i ].x += pb.x;
		pixels[ i ].y += pb.y;
		pixels[ i ].z += pb.z;
		
		i++;

		for( x=1; x<width; x++ )
		{
			pa = pixels[ i-1 ];
			pb = pixels[ i-width ];
			pc = pixels[ i-1-width ];

			pixels[ i ].x += pa.x + pb.x - pc.x;
			pixels[ i ].y += pa.y + pb.y - pc.y;
			pixels[ i ].z += pa.z + pb.z - pc.z;
			
			i++;
		}
	}
}

/*******************************************************************************
* Function to apply the Paeth transform to an image                            *
*                                                                              *
* image is the image be processed                                              *
*                                                                              *
* Modifies image                                                               *
*******************************************************************************/
void image_transform( struct image *image )
{
	int x, y, i, width, height, aerr, berr, cerr, ia, ib, ic;
	struct pixel a, b, c, p;
	struct pixel *pixels;
	int px, py, pz, qx, qy, qz;

	image->transform = 2;

	width = image->width;
	height = image->height;
	pixels = image->pixels;

	for( y=height-1; y>0; y-- )
	{
		for( x=width-1; x>0; x-- )
		{
			i = x+y*width;
			ia = i-1;
			ib = i-width;
			ic = i-1-width;

			a = pixels[ ia ];
			b = pixels[ ib ];
			c = pixels[ ic ];

			px = b.x - c.x;
			py = b.y - c.y;
			pz = b.z - c.z;

			qx = a.x - c.x;
			qy = a.y - c.y;
			qz = a.z - c.z;

			aerr = abs(px) + abs(py) + abs(pz);
			berr = abs(qx) + abs(qy) + abs(qz);
			cerr = abs(px + qx) + abs(py + qy) + abs(pz + qz);

			if (berr < aerr) aerr = berr, a = b;
			if (cerr < aerr) a = c;

			pixels[ i ].x -= a.x;
			pixels[ i ].y -= a.y;
			pixels[ i ].z -= a.z;
		}
		
		i = y*width;

		p = pixels[ i-width ];
		
		pixels[ i ].x -= p.x;
		pixels[ i ].y -= p.y;
		pixels[ i ].z -= p.z;
	}

	for( x=width-1; x>0; x-- )
	{
		i = x;

		p = pixels[ i-1 ];

		pixels[ i ].x -= p.x;
		pixels[ i ].y -= p.y;
		pixels[ i ].z -= p.z;
	}
}

/*******************************************************************************
* Function to apply the reverse Paeth transform to an image                    *
*                                                                              *
* image is the image be processed                                              *
*                                                                              *
* Modifies image                                                               *
*******************************************************************************/
void image_transform_rev( struct image *image )
{
	int x, y, i, width, height, aerr, berr, cerr, ia, ib, ic;
	struct pixel p, a, b, c;
	struct pixel *pixels;
	int px, py, pz, qx, qy, qz;

	image->transform = 0;

	width = image->width;
	height = image->height;
	pixels = image->pixels;

	for( x=1; x<width; x++ )
	{
		i = x;

		p = pixels[ i-1 ];

		pixels[ i ].x += p.x;
		pixels[ i ].y += p.y;
		pixels[ i ].z += p.z;
	}

	for( y=1; y<height; y++ )
	{
		i = y*width;

		p = pixels[ i-width ];
		
		pixels[ i ].x += p.x;
		pixels[ i ].y += p.y;
		pixels[ i ].z += p.z;
		
		for( x=1; x<width; x++ )
		{
			i = x+y*width;
			ia = i-1;
			ib = i-width;
			ic = i-1-width;

			a = pixels[ ia ];
			b = pixels[ ib ];
			c = pixels[ ic ];

			px = b.x - c.x;
			py = b.y - c.y;
			pz = b.z - c.z;

			qx = a.x - c.x;
			qy = a.y - c.y;
			qz = a.z - c.z;

			aerr = abs(px) + abs(py) + abs(pz);
			berr = abs(qx) + abs(qy) + abs(qz);
			cerr = abs(px + qx) + abs(py + qy) + abs(pz + qz);

			if (berr < aerr) aerr = berr, a = b;
			if (cerr < aerr) a = c;

			pixels[ i ].x += a.x;
			pixels[ i ].y += a.y;
			pixels[ i ].z += a.z;
		}
	}
}

