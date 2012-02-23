/*
*    QTC: x11grab.h (c) 2011, 2012 50m30n3
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

#ifndef X11GRAB_H
#define X11GRAB_H

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

/*******************************************************************************
* Structure to hold all the data associated with an X11 grabber                *
*                                                                              *
* display is the X display to capture from                                     *
* screen is the X screen to capture from                                       *
* x and y are the offset of the capture window                                 *
* width and height are the width an dheight of the capture window              *
* mouse indicates wether to capture the mouse cursor or not                    *
* image is the XSHM image representing the capture aread                       *
* shminfo is the shared memory info for the image                              *
*******************************************************************************/
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

