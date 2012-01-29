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
* pixels points to the image data                                              *
*******************************************************************************/
struct image
{
	int width, height;
	struct pixel *pixels;
};

extern int image_create( struct image *image, int width, int height );
extern void image_free( struct image *image );
extern void image_copy( struct image *in, struct image *out );

extern void image_color_diff( struct image *image );
extern void image_color_diff_rev( struct image *image );

extern void image_transform_fast( struct image *image );
extern void image_transform_fast_rev( struct image *image );
extern void image_transform( struct image *image );
extern void image_transform_rev( struct image *image );

#endif

