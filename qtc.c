#include <stdlib.h>
#include <stdio.h>

#include "databuffer.h"
#include "qti.h"
#include "image.h"

#include "qtc.h"

static inline void put_rgb_pixel( struct databuffer *databuffer, unsigned int pixel )
{
	databuffer_add_byte( pixel&0xff, databuffer );
	databuffer_add_byte( (pixel>>8)&0xff, databuffer );
	databuffer_add_byte( (pixel>>16)&0xff, databuffer );
}

static inline void put_bgr_pixel( struct databuffer *databuffer, unsigned int pixel )
{
	databuffer_add_byte( (pixel>>16)&0xff, databuffer );
	databuffer_add_byte( (pixel>>8)&0xff, databuffer );
	databuffer_add_byte( pixel&0xff, databuffer );
}

static inline void put_luma_pixel( struct databuffer *databuffer, unsigned int pixel )
{
	databuffer_add_byte( (pixel>>8)&0xff, databuffer );
}

static inline void put_rgb_chroma_pixel( struct databuffer *databuffer, unsigned int pixel )
{
	databuffer_add_byte( pixel&0xff, databuffer );
	databuffer_add_byte( (pixel>>16)&0xff, databuffer );
}

static inline void put_bgr_chroma_pixel( struct databuffer *databuffer, unsigned int pixel )
{
	databuffer_add_byte( (pixel>>16)&0xff, databuffer );
	databuffer_add_byte( pixel&0xff, databuffer );
}

static inline void put_pixels( struct databuffer *databuffer, unsigned int *pixels, int x1, int x2, int y1, int y2, int width, int bgra, int colordiff, int luma )
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


static inline unsigned int get_rgb_pixel( struct databuffer *databuffer )
{
	unsigned int pixel;

	pixel = databuffer_get_byte( databuffer );
	pixel |= databuffer_get_byte( databuffer ) << 8;
	pixel |= databuffer_get_byte( databuffer ) << 16;

	return pixel;
}

static inline unsigned int get_bgr_pixel( struct databuffer *databuffer )
{
	unsigned int pixel;

	pixel = databuffer_get_byte( databuffer ) << 16;
	pixel |= databuffer_get_byte( databuffer ) << 8;
	pixel |= databuffer_get_byte( databuffer );

	return pixel;
}

static inline unsigned int get_luma_pixel( struct databuffer *databuffer )
{
	unsigned int pixel;

	pixel = databuffer_get_byte( databuffer ) << 8;
	
	return pixel;
}

static inline unsigned int get_rgb_chroma_pixel( struct databuffer *databuffer )
{
	unsigned int pixel;

	pixel = databuffer_get_byte( databuffer );
	pixel |= databuffer_get_byte( databuffer ) << 16;

	return pixel;
}

static inline unsigned int get_bgr_chroma_pixel( struct databuffer *databuffer )
{
	unsigned int pixel;

	pixel = databuffer_get_byte( databuffer ) << 16;
	pixel |= databuffer_get_byte( databuffer );

	return pixel;
}

static inline void get_pixels( struct databuffer *databuffer, unsigned int *pixels, int x1, int x2, int y1, int y2, int width, int bgra, int colordiff, int luma )
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
					pixels[i++] = get_bgr_pixel( databuffer );
			}
		}
		else
		{
			for( y=y1; y<y2; y++ )
			{
				i = x1 + y*width;
				for( x=x1; x<x2; x++ )
					pixels[i++] = get_rgb_pixel( databuffer );
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
					pixels[i++] = get_luma_pixel( databuffer );
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
						pixels[i++] |= get_bgr_chroma_pixel( databuffer );
				}
			}
			else
			{
				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*width;
					for( x=x1; x<x2; x++ )
						pixels[i++] |= get_rgb_chroma_pixel( databuffer );
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
			
			p = inpixels[ x1 + y1*input->width ];
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
						put_pixels( imagedata, inpixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma );
					}
				}
			}
			else
			{
				put_pixels( imagedata, inpixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma );
			}
		}
		else
		{
			databuffer_add_bits( 1, commanddata, 1 );

			if( ! colordiff )
			{
				if( bgra )
					put_bgr_pixel( imagedata, p );
				else
					put_rgb_pixel( imagedata, p );
			}
			else
			{
				if( luma )
				{
					put_luma_pixel( imagedata, p );
				}
				else
				{
					if( bgra )
						put_bgr_chroma_pixel( imagedata, p );
					else
						put_rgb_chroma_pixel( imagedata, p );
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


int qtc_decompress( struct qti *input, struct image *refimage, struct image *output, int bgra, int colordiff )
{
	struct databuffer *commanddata, *imagedata;
	int minsize, maxdepth;
	unsigned int *outpixels, *refpixels;
	unsigned int mask;
	int luma;

	void qtc_decompress_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int x, y, sx, sy, i;
		unsigned int color;
		unsigned char status;

		if( refimage != NULL )
			status = databuffer_get_bits( commanddata, 1 );
	
		if( ( refimage != NULL ) && ( status == 0 ) )
		{
			for( y=y1; y<y2; y++ )
			{
				i = x1 + y*input->width;
				for( x=x1; x<x2; x++ )
				{
					outpixels[ i ] = refpixels[ i ];
					i++;
				}
			}
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
							get_pixels( imagedata, outpixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma );
						}
					}
				}
				else
				{
					get_pixels( imagedata, outpixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma );
				}
			}
			else
			{
				if( ! colordiff )
				{
					if( bgra )
						color = get_bgr_pixel( imagedata );
					else
						color = get_rgb_pixel( imagedata );

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
						color = get_luma_pixel( imagedata );

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
						if( bgra )
							color = get_bgr_chroma_pixel( imagedata );
						else
							color = get_rgb_chroma_pixel( imagedata );

						for( y=y1; y<y2; y++ )
						{
							i = x1 + y*input->width;
							for( x=x1; x<x2; x++ )
							{
								outpixels[ i++ ] |= color;
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

	outpixels = (unsigned int *)output->pixels;

	if( refimage != NULL )
		refpixels = (unsigned int *)refimage->pixels;

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

int qtc_decompress_ccode( struct qti *input, struct image *output, int refimage )
{
/*	struct databuffer *commanddata, *imagedata;
	int minsize, maxdepth;
	int i;

	if( ! image_create( output, input->width, input->height ) )
		return 0;
	
	for( i=0; i<input->width*input->height; i++ )
	{
		output->pixels[i].r = output->pixels[i].g = output->pixels[i].b = 0;
	}
	
	commanddata = input->commanddata;
	imagedata = input->imagedata;
	minsize = input->minsize;
	maxdepth = input->maxdepth;

	int qtc_decompress_ccode_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int x, y, sx, sy;
		unsigned char status;

		for( y=y1; y<y2; y++ )
		for( x=x1; x<x2; x++ )
		{
			output->pixels[ x + y*output->width  ].r += 64/maxdepth;
			output->pixels[ x + y*output->width  ].g += 64/maxdepth;
			output->pixels[ x + y*output->width  ].b += 64/maxdepth;
		}

		if( refimage )
			status = databuffer_get_bits( commanddata, 1 );
	
		if( ( refimage ) && ( status == 0 ) )
		{
			for( y=y1; y<y2; y++ )
			for( x=x1; x<x2; x++ )
			{
				if( ( x == x1 ) || ( y == y1 ) )
					output->pixels[ x + y*output->width  ].b = 255;
				else
					output->pixels[ x + y*output->width  ].b = 128;
			}
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
							for( y=y1; y<y2; y++ )
							for( x=x1; x<x2; x++ )
							{
								if( ( x == x1 ) || ( y == y1 ) )
									output->pixels[ x + y*output->width  ].r = 255;
								else
									output->pixels[ x + y*output->width  ].r = 128;
							}
						}
					}
				}
				else
				{
					for( y=y1; y<y2; y++ )
					for( x=x1; x<x2; x++ )
					{
						if( ( x == x1 ) || ( y == y1 ) )
							output->pixels[ x + y*output->width  ].r = 255;
						else
							output->pixels[ x + y*output->width  ].r = 128;
					}
				}
			}
			else
			{
				for( y=y1; y<y2; y++ )
				for( x=x1; x<x2; x++ )
				{
					if( ( x == x1 ) || ( y == y1 ) )
						output->pixels[ x + y*output->width  ].g = 255;
					else
						output->pixels[ x + y*output->width  ].g = 128;
				}
			}
		}
	
		return 1;
	}
	
	return qtc_decompress_ccode_rec( 0, 0, input->width, input->height, 0 );*/
}

