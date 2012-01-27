#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xfixes.h>

#include "image.h"

#include "x11grab.h"

int x11grabber_create( struct x11grabber *grabber, char *disp_name, int x, int y, int width, int height, int mouse )
{
	Display *display;
	int screen, cap_w, cap_h;
	XImage *image;
	XWindowAttributes screeninfo;

	display = XOpenDisplay( disp_name );
	if( display == NULL )
	{
		fputs( "x11grabber_create: Could not open display\n", stderr );
		return 0;
	}

	if( ! XShmQueryExtension( display ) )
	{
		fputs( "x11grabber_create: XShm not supported\n", stderr );
		XCloseDisplay( display );
		return 0;
	}

	screen = XDefaultScreen( display );

	if( ! XGetWindowAttributes( display, RootWindow( display, screen ), &screeninfo ) )
	{
		fputs( "x11grabber_create: Cannot get root window attributes\n", stderr );
		XCloseDisplay( display );
		return 0;
	}

	if( ( width == -1 ) && ( height == -1 ) )
	{
		cap_w = screeninfo.width;
		cap_h = screeninfo.height;
	}
	else
	{
		cap_w = width;
		cap_h = height;
	}

	if ( ( cap_w+x > screeninfo.width ) || ( cap_h+y > screeninfo.height ) || ( x < 0 ) || ( y < 0 ) )
	{
		fputs( "x11grabber_create: Trying to capture outside screen\n", stderr );
		XCloseDisplay( display );
		return 0;
	}

	image = XShmCreateImage( display,
								DefaultVisual( display, screen ),
								DefaultDepth( display, screen ),
								ZPixmap,
								NULL,
								&grabber->shminfo,
								cap_w, cap_h );

	if( image == NULL )
	{
		fputs( "x11grabber_create: Cannot create SHM image\n", stderr );
		XCloseDisplay( display );
        return 0;
	}

	if( image->bits_per_pixel != 32 )
	{
		fputs( "x11grabber_create: Unsupported bitdepth\n", stderr );
		XDestroyImage( image );
        XCloseDisplay( display );
		return 0;
	}

	grabber->shminfo.shmid = shmget( IPC_PRIVATE,
							image->bytes_per_line * image->height,
							IPC_CREAT|0777 );

    if ( grabber->shminfo.shmid < 0 )
    {
        fputs( "create_x11grabber: Cannot get system shared memory\n", stderr );
        XDestroyImage( image );
        XCloseDisplay( display );
        return 0;
    }

	grabber->shminfo.shmaddr = image->data = shmat( grabber->shminfo.shmid, 0, 0 );
	if( grabber->shminfo.shmaddr == (char *) -1 )
	{
		fputs( "x11grabber_create: Cannot attach to system shared memory\n", stderr );
		XDestroyImage( image );
        XCloseDisplay( display );
		return 0;
	}

	grabber->shminfo.readOnly = False;

	if ( ! XShmAttach( display, &grabber->shminfo ) )
	{
		fputs( "x11grabber_create: Cannot attach to X shared memory\n", stderr );
		shmdt( grabber->shminfo.shmaddr);
		XDestroyImage( image );
        XCloseDisplay( display );
		return 0;
	}

	grabber->display = display;
	grabber->screen = screen;
	grabber->x = x;
	grabber->y = y;
	grabber->width = cap_w;
	grabber->height = cap_h;
	grabber->mouse = mouse;
	grabber->image = image;

	return 1;
}

void x11grabber_free( struct x11grabber *grabber )
{
    XShmDetach( grabber->display, &grabber->shminfo );
    shmdt( grabber->shminfo.shmaddr );

    XDestroyImage( grabber->image );

	XCloseDisplay( grabber->display );
}

int x11grabber_grab_frame( struct image *image, struct x11grabber *grabber )
{
	int x, y, cx, cy, i, ci;
	int xmin, xmax, ymin, ymax;
	unsigned char alpha;
	XFixesCursorImage *xcim = NULL;

	if( grabber->mouse )
	{
		xcim = XFixesGetCursorImage( grabber->display );
		if( xcim == NULL )
		{
			fputs( "x11grabber_grab_frame: Could not get mouse cursor\n", stderr );
			return 0;
		}
	}

	if ( ! XShmGetImage( grabber->display, RootWindow( grabber->display, grabber->screen ), grabber->image, grabber->x, grabber->y, AllPlanes ) )
	{
		fputs( "x11grabber_grab_frame: Could not get image\n", stderr );
		return 0;
	}

	image_create( image, grabber->width, grabber->height );

	memcpy( image->pixels, grabber->image->data, image->width * image->height * 4 );

	if( xcim )
	{
		cx = xcim->x - xcim->xhot - grabber->x;
		cy = xcim->y - xcim->yhot - grabber->y;

		xmin = cx<0?0:cx;
		xmax = ((cx + xcim->width)<grabber->width)?(cx + xcim->width):grabber->width;
		ymin = cy<0?0:cy;
		ymax = ((cy + xcim->height)<grabber->height)?(cy + xcim->height):grabber->height;

		for( y=ymin; y<ymax; y++ )
		{
			i = xmin+y*image->width;
			ci = (xmin-cx) + (y-cy)*xcim->width;

			for( x=xmin; x<xmax; x++ )
			{
				alpha = xcim->pixels[ci] >> 24 & 0xff;

				if( alpha != 0 )
				{
					if( alpha == 255 )
					{
						image->pixels[i].x = xcim->pixels[ci] >>  0 & 0xff;
						image->pixels[i].y = xcim->pixels[ci] >>  8 & 0xff;
						image->pixels[i].z = xcim->pixels[ci] >> 16 & 0xff;
					}
					else
					{
						image->pixels[i].x = (image->pixels[i].x*(255-alpha)/255) + ((xcim->pixels[ci] >>  0 & 0xff)*alpha/255);
						image->pixels[i].y = (image->pixels[i].y*(255-alpha)/255) + ((xcim->pixels[ci] >>  8 & 0xff)*alpha/255);
						image->pixels[i].z = (image->pixels[i].z*(255-alpha)/255) + ((xcim->pixels[ci] >> 16 & 0xff)*alpha/255);
					}
				}
			
				i++;
				ci++;
			}
		}
	}

	return 1;
}

