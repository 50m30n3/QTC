/*
*    QTC: qtc.c (c) 2011, 2012 50m30n3
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

#include "databuffer.h"
#include "qti.h"
#include "tilecache.h"
#include "image.h"

#include "qtc.h"


/*******************************************************************************
* Functions to wite pixel data to a databuffer from various source formats     *
*                                                                              *
* databuffer is the databuffer to write to                                     *
* pixel is the pixel to write                                                  *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
static inline int put_rgb_pixel( struct databuffer *databuffer, struct pixel pixel )
{
	return ( ( databuffer_add_byte( pixel.x, databuffer ) ) &&
			 ( databuffer_add_byte( pixel.y, databuffer ) ) &&
			 ( databuffer_add_byte( pixel.z, databuffer ) ) );
}

static inline int put_bgr_pixel( struct databuffer *databuffer, struct pixel pixel )
{
	return ( ( databuffer_add_byte( pixel.z, databuffer ) ) &&
			 ( databuffer_add_byte( pixel.y, databuffer ) ) &&
			 ( databuffer_add_byte( pixel.x, databuffer ) ) );
}

static inline int put_luma_pixel( struct databuffer *databuffer, struct pixel pixel )
{
	return databuffer_add_byte( pixel.y, databuffer );
}

static inline int put_rgb_chroma_pixel( struct databuffer *databuffer, struct pixel pixel )
{
	return ( ( databuffer_add_byte( pixel.x, databuffer ) ) &&
			 ( databuffer_add_byte( pixel.z, databuffer ) ) );
}

static inline int put_bgr_chroma_pixel( struct databuffer *databuffer, struct pixel pixel )
{
	return ( ( databuffer_add_byte( pixel.z, databuffer ) ) &&
			 ( databuffer_add_byte( pixel.x, databuffer ) ) );
}


/*******************************************************************************
* Function to write pixel data from an image area to a databuffer              *
*                                                                              *
* databuffer is the databuffer to write to                                     *
* pixels is an array containing the complete image data                        *
* x1, x2, y1, y2 describe the sub-area to write                                *
* with is the width of the complete image                                      *
* bgra decides wether to use bgra mode (1) or rgba mode (0)                    *
* colordiff decides wether the image data is in fakeyuv format                 *
* luma decidec wether to write the luma (1) or chroma (0) channel              *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
static inline int put_pixels( struct databuffer *databuffer, struct pixel *pixels, int x1, int x2, int y1, int y2, int width, int bgra, int colordiff, int luma )
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
					if( ! put_bgr_pixel( databuffer, pixels[i++] ) )
						return 0;
			}
		}
		else
		{
			for( y=y1; y<y2; y++ )
			{
				i = x1 + y*width;
				for( x=x1; x<x2; x++ )
					if( ! put_rgb_pixel( databuffer, pixels[i++] ) )
						return 0;
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
					if( ! put_luma_pixel( databuffer, pixels[i++] ) )
						return 0;
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
						if( ! put_bgr_chroma_pixel( databuffer, pixels[i++] ) )
							return 0;
				}
			}
			else
			{
				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*width;
					for( x=x1; x<x2; x++ )
						if( ! put_rgb_chroma_pixel( databuffer, pixels[i++] ) )
							return 0;
				}
			}
		}
	}

	return 1;
}

/*******************************************************************************
* Function to compress an image using quad tree compression                    *
*                                                                              *
* input is the input image                                                     *
* refimage is the reference image, set to NULL for keyframes                   *
* output is the compressed image                                               *
* lazyness indicates how many levels to skip at the beginning                  *
* colordiff enables splitting of channels for colordiff images                 *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtc_compress( struct image *input, struct image *refimage, struct qti *output, int lazyness, int colordiff )
{
	struct databuffer *commanddata, *imagedata, *indexdata;
	int minsize, maxdepth;
	unsigned int *inpixels, *refpixels;
	unsigned int mask;
	int luma, bgra;

	int qtc_compress_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int x, y, sx, sy, i;
		unsigned int p;
		struct pixel color;
		int index;
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
					return 1;
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

					if( ( ! qtc_compress_rec( x1, y1, sx, sy, depth+1 ) ) ||
					    ( ! qtc_compress_rec( x1, sy, sx, y2, depth+1 ) ) ||
					    ( ! qtc_compress_rec( sx, y1, x2, sy, depth+1 ) ) ||
					    ( ! qtc_compress_rec( sx, sy, x2, y2, depth+1 ) ) )
					{
						return 0;
					}
				}
				else
				{
					if( x2-x1 > minsize )
					{
						sx = x1 + (x2-x1)/2;
		
						if( ( ! qtc_compress_rec( x1, y1, sx, y2, depth+1 ) ) ||
						    ( ! qtc_compress_rec( sx, y1, x2, y2, depth+1 ) ) )
						{
							return 0;
						}
					}
					else if ( y2-y1 > minsize )
					{
						sy = y1 + (y2-y1)/2;
		
						if( ( ! qtc_compress_rec( x1, y1, x2, sy, depth+1 ) ) ||
						    ( ! qtc_compress_rec( x1, sy, x2, y2, depth+1 ) ) )
						{
							return 0;
						}
					}
					else
					{
						if( output->tilecache != NULL )
						{
							index = tilecache_write( output->tilecache, inpixels, x1, x2, y1, y2, input->width, mask );

							if( index < 0 )
							{
								databuffer_add_bits( 1, commanddata, 1 );

								if( ! put_pixels( imagedata, input->pixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma ) )
									return 0;
							}
							else
							{
								databuffer_add_bits( 0, commanddata, 1 );
								
								databuffer_add_bits( index, indexdata, 32 );
							}
						}
						else
						{
							if( ! put_pixels( imagedata, input->pixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma ) )
								return 0;
						}
					}
				}
			}
			else
			{
				if( ! put_pixels( imagedata, input->pixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma ) )
					return 0;
			}
		}
		else
		{
			databuffer_add_bits( 1, commanddata, 1 );

			color = input->pixels[ x1 + y1*input->width ];

			if( ! colordiff )
			{
				if( bgra )
				{
					if( ! put_bgr_pixel( imagedata, color ) )
						return 0;
				}
				else
				{
					if( ! put_rgb_pixel( imagedata, color ) )
						return 0;
				}
			}
			else
			{
				if( luma )
				{
					if( ! put_luma_pixel( imagedata, color ) )
						return 0;
				}
				else
				{
					if( bgra )
					{
						if( ! put_bgr_chroma_pixel( imagedata, color ) )
							return 0;
					}
					else
					{
						if( ! put_rgb_chroma_pixel( imagedata, color ) )
							return 0;
					}
				}
			}
		}

		return 1;
	}
	
	commanddata = output->commanddata;
	imagedata = output->imagedata;
	indexdata = output->indexdata;
	minsize = output->minsize;
	maxdepth = output->maxdepth;

	bgra = input->bgra;

	output->transform = input->transform;
	
	if( colordiff )
		output->colordiff = 2;
	else
		output->colordiff = input->colordiff;

	inpixels = (unsigned int *)input->pixels;

	if( refimage != NULL )
	{
		refpixels = (unsigned int *)refimage->pixels;
		output->keyframe = 0;
	}
	else
	{
		output->keyframe = 1;
	}

	if( ! colordiff )
	{
		mask = 0x00FFFFFF;
		luma = 0;
		if( ! qtc_compress_rec( 0, 0, input->width, input->height, 0 ) )
			return 0;
	}
	else
	{
		mask = 0x0000FF00;
		luma = 1;
		if( ! qtc_compress_rec( 0, 0, input->width, input->height, 0 ) )
			return 0;

		mask = 0x00FF00FF;
		luma = 0;
		if( ! qtc_compress_rec( 0, 0, input->width, input->height, 0 ) )
			return 0;
	}
	
	return 1;
}


/*******************************************************************************
* Function to write pixel data from a databuffer to an image area              *
*                                                                              *
* imagedata is the databuffer to read from                                     *
* pixels is an array containing the complete image                             *
* x1, x2, y1, y2 describe the sub-area to write                                *
* with is the width of the complete image                                      *
* bgra decides wether to use bgra mode (1) or rgba mode (0)                    *
* colordiff decides wether the image data is in fakeyuv format                 *
* luma decidec wether to write the luma (1) or chroma (0) channel              *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
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

/*******************************************************************************
* Function to decompress an image compressed using quad tree compression       *
*                                                                              *
* input is the compressed input image                                          *
* refimage is the reference image, set to NULL for keyframes                   *
* output is the uncompressed image                                             *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtc_decompress( struct qti *input, struct image *refimage, struct image *output )
{
	struct databuffer *commanddata, *imagedata, *indexdata;
	int minsize, maxdepth;
	int keyframe;
	struct pixel *outpixels;
	unsigned int mask;
	int luma, bgra, colordiff;

	void qtc_decompress_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int x, y, sx, sy, i;
		struct pixel color;
		unsigned char status;
		int index;

		if( keyframe )
			status = 1;
		else
			status = databuffer_get_bits( commanddata, 1 );

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
							if( input->tilecache != NULL )
							{
								status = databuffer_get_bits( commanddata, 1 );
								if( status == 1 )
								{
									get_pixels( imagedata, outpixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma );
									tilecache_add( input->tilecache, (unsigned int *)outpixels, x1, x2, y1, y2, input->width, mask );
								}
								else
								{
									index = databuffer_get_bits( indexdata, 32 );
									tilecache_read( input->tilecache, (unsigned int *)outpixels, index, x1, x2, y1, y2, input->width, mask );
								}
							}
							else
							{
								get_pixels( imagedata, outpixels, x1, x2, y1, y2, input->width, bgra, colordiff, luma );
							}
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
	
	commanddata = input->commanddata;
	imagedata = input->imagedata;
	indexdata = input->indexdata;
	minsize = input->minsize;
	maxdepth = input->maxdepth;
	colordiff = input->colordiff == 2;
	keyframe = input->keyframe;

	bgra = output->bgra;

	output->transform = input->transform;

	if( input->colordiff >= 1 )
		output->colordiff = 1;

	outpixels = output->pixels;

	if( ( !keyframe ) && ( refimage != NULL ) )
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


/*******************************************************************************
* Function to draw a transparent box with outlines into an image               *
*                                                                              *
* pixels is an array containing the complete image                             *
* x1, x2, y1, y2 describe the position of the box                              *
* with is the width of the complete image                                      *
* color is the color of the box, as unsigned int                               *
* linecolor is the color of the box outline                                    *
*******************************************************************************/
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

/*******************************************************************************
* Function to create an analysis image from a compressed image                 *
*                                                                              *
* input is the compressed input image                                          *
* output is the analysis image                                                 *
* channel decides wether to decode the luma or chroma channel in fakeyuv mode  *
*                                                                              *
* Returns 0 on failure, 1 on success                                           *
*******************************************************************************/
int qtc_decompress_ccode( struct qti *input, struct image *output, int channel )
{
	struct databuffer *commanddata;
	int minsize, maxdepth;
	int keyframe;
	struct pixel *outpixels;
	int bgra, colordiff;

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
				outpixels[ i ].x += color;
				outpixels[ i ].y += color;
				outpixels[ i ].z += color;
				i++;
			}
		}

		if( keyframe )
			status = 1;
		else
			status = databuffer_get_bits( commanddata, 1 );

		if( status == 0 )
		{
			if( bgra )
				put_ccode_box( (unsigned int *)outpixels, x1, x2, y1, y2, input->width, 0x0000007F, 0x000000FF );
			else
				put_ccode_box( (unsigned int *)outpixels, x1, x2, y1, y2, input->width, 0x007F0000, 0x00FF0000 );
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
								put_ccode_box( (unsigned int *)outpixels, x1, x2, y1, y2, input->width, 0x007F0000, 0x00FF0000 );
							else
								put_ccode_box( (unsigned int *)outpixels, x1, x2, y1, y2, input->width, 0x0000007F, 0x000000FF );
						}
					}
				}
				else
				{
					if( bgra )
						put_ccode_box( (unsigned int *)outpixels, x1, x2, y1, y2, input->width, 0x007F0000, 0x00FF0000 );
					else
						put_ccode_box( (unsigned int *)outpixels, x1, x2, y1, y2, input->width, 0x0000007F, 0x000000FF );
				}
			}
			else
			{
				put_ccode_box( (unsigned int *)outpixels, x1, x2, y1, y2, input->width, 0x00007F00, 0x0000FF00 );
			}
		}
	}

	void qtc_decompress_ccode_dummy_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int sx, sy;
		unsigned char status;

		if( keyframe )
			status = 1;
		else
			status = databuffer_get_bits( commanddata, 1 );

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
	
	commanddata = input->commanddata;
	minsize = input->minsize;
	maxdepth = input->maxdepth;
	colordiff = input->colordiff == 2;
	keyframe = input->keyframe;

	bgra = output->bgra;

	output->transform = 0;
	output->colordiff = 0;

	outpixels = output->pixels;
	
	memset( outpixels, 0, input->width*input->height*4 );

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

