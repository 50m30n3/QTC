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
*                                                                              *
* Modifies the image struct                                                    *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int image_create( struct image *image, int width, int height )
{
	image->width = width;
	image->height = height;

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
	struct pixel p;
	struct pixel *pixels;
	int pr, pg, pb;

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

			pr = pixels[ ia ].x + pixels[ ib ].x - pixels[ ic ].x;
			pg = pixels[ ia ].y + pixels[ ib ].y - pixels[ ic ].y;
			pb = pixels[ ia ].z + pixels[ ib ].z - pixels[ ic ].z;

			p = pixels[ ia ];
			aerr = abs(p.x - pr);
			aerr += abs(p.y - pg);
			aerr += abs(p.z - pb);

			p = pixels[ ib ];
			berr = abs(p.x - pr);
			berr += abs(p.y - pg);
			berr += abs(p.z - pb);
		
			p = pixels[ ic ];
			cerr = abs(p.x - pr);
			cerr += abs(p.y - pg);
			cerr += abs(p.z - pb);

			if( ( aerr < berr ) && ( aerr < cerr ) )
				p = pixels[ ia ];
			else if( berr < cerr )
				p = pixels[ ib ];
			else
				p = pixels[ ic ];

			pixels[ i ].x -= p.x;
			pixels[ i ].y -= p.y;
			pixels[ i ].z -= p.z;
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
	struct pixel p;
	struct pixel *pixels;
	int pr, pg, pb;

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

			pr = pixels[ ia ].x + pixels[ ib ].x - pixels[ ic ].x;
			pg = pixels[ ia ].y + pixels[ ib ].y - pixels[ ic ].y;
			pb = pixels[ ia ].z + pixels[ ib ].z - pixels[ ic ].z;

			p = pixels[ ia ];
			aerr = abs(p.x - pr);
			aerr += abs(p.y - pg);
			aerr += abs(p.z - pb);

			p = pixels[ ib ];
			berr = abs(p.x - pr);
			berr += abs(p.y - pg);
			berr += abs(p.z - pb);
		
			p = pixels[ ic ];
			cerr = abs(p.x - pr);
			cerr += abs(p.y - pg);
			cerr += abs(p.z - pb);

			if( ( aerr < berr ) && ( aerr < cerr ) )
				p = pixels[ ia ];
			else if( berr < cerr )
				p = pixels[ ib ];
			else
				p = pixels[ ic ];

			pixels[ i ].x += p.x;
			pixels[ i ].y += p.y;
			pixels[ i ].z += p.z;
		}
	}
}

