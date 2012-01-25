#ifndef IMAGE_H
#define IMAGE_H

struct image
{
	int width, height;
	unsigned char *pixels;
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

