#ifndef IMAGE_H
#define IMAGE_H

struct color
{
	int r;
	int g;
	int b;
};

struct pixel
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct image
{
	unsigned int width, height;
	struct pixel *pixels;
};

extern int create_image( struct image *image, unsigned int width, unsigned int height );
extern void free_image( struct image *image );
extern void copy_image( struct image *in, struct image *out );
extern void transform_image_fast( struct image *image );
extern void transform_image_fast_rev( struct image *image );
extern void transform_image( struct image *image );
extern void transform_image_rev( struct image *image );


#endif

