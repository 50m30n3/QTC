#ifndef X11GRAB_H
#define X11GRAB_H

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

struct x11grabber
{
	Display *display;
	int screen, x, y, width, height;
	int mouse;
	XImage *image;
	XShmSegmentInfo shminfo;
};

extern int x11grabber_create( struct x11grabber *grabber, char *disp_name, int x, int y, int width, int height, int mouse );
extern void x11grabber_free( struct x11grabber *grabber );
extern int x11grabber_grab_frame( struct image *image, struct x11grabber *grabber );

#endif

