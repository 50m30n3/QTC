#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "databuffer.h"
#include "qti.h"
#include "image.h"

#include "qtc.h"


static inline void put_rgb_pixel( struct databuffer *databuffer, struct pixel pixel )
{
	databuffer_add_byte( pixel.x, databuffer );
	databuffer_add_byte( pixel.y, databuffer );
	databuffer_add_byte( pixel.z, databuffer );
}

static inline void put_bgr_pixel( struct databuffer *databuffer, struct pixel pixel )
{
	databuffer_add_byte( pixel.z, databuffer );
	databuffer_add_byte( pixel.y, databuffer );
	databuffer_add_byte( pixel.x, databuffer );
}

static inline void put_luma_pixel( struct databuffer *databuffer, struct pixel pixel )
{
	databuffer_add_byte( pixel.y, databuffer );
}

static inline void put_rgb_chroma_pixel( struct databuffer *databuffer, struct pixel pixel )
{
	databuffer_add_byte( pixel.x, databuffer );
	databuffer_add_byte( pixel.z, databuffer );
}

static inline void put_bgr_chroma_pixel( struct databuffer *databuffer, struct pixel pixel )
{
	databuffer_add_byte( pixel.z, databuffer );
	databuffer_add_byte( pixel.x, databuffer );
}

static inline void put_pixels( struct databuffer *databuffer, struct pixel *pixels, int x1, int x2, int y1, int y2, int width, int bgra, int colordiff, int luma )
{
	int x, y, i;

	if( ! colordiff )
	{
		if( bgra )
		{
			for( y=y1; y<y2; y++ )
			{
				i = x1 + y*width;
				for( x=x1; x<x2; x++ )
					put_bgr_pixel( databuffer, pixels[i++] );
			}
		}
		else
		{
			for( y=y1; y<y2; y++ )
			{
				i = x1 + y*width;
				for( x=x1; x<x2; x++ )
					put_rgb_pixel( databuffer, pixels[i++] );
			}
		}
	}
	else
	{
		if( luma )
		{
			for( y=y1; y<y2; y++ )
			{
				i = x1 + y*width;
				for( x=x1; x<x2; x++ )
					put_luma_pixel( databuffer, pixels[i++] );
			}
		}
		else
		{
			if( bgra )
			{
				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*width;
					for( x=x1; x<x2; x++ )
						put_bgr_chroma_pixel( databuffer, pixels[i++] );
				}
			}
			else
			{
				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*width;
					for( x=x1; x<x2; x++ )
						put_rgb_chroma_pixel( databuffer, pixels[i++] );
				}
			}
		}
	}
}

int qtc_compress( struct image *input, struct image *refimage, struct qti *output, int minsize, int maxdepth, int lazyness, int bgra, int colordiff )
{
	struct databuffer *commanddata, *imagedata;
	unsigned int *inpixels, *refpixels;
	unsigned int mask;
	int luma;

	void qtc_compress_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int x, y, sx, sy, i;
		unsigned int p;
		struct pixel color;
		int error;

		if( depth >= lazyness )
		{
			if( refimage != NULL )
			{
				error = 0;

				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*input->width;
					for( x=x1; x<x2; x++ )
					{
						if( ( inpixels[ i ] ^ refpixels[ i ] ) & mask )
						{
							error = 1;
							break;
						}

						i++;				
					}
			
					if( error )
						break;
				}
		
				if( error )
				{
					databuffer_add_bits( 1, commanddata, 1 );
				}
				else
				{
					databuffer_add_bits( 0, commanddata, 1 );
					return;
				}
			}


			error = 0;

			p = inpixels[ x1 + y1*input->width ];

			for( y=y1; y<y2; y++ )
			{
				i = x1 + y*input->width;
				for( x=x1; x<x2; x++ )
				{
					if( ( p ^ inpixels[ i++ ] ) & mask )
					{
						error = 1;
						break;
					}
				}

				if( error )
					break;
			}
		}
		else
		{
			if( refimage != NULL )
				databuffer_add_bits( 1, commanddata, 1 );

			error = 1;
		}

		if( error )
		{
			databuffer_add_bits( 0, commanddata, 1 );
			if( depth < maxdepth )
			{
				if( ( x2-x1 > minsize ) && ( y2-y1 > minsize ) )
				{
					sx = x1 + (x2-x1)/2;
					sy = y1 + (y2-y1)/2;

					qtc_compress_rec( x1, y1, sx, sy, depth+1 );
					qtc_compress_rec( x1, sy, sx, y2, depth+1 );
					qtc_compress_rec( sx, y1, x2, sy, depth+1 );
					qtc_compress_rec( sx, sy, x2, y2, depth+1 );
				}
				else
				{
					if( x2-x1 > minsize )
					{
						sx = x1 + (x2-x1)/2;
		
						qtc_compress_rec( x1, y1, sx, y2, depth+1 );
						qtc_compress_rec( sx, y1, x2, y2, depth+1 );
					}
					else if ( y2-y1 > minsize )
					{
						sy = y1 + (y2-y1)/2;
		
						qtc_compress_rec( x1, y1, x2, sy, depth+1 );
						qtc_compress_rec( x1, sy, x2, y2, depth+1 );
					}
					else
					{
						put_pixels( imagedata, input->pixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma );
					}
				}
			}
			else
			{
				put_pixels( imagedata, input->pixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma );
			}
		}
		else
		{
			databuffer_add_bits( 1, commanddata, 1 );

			color = input->pixels[ x1 + y1*input->width ];

			if( ! colordiff )
			{
				if( bgra )
					put_bgr_pixel( imagedata, color );
				else
					put_rgb_pixel( imagedata, color );
			}
			else
			{
				if( luma )
				{
					put_luma_pixel( imagedata, color );
				}
				else
				{
					if( bgra )
						put_bgr_chroma_pixel( imagedata, color );
					else
						put_rgb_chroma_pixel( imagedata, color );
				}
			}
		}
	}

	if( ! qti_create( input->width, input->height, minsize, maxdepth, output ) )
		return 0;
	
	commanddata = output->commanddata;
	imagedata = output->imagedata;

	inpixels = (unsigned int *)input->pixels;

	if( refimage != NULL )
		refpixels = (unsigned int *)refimage->pixels;

	if( ! colordiff )
	{
		mask = 0x00FFFFFF;
		luma = 0;
		qtc_compress_rec( 0, 0, input->width, input->height, 0 );
	}
	else
	{
		mask = 0x0000FF00;
		luma = 1;
		qtc_compress_rec( 0, 0, input->width, input->height, 0 );

		mask = 0x00FF00FF;
		luma = 0;
		qtc_compress_rec( 0, 0, input->width, input->height, 0 );
	}
	
	return 1;
}


static inline void get_pixels( struct databuffer *imagedata, struct pixel *pixels, int x1, int x2, int y1, int y2, int width, int bgra, int colordiff, int luma )
{
	int x, y, i;

	if( ! colordiff )
	{
		if( bgra )
		{
			for( y=y1; y<y2; y++ )
			{
				i = x1 + y*width;
				for( x=x1; x<x2; x++ )
				{
					pixels[i].z = databuffer_get_byte( imagedata );
					pixels[i].y = databuffer_get_byte( imagedata );
					pixels[i].x = databuffer_get_byte( imagedata );
					i++;
				}
			}
		}
		else
		{
			for( y=y1; y<y2; y++ )
			{
				i = x1 + y*width;
				for( x=x1; x<x2; x++ )
				{
					pixels[i].x = databuffer_get_byte( imagedata );
					pixels[i].y = databuffer_get_byte( imagedata );
					pixels[i].z = databuffer_get_byte( imagedata );
					i++;
				}
			}
		}
	}
	else
	{
		if( luma )
		{
			for( y=y1; y<y2; y++ )
			{
				i = x1 + y*width;
				for( x=x1; x<x2; x++ )
				{
					pixels[i++].y = databuffer_get_byte( imagedata );
				}
			}
		}
		else
		{
			if( ! bgra )
			{
				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*width;
					for( x=x1; x<x2; x++ )
					{
						pixels[i].x = databuffer_get_byte( imagedata );
						pixels[i].z = databuffer_get_byte( imagedata );
						i++;
					}
				}
			}
			else
			{
				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*width;
					for( x=x1; x<x2; x++ )
					{
						pixels[i].z = databuffer_get_byte( imagedata );
						pixels[i].x = databuffer_get_byte( imagedata );
						i++;
					}
				}
			}
		}
	}
}

int qtc_decompress( struct qti *input, struct image *refimage, struct image *output, int bgra, int colordiff )
{
	struct databuffer *commanddata, *imagedata;
	int minsize, maxdepth;
	struct pixel *outpixels;
	unsigned int mask;
	int luma;

	void qtc_decompress_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int x, y, sx, sy, i;
		struct pixel color;
		unsigned char status;

		if( refimage )
			status = databuffer_get_bits( commanddata, 1 );
		else
			status = 1;

		if( status != 0 )
		{
			status = databuffer_get_bits( commanddata, 1 );
			if( status == 0 )
			{
				if( depth < maxdepth )
				{
					if( ( x2-x1 > minsize ) && ( y2-y1 > minsize ) )
					{
						sx = x1 + (x2-x1)/2;
						sy = y1 + (y2-y1)/2;

						qtc_decompress_rec( x1, y1, sx, sy, depth+1 );
						qtc_decompress_rec( x1, sy, sx, y2, depth+1 );
						qtc_decompress_rec( sx, y1, x2, sy, depth+1 );
						qtc_decompress_rec( sx, sy, x2, y2, depth+1 );
					}
					else
					{
						if( x2-x1 > minsize )
						{
							sx = x1 + (x2-x1)/2;

							qtc_decompress_rec( x1, y1, sx, y2, depth+1 );
							qtc_decompress_rec( sx, y1, x2, y2, depth+1 );
						}
						else if ( y2-y1 > minsize )
						{
							sy = y1 + (y2-y1)/2;
	
							qtc_decompress_rec( x1, y1, x2, sy, depth+1 );
							qtc_decompress_rec( x1, sy, x2, y2, depth+1 );
						}
						else
						{
							get_pixels( imagedata, (struct pixel *)outpixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma );
						}
					}
				}
				else
				{
					get_pixels( imagedata, (struct pixel *)outpixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma );
				}
			}
			else
			{
				if( ! colordiff )
				{
					if( bgra )
					{
						color.z = databuffer_get_byte( imagedata );
						color.y = databuffer_get_byte( imagedata );
						color.x = databuffer_get_byte( imagedata );
						color.a = 0;
					}
					else
					{
						color.x = databuffer_get_byte( imagedata );
						color.y = databuffer_get_byte( imagedata );
						color.z = databuffer_get_byte( imagedata );
						color.a = 0;
					}

					for( y=y1; y<y2; y++ )
					{
						i = x1 + y*input->width;
						for( x=x1; x<x2; x++ )
						{
							outpixels[ i++ ] = color;
						}
					}
				}
				else
				{
					if( luma )
					{
						color.y = databuffer_get_byte( imagedata );

						for( y=y1; y<y2; y++ )
						{
							i = x1 + y*input->width;
							for( x=x1; x<x2; x++ )
							{
								outpixels[ i++ ].y = color.y;
							}
						}
					}
					else
					{
						if( bgra )
						{
							color.z = databuffer_get_byte( imagedata );
							color.x = databuffer_get_byte( imagedata );
						}
						else
						{
							color.x = databuffer_get_byte( imagedata );
							color.z = databuffer_get_byte( imagedata );
						}

						for( y=y1; y<y2; y++ )
						{
							i = x1 + y*input->width;
							for( x=x1; x<x2; x++ )
							{
								outpixels[ i ].x = color.x;
								outpixels[ i ].z = color.z;
								i++;
							}
						}
					}
				}
			}
		}
	}

	if( ! image_create( output, input->width, input->height ) )
		return 0;
	
	commanddata = input->commanddata;
	imagedata = input->imagedata;
	minsize = input->minsize;
	maxdepth = input->maxdepth;

	outpixels = output->pixels;

	if( refimage != NULL )
		memcpy( outpixels, refimage->pixels, input->width * input->height * 4 );

	if( ! colordiff )
	{
		mask = 0x00FFFFFF;
		luma = 0;
		qtc_decompress_rec( 0, 0, input->width, input->height, 0 );
	}
	else
	{
		mask = 0x0000FF00;
		luma = 1;
		qtc_decompress_rec( 0, 0, input->width, input->height, 0 );

		mask = 0x00FF00FF;
		luma = 0;
		qtc_decompress_rec( 0, 0, input->width, input->height, 0 );
	}

	return 1;
}


static inline void put_ccode_box( unsigned int *pixels, int x1, int x2, int y1, int y2, int width, unsigned int color, unsigned int linecolor )
{
	int x, y, i;

	i = x1 + y1*width;
	for( x=x1; x<x2; x++ )
	{
		pixels[ i++ ] |= linecolor;
	}

	for( y=y1+1; y<y2-1; y++ )
	{
		i = x1 + y*width;

		pixels[ i++ ] |= linecolor;

		for( x=x1+1; x<x2-1; x++ )
		{
			pixels[ i++ ] += color;
		}

		pixels[ i++ ] |= linecolor;
	}

	i = x1 + (y2-1)*width;
	for( x=x1; x<x2; x++ )
	{
		pixels[ i++ ] |= linecolor;
	}
}

int qtc_decompress_ccode( struct qti *input, struct image *output, int refimage, int bgra, int colordiff, int channel )
{
	struct databuffer *commanddata, *imagedata;
	int minsize, maxdepth;
	unsigned int *outpixels;
	int i;

	void qtc_decompress_ccode_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int x, y, sx, sy, i;
		unsigned char status;
		unsigned int color;

		color = 64/maxdepth;

		for( y=y1; y<y2; y++ )
		{
			i = x1 + y*input->width;
			for( x=x1; x<x2; x++ )
			{
				outpixels[ i ] += color;
				outpixels[ i ] += color<<8;
				outpixels[ i ] += color<<16;
				i++;
			}
		}

		if( refimage )
			status = databuffer_get_bits( commanddata, 1 );
		else
			status = 1;

		if( status == 0 )
		{
			if( bgra )
				put_ccode_box( outpixels, x1, x2, y1, y2, input->width, 0x0000007F, 0x000000FF );
			else
				put_ccode_box( outpixels, x1, x2, y1, y2, input->width, 0x007F0000, 0x00FF0000 );
		}
		else
		{
			status = databuffer_get_bits( commanddata, 1 );
			if( status == 0 )
			{
				if( depth < maxdepth )
				{
					if( ( x2-x1 > minsize ) && ( y2-y1 > minsize ) )
					{
						sx = x1 + (x2-x1)/2;
						sy = y1 + (y2-y1)/2;

						qtc_decompress_ccode_rec( x1, y1, sx, sy, depth+1 );
						qtc_decompress_ccode_rec( x1, sy, sx, y2, depth+1 );
						qtc_decompress_ccode_rec( sx, y1, x2, sy, depth+1 );
						qtc_decompress_ccode_rec( sx, sy, x2, y2, depth+1 );
					}
					else
					{
						if( x2-x1 > minsize )
						{
							sx = x1 + (x2-x1)/2;

							qtc_decompress_ccode_rec( x1, y1, sx, y2, depth+1 );
							qtc_decompress_ccode_rec( sx, y1, x2, y2, depth+1 );
						}
						else if ( y2-y1 > minsize )
						{
							sy = y1 + (y2-y1)/2;
	
							qtc_decompress_ccode_rec( x1, y1, x2, sy, depth+1 );
							qtc_decompress_ccode_rec( x1, sy, x2, y2, depth+1 );
						}
						else
						{
							if( bgra )
								put_ccode_box( outpixels, x1, x2, y1, y2, input->width, 0x007F0000, 0x00FF0000 );
							else
								put_ccode_box( outpixels, x1, x2, y1, y2, input->width, 0x0000007F, 0x000000FF );
						}
					}
				}
				else
				{
					if( bgra )
						put_ccode_box( outpixels, x1, x2, y1, y2, input->width, 0x007F0000, 0x00FF0000 );
					else
						put_ccode_box( outpixels, x1, x2, y1, y2, input->width, 0x0000007F, 0x000000FF );
				}
			}
			else
			{
				put_ccode_box( outpixels, x1, x2, y1, y2, input->width, 0x00007F00, 0x0000FF00 );
			}
		}
	}

	void qtc_decompress_ccode_dummy_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int sx, sy;
		unsigned char status;

		if( refimage )
			status = databuffer_get_bits( commanddata, 1 );
		else
			status = 1;

		if( status != 0 )
		{
			status = databuffer_get_bits( commanddata, 1 );
			if( status == 0 )
			{
				if( depth < maxdepth )
				{
					if( ( x2-x1 > minsize ) && ( y2-y1 > minsize ) )
					{
						sx = x1 + (x2-x1)/2;
						sy = y1 + (y2-y1)/2;

						qtc_decompress_ccode_dummy_rec( x1, y1, sx, sy, depth+1 );
						qtc_decompress_ccode_dummy_rec( x1, sy, sx, y2, depth+1 );
						qtc_decompress_ccode_dummy_rec( sx, y1, x2, sy, depth+1 );
						qtc_decompress_ccode_dummy_rec( sx, sy, x2, y2, depth+1 );
					}
					else
					{
						if( x2-x1 > minsize )
						{
							sx = x1 + (x2-x1)/2;

							qtc_decompress_ccode_dummy_rec( x1, y1, sx, y2, depth+1 );
							qtc_decompress_ccode_dummy_rec( sx, y1, x2, y2, depth+1 );
						}
						else if ( y2-y1 > minsize )
						{
							sy = y1 + (y2-y1)/2;
	
							qtc_decompress_ccode_dummy_rec( x1, y1, x2, sy, depth+1 );
							qtc_decompress_ccode_dummy_rec( x1, sy, x2, y2, depth+1 );
						}
					}
				}
			}
		}
	}

	if( ! image_create( output, input->width, input->height ) )
		return 0;
	
	commanddata = input->commanddata;
	imagedata = input->imagedata;
	minsize = input->minsize;
	maxdepth = input->maxdepth;

	outpixels = (unsigned int *)output->pixels;

	for( i=0; i<input->width*input->height; i++ )
		outpixels[i] = 0;

	if( ! colordiff )
	{
		qtc_decompress_ccode_rec( 0, 0, input->width, input->height, 0 );
	}
	else
	{
		if( channel == 0 )
		{
			qtc_decompress_ccode_rec( 0, 0, input->width, input->height, 0 );
		}
		else
		{
			qtc_decompress_ccode_dummy_rec( 0, 0, input->width, input->height, 0 );
			qtc_decompress_ccode_rec( 0, 0, input->width, input->height, 0 );
		}
	}

	return 1;
}

