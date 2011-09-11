#include <stdlib.h>
#include <stdio.h>

#include "image.h"

int create_image( struct image *image, unsigned int width, unsigned int height )
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
	int x, y, w, h;

	if( in->width < out->width )
		w = in->width;
	else
		w = out->width;
	
	if( in->height < out->height )
		h = in->height;
	else
		h = out->height;

	for( y=0; y<h; y++ )
	for( x=0; x<w; x++ )
	{
		out->pixels[x+y*w] = in->pixels[x+y*w];
	}
}

void transform_image_fast( struct image *image )
{
	int x, y, i, width, height;
	struct pixel pa, pb, pc;

	width = image->width;
	height = image->height;

	for( y=height-1; y>0; y-- )
	{
		for( x=width-1; x>0; x-- )
		{
			i = x+y*width;

			pa = image->pixels[ i-1 ];
			pb = image->pixels[ i-width ];
			pc = image->pixels[ i-1-width ];

			image->pixels[ i ].r -= pa.r + pb.r - pc.r;
			image->pixels[ i ].g -= pa.g + pb.g - pc.g;
			image->pixels[ i ].b -= pa.b + pb.b - pc.b;
		}
		
		i = y*width;

		pb = image->pixels[ i-width ];

		image->pixels[ i ].r -= pb.r;
		image->pixels[ i ].g -= pb.g;
		image->pixels[ i ].b -= pb.b;
	}

	for( x=width-1; x>0; x-- )
	{
		i = x;

		pa = image->pixels[ i-1 ];

		image->pixels[ i ].r -= pa.r;
		image->pixels[ i ].g -= pa.g;
		image->pixels[ i ].b -= pa.b;
	}
}

void transform_image_fast_rev( struct image *image )
{
	int x, y, i, width, height;
	struct pixel pa, pb, pc;

	width = image->width;
	height = image->height;

	for( x=1; x<width; x++ )
	{
		i = x;

		pa = image->pixels[ i-1 ];

		image->pixels[ i ].r += pa.r;
		image->pixels[ i ].g += pa.g;
		image->pixels[ i ].b += pa.b;
	}

	for( y=1; y<height; y++ )
	{
		i = y*width;

		pb = image->pixels[ i-width ];

		image->pixels[ i ].r += pb.r;
		image->pixels[ i ].g += pb.g;
		image->pixels[ i ].b += pb.b;

		for( x=1; x<width; x++ )
		{
			i = x+y*width;

			pa = image->pixels[ i-1 ];
			pb = image->pixels[ i-width ];
			pc = image->pixels[ i-1-width ];

			image->pixels[ i ].r += pa.r + pb.r - pc.r;
			image->pixels[ i ].g += pa.g + pb.g - pc.g;
			image->pixels[ i ].b += pa.b + pb.b - pc.b;
		}
	}
}

void transform_image( struct image *image )
{
	int x, y, i, width, height, aerr, berr, cerr, ia, ib, ic;
	struct pixel color, p1, p2;
	struct color p;

	width = image->width;
	height = image->height;

	for( y=height-1; y>0; y-- )
	{
		for( x=width-1; x>0; x-- )
		{
			i = x+y*width;
			ia = i-1;
			ib = i-width;
			ic = i-1-width;

			p.r = image->pixels[ ia ].r + image->pixels[ ib ].r - image->pixels[ ic ].r;
			p.g = image->pixels[ ia ].g + image->pixels[ ib ].g - image->pixels[ ic ].g;
			p.b = image->pixels[ ia ].b + image->pixels[ ib ].b - image->pixels[ ic ].b;

			p1 = image->pixels[ ia ];
			aerr = abs(p1.r - p.r);
			aerr += abs(p1.g - p.g);
			aerr += abs(p1.b - p.b);

			p1 = image->pixels[ ib ];
			berr = abs(p1.r - p.r);
			berr += abs(p1.g - p.g);
			berr += abs(p1.b - p.b);
		
			p1 = image->pixels[ ic ];
			cerr = abs(p1.r - p.r);
			cerr += abs(p1.g - p.g);
			cerr += abs(p1.b - p.b);

			p2 = image->pixels[ i ];

			if( ( aerr < berr ) && ( aerr < cerr ) )
			{
				p1 = image->pixels[ ia ];
				color.r = p1.r - p2.r;
				color.g = p1.g - p2.g;
				color.b = p1.b - p2.b;
			}
			else if( berr < cerr )
			{
				p1 = image->pixels[ ib ];
				color.r = p1.r - p2.r;
				color.g = p1.g - p2.g;
				color.b = p1.b - p2.b;
			}
			else
			{
				p1 = image->pixels[ ic ];
				color.r = p1.r - p2.r;
				color.g = p1.g - p2.g;
				color.b = p1.b - p2.b;
			}

			image->pixels[ i ] = color;
		}
		
		i = y*width;

		p1 = image->pixels[ i-width ];
		p2 = image->pixels[ i ];
		
		color.r = p1.r - p2.r;
		color.g = p1.g - p2.g;
		color.b = p1.b - p2.b;
		
		image->pixels[ i ] = color;
	}

	for( x=width-1; x>0; x-- )
	{
		i = x;

		p1 = image->pixels[ i-1 ];
		p2 = image->pixels[ i ];

		color.r = p1.r - p2.r;
		color.g = p1.g - p2.g;
		color.b = p1.b - p2.b;

		image->pixels[ i ] = color;
	}
}

void transform_image_rev( struct image *image )
{
	int x, y, i, width, height, aerr, berr, cerr, ia, ib, ic;
	struct pixel color, p1, p2;
	struct color p;

	width = image->width;
	height = image->height;

	for( x=1; x<width; x++ )
	{
		i = x;

		p1 = image->pixels[ i-1 ];
		p2 = image->pixels[ i ];

		color.r = p1.r - p2.r;
		color.g = p1.g - p2.g;
		color.b = p1.b - p2.b;

		image->pixels[ i ] = color;
	}

	for( y=1; y<height; y++ )
	{
		i = y*width;

		p1 = image->pixels[ i-width ];
		p2 = image->pixels[ i ];
		
		color.r = p1.r - p2.r;
		color.g = p1.g - p2.g;
		color.b = p1.b - p2.b;
		
		image->pixels[ i ] = color;

		for( x=1; x<width; x++ )
		{
			i = x+y*width;
			ia = i-1;
			ib = i-width;
			ic = i-1-width;

			p.r = image->pixels[ ia ].r + image->pixels[ ib ].r - image->pixels[ ic ].r;
			p.g = image->pixels[ ia ].g + image->pixels[ ib ].g - image->pixels[ ic ].g;
			p.b = image->pixels[ ia ].b + image->pixels[ ib ].b - image->pixels[ ic ].b;

			p1 = image->pixels[ ia ];
			aerr = abs(p1.r - p.r);
			aerr += abs(p1.g - p.g);
			aerr += abs(p1.b - p.b);

			p1 = image->pixels[ ib ];
			berr = abs(p1.r - p.r);
			berr += abs(p1.g - p.g);
			berr += abs(p1.b - p.b);
		
			p1 = image->pixels[ ic ];
			cerr = abs(p1.r - p.r);
			cerr += abs(p1.g - p.g);
			cerr += abs(p1.b - p.b);

			p2 = image->pixels[ i ];

			if( ( aerr < berr ) && ( aerr < cerr ) )
			{
				p1 = image->pixels[ ia ];
				color.r = p1.r - p2.r;
				color.g = p1.g - p2.g;
				color.b = p1.b - p2.b;
			}
			else if( berr < cerr )
			{
				p1 = image->pixels[ ib ];
				color.r = p1.r - p2.r;
				color.g = p1.g - p2.g;
				color.b = p1.b - p2.b;
			}
			else
			{
				p1 = image->pixels[ ic ];
				color.r = p1.r - p2.r;
				color.g = p1.g - p2.g;
				color.b = p1.b - p2.b;
			}

			image->pixels[ i ] = color;
		}
	}
}

