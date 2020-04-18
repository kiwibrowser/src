// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef libX11_hpp
#define libX11_hpp

#define Bool int
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

struct LibX11exports
{
	LibX11exports(void *libX11, void *libXext);

	Display *(*XOpenDisplay)(char *display_name);
	Status (*XGetWindowAttributes)(Display *display, Window w, XWindowAttributes *window_attributes_return);
	Screen *(*XDefaultScreenOfDisplay)(Display *display);
	int (*XWidthOfScreen)(Screen *screen);
	int (*XHeightOfScreen)(Screen *screen);
	int (*XPlanesOfScreen)(Screen *screen);
	GC (*XDefaultGC)(Display *display, int screen_number);
	int (*XDefaultDepth)(Display *display, int screen_number);
	Status (*XMatchVisualInfo)(Display *display, int screen, int depth, int screen_class, XVisualInfo *vinfo_return);
	Visual *(*XDefaultVisual)(Display *display, int screen_number);
	int (*(*XSetErrorHandler)(int (*handler)(Display*, XErrorEvent*)))(Display*, XErrorEvent*);
	int (*XSync)(Display *display, Bool discard);
	XImage *(*XCreateImage)(Display *display, Visual *visual, unsigned int depth, int format, int offset, char *data, unsigned int width, unsigned int height, int bitmap_pad, int bytes_per_line);
	int (*XCloseDisplay)(Display *display);
	int (*XPutImage)(Display *display, Drawable d, GC gc, XImage *image, int src_x, int src_y, int dest_x, int dest_y, unsigned int width, unsigned int height);
	int (*XDrawString)(Display *display, Drawable d, GC gc, int x, int y, char *string, int length);

	Bool (*XShmQueryExtension)(Display *display);
	XImage *(*XShmCreateImage)(Display *display, Visual *visual, unsigned int depth, int format, char *data, XShmSegmentInfo *shminfo, unsigned int width, unsigned int height);
	Bool (*XShmAttach)(Display *display, XShmSegmentInfo *shminfo);
	Bool (*XShmDetach)(Display *display, XShmSegmentInfo *shminfo);
	int (*XShmPutImage)(Display *display, Drawable d, GC gc, XImage *image, int src_x, int src_y, int dest_x, int dest_y, unsigned int width, unsigned int height, bool send_event);
};

#undef Bool

class LibX11
{
public:
	operator bool()
	{
		return loadExports();
	}

	LibX11exports *operator->();

private:
	LibX11exports *loadExports();
};

extern LibX11 libX11;

#endif   // libX11_hpp
