#ifndef PPM_H
#define PPM_H

extern int ppm_read( struct image *image, char filename[] );
extern int ppm_write( struct image *image, char filename[] );

#endif

