#include <stdlib.h>
#include <stdio.h>

#include "databuffer.h"
#include "qti.h"
#include "image.h"

#include "qtc.h"

int qtc_compress( struct image *input, struct image *refimage, struct qti *output, int maxerror, int minsize, int maxdepth, int lazyness )
{
	struct databuffer *commanddata, *imagedata;

	if( ! qti_create( input->width, input->height, minsize, maxdepth, output ) )
		return 0;
	
	commanddata = output->commanddata;
	imagedata = output->imagedata;

	void qtc_compress_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int x, y, sx, sy, i;
		struct pixel p1, p2;
		struct color avgcolor;
		int error;

		if( depth >= lazyness )
		{
			error = 0;

			if( refimage != NULL )
			{
				if( maxerror == 0 )
				{
					for( y=y1; y<y2; y++ )
					{
						i = x1 + y*input->width;
						for( x=x1; x<x2; x++ )
						{
							p1 = input->pixels[ i ];
							p2 = refimage->pixels[ i ];

							if( ( p1.r != p2.r ) || ( p1.g != p2.g ) || ( p1.b != p2.b ) )
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
				else
				{
					for( y=y1; y<y2; y++ )
					{
						i = x1 + y*input->width;

						for( x=x1; x<x2; x++ )
						{
							p1 = input->pixels[ i ];
							p2 = refimage->pixels[ i ];
							
							error += abs( p1.r - p2.r );
							error += abs( p1.g - p2.g );
							error += abs( p1.b - p2.b );

							if( error > maxerror )
								break;

							i++;
						}
				
						if( error > maxerror )
							break;
					}
			
					if( error > maxerror )
					{
						databuffer_add_bits( 1, commanddata, 1 );
					}
					else
					{
						databuffer_add_bits( 0, commanddata, 1 );
						return;
					}
				}
			}


			error = 0;

			if( maxerror == 0 )
			{
				p1 = input->pixels[ x1 + y1*input->width ];

				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*input->width;
					for( x=x1; x<x2; x++ )
					{
						p2 = input->pixels[ i++ ];

						if( ( p1.r != p2.r ) || ( p1.g != p2.g ) || ( p1.b != p2.b ) )
						{
							error = 1;
							break;
						}

						if( error )
							break;
					}
	
					if( error )
						break;
				}
			}
			else
			{
				avgcolor.r = 0;
				avgcolor.g = 0;
				avgcolor.b = 0;

				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*input->width;
					for( x=x1; x<x2; x++ )
					{
						p1 = input->pixels[ i++ ];
					
						avgcolor.r += p1.r;
						avgcolor.g += p1.g;
						avgcolor.b += p1.b;
					}
				}

				p1.r = avgcolor.r / ((x2-x1)*(y2-y1));
				p1.g = avgcolor.g / ((x2-x1)*(y2-y1));
				p1.b = avgcolor.b / ((x2-x1)*(y2-y1));

//				p1 = input->pixels[ (x1+(x2-x1)/2) + (y1+(y2-y1)/2)*input->width ];

				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*input->width;

					for( x=x1; x<x2; x++ )
					{
						p2 = input->pixels[ i++ ];
						
						error += abs( p1.r - p2.r );
						error += abs( p1.g - p2.g );
						error += abs( p1.b - p2.b );

						if( error > maxerror )
							break;
			
					}

					if( error > maxerror )
						break;
				}
			}

		}
		else
		{
			if( refimage != NULL )
				databuffer_add_bits( 1, commanddata, 1 );

			error = maxerror+1;
			
			p1 = input->pixels[ x1 + y1*input->width ];
		}

		if( error > maxerror )
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
						for( y=y1; y<y2; y++ )
						{
							i = x1 + y*input->width;
							
							for( x=x1; x<x2; x++ )
							{
								p1 = input->pixels[ i++ ];
								databuffer_add_byte( p1.r, imagedata );
								databuffer_add_byte( p1.g, imagedata );
								databuffer_add_byte( p1.b, imagedata );
							}
						}
					}
				}
			}
			else
			{
				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*input->width;
					for( x=x1; x<x2; x++ )
					{
						p1 = input->pixels[ i++ ];
						databuffer_add_byte( p1.r, imagedata );
						databuffer_add_byte( p1.g, imagedata );
						databuffer_add_byte( p1.b, imagedata );
					}
				}
			}
		}
		else
		{
			databuffer_add_bits( 1, commanddata, 1 );
			databuffer_add_byte( p1.r, imagedata );
			databuffer_add_byte( p1.g, imagedata );
			databuffer_add_byte( p1.b, imagedata );
		}
	}
	
	qtc_compress_rec( 0, 0, input->width, input->height, 0 );
	
	return 1;
}

int qtc_decompress( struct qti *input, struct image *refimage, struct image *output )
{
	struct databuffer *commanddata, *imagedata;
	int minsize, maxdepth;

	if( ! image_create( output, input->width, input->height ) )
		return 0;
	
	commanddata = input->commanddata;
	imagedata = input->imagedata;
	minsize = input->minsize;
	maxdepth = input->maxdepth;

	int qtc_decompress_rec( int x1, int y1, int x2, int y2, int depth )
	{
		int x, y, sx, sy, i;
		struct pixel color;
		unsigned char status;

		if( refimage != NULL )
			status = databuffer_get_bits( commanddata, 1 );
	
		if( ( refimage != NULL ) && ( status == 0 ) )
		{
			for( x=x1; x<x2; x++ )
			for( y=y1; y<y2; y++ )
			{
				i = x + y*output->width;
				output->pixels[ i ] = refimage->pixels[ i ];
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
							for( y=y1; y<y2; y++ )
							{
								i = x1 + y*input->width;
								for( x=x1; x<x2; x++ )
								{
									output->pixels[ i ].r = databuffer_get_byte( imagedata );
									output->pixels[ i ].g = databuffer_get_byte( imagedata );
									output->pixels[ i ].b = databuffer_get_byte( imagedata );
									i++;
								}
							}
						}
					}
				}
				else
				{
					for( y=y1; y<y2; y++ )
					{
						i = x1 + y*input->width;
						for( x=x1; x<x2; x++ )
						{
							output->pixels[ i ].r = databuffer_get_byte( imagedata );
							output->pixels[ i ].g = databuffer_get_byte( imagedata );
							output->pixels[ i ].b = databuffer_get_byte( imagedata );
							i++;
						}
					}
				}
			}
			else
			{
				color.r = databuffer_get_byte( imagedata );
				color.g = databuffer_get_byte( imagedata );
				color.b = databuffer_get_byte( imagedata );

				for( y=y1; y<y2; y++ )
				{
					i = x1 + y*input->width;
					for( x=x1; x<x2; x++ )
					{
						output->pixels[ i++ ] = color;
					}
				}
			}
		}
	
		return 1;
	}
	
	return qtc_decompress_rec( 0, 0, input->width, input->height, 0 );
}

int qtc_decompress_ccode( struct qti *input, struct image *output, int refimage )
{
	struct databuffer *commanddata, *imagedata;
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
								databuffer_get_bits( imagedata, 8 );
								databuffer_get_bits( imagedata, 8 );
								databuffer_get_bits( imagedata, 8 );

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
						databuffer_get_bits( imagedata, 8 );
						databuffer_get_bits( imagedata, 8 );
						databuffer_get_bits( imagedata, 8 );

						if( ( x == x1 ) || ( y == y1 ) )
							output->pixels[ x + y*output->width  ].r = 255;
						else
							output->pixels[ x + y*output->width  ].r = 128;
					}
				}
			}
			else
			{
				databuffer_get_bits( imagedata, 8 );
				databuffer_get_bits( imagedata, 8 );
				databuffer_get_bits( imagedata, 8 );

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
	
	return qtc_decompress_ccode_rec( 0, 0, input->width, input->height, 0 );
}

