#include <stdlib.h>
#include <stdio.h>

#include "image.h"

int create_image( struct image *image, int width, int height )
{
	image->width = width;
	image->height = height;

	image->pixels = malloc( sizeof( struct pixel ) * width * height );

	if( image->pixels == NULL )
	{
		perror( "create_image" );
		return 0;
	}
	else
	{
		return 1;
	}
}

void free_image( struct image *image )
{
	if( image->pixels != NULL )
	{
		free( image->pixels );
		image->pixels = NULL;
	}
}

void copy_image( struct image *in, struct image *out )
{
	int i;

	for( i=0; i<in->width*in->height; i++ )
		out->pixels[i] = in->pixels[i];
}

void transform_image_fast( struct image *image )
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

			pixels[ i ].r -= pa.r + pb.r - pc.r;
			pixels[ i ].g -= pa.g + pb.g - pc.g;
			pixels[ i ].b -= pa.b + pb.b - pc.b;
			
			i--;
		}
		
		pb = pixels[ i-width ];

		pixels[ i ].r -= pb.r;
		pixels[ i ].g -= pb.g;
		pixels[ i ].b -= pb.b;
		
		i--;
	}

	for( x=width-1; x>0; x-- )
	{
		i = x;

		pa = pixels[ i-1 ];

		pixels[ i ].r -= pa.r;
		pixels[ i ].g -= pa.g;
		pixels[ i ].b -= pa.b;
	}
}

void transform_image_fast_rev( struct image *image )
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

		pixels[ i ].r += pa.r;
		pixels[ i ].g += pa.g;
		pixels[ i ].b += pa.b;
	}

	i = width;
	for( y=1; y<height; y++ )
	{
		pb = pixels[ i-width ];

		pixels[ i ].r += pb.r;
		pixels[ i ].g += pb.g;
		pixels[ i ].b += pb.b;
		
		i++;

		for( x=1; x<width; x++ )
		{
			pa = pixels[ i-1 ];
			pb = pixels[ i-width ];
			pc = pixels[ i-1-width ];

			pixels[ i ].r += pa.r + pb.r - pc.r;
			pixels[ i ].g += pa.g + pb.g - pc.g;
			pixels[ i ].b += pa.b + pb.b - pc.b;
			
			i++;
		}
	}
}

void transform_image( struct image *image )
{
	int x, y, i, width, height, aerr, berr, cerr, ia, ib, ic;
	struct pixel p1, p2;
	struct pixel *pixels;
	struct color p;

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

			p.r = pixels[ ia ].r + pixels[ ib ].r - pixels[ ic ].r;
			p.g = pixels[ ia ].g + pixels[ ib ].g - pixels[ ic ].g;
			p.b = pixels[ ia ].b + pixels[ ib ].b - pixels[ ic ].b;

			p1 = pixels[ ia ];
			aerr = abs(p1.r - p.r);
			aerr += abs(p1.g - p.g);
			aerr += abs(p1.b - p.b);

			p1 = pixels[ ib ];
			berr = abs(p1.r - p.r);
			berr += abs(p1.g - p.g);
			berr += abs(p1.b - p.b);
		
			p1 = pixels[ ic ];
			cerr = abs(p1.r - p.r);
			cerr += abs(p1.g - p.g);
			cerr += abs(p1.b - p.b);

			p2 = pixels[ i ];

			if( ( aerr < berr ) && ( aerr < cerr ) )
				p1 = pixels[ ia ];
			else if( berr < cerr )
				p1 = pixels[ ib ];
			else
				p1 = pixels[ ic ];

			pixels[ i ].r -= p1.r;
			pixels[ i ].g -= p1.g;
			pixels[ i ].b -= p1.b;
		}
		
		i = y*width;

		p1 = pixels[ i-width ];
		
		pixels[ i ].r -= p1.r;
		pixels[ i ].g -= p1.g;
		pixels[ i ].b -= p1.b;
	}

	for( x=width-1; x>0; x-- )
	{
		i = x;

		p1 = pixels[ i-1 ];

		pixels[ i ].r -= p1.r;
		pixels[ i ].g -= p1.g;
		pixels[ i ].b -= p1.b;
	}
}

void transform_image_rev( struct image *image )
{
	int x, y, i, width, height, aerr, berr, cerr, ia, ib, ic;
	struct pixel p1, p2;
	struct pixel *pixels;
	struct color p;

	width = image->width;
	height = image->height;
	pixels = image->pixels;

	for( x=1; x<width; x++ )
	{
		i = x;

		p1 = pixels[ i-1 ];

		pixels[ i ].r += p1.r;
		pixels[ i ].g += p1.g;
		pixels[ i ].b += p1.b;
	}

	for( y=1; y<height; y++ )
	{
		i = y*width;

		p1 = pixels[ i-width ];
		
		pixels[ i ].r += p1.r;
		pixels[ i ].g += p1.g;
		pixels[ i ].b += p1.b;
		
		for( x=1; x<width; x++ )
		{
			i = x+y*width;
			ia = i-1;
			ib = i-width;
			ic = i-1-width;

			p.r = pixels[ ia ].r + pixels[ ib ].r - pixels[ ic ].r;
			p.g = pixels[ ia ].g + pixels[ ib ].g - pixels[ ic ].g;
			p.b = pixels[ ia ].b + pixels[ ib ].b - pixels[ ic ].b;

			p1 = pixels[ ia ];
			aerr = abs(p1.r - p.r);
			aerr += abs(p1.g - p.g);
			aerr += abs(p1.b - p.b);

			p1 = pixels[ ib ];
			berr = abs(p1.r - p.r);
			berr += abs(p1.g - p.g);
			berr += abs(p1.b - p.b);
		
			p1 = pixels[ ic ];
			cerr = abs(p1.r - p.r);
			cerr += abs(p1.g - p.g);
			cerr += abs(p1.b - p.b);

			p2 = pixels[ i ];

			if( ( aerr < berr ) && ( aerr < cerr ) )
				p1 = pixels[ ia ];
			else if( berr < cerr )
				p1 = pixels[ ib ];
			else
				p1 = pixels[ ic ];

			pixels[ i ].r += p1.r;
			pixels[ i ].g += p1.g;
			pixels[ i ].b += p1.b;
		}
	}
}

