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

#include "libX11.hpp"

#include "Common/SharedLibrary.hpp"

#define Bool int

LibX11exports::LibX11exports(void *libX11, void *libXext)
{
	XOpenDisplay = (Display *(*)(char*))getProcAddress(libX11, "XOpenDisplay");
	XGetWindowAttributes = (Status (*)(Display*, Window, XWindowAttributes*))getProcAddress(libX11, "XGetWindowAttributes");
	XDefaultScreenOfDisplay = (Screen *(*)(Display*))getProcAddress(libX11, "XDefaultScreenOfDisplay");
	XWidthOfScreen = (int (*)(Screen*))getProcAddress(libX11, "XWidthOfScreen");
	XHeightOfScreen = (int (*)(Screen*))getProcAddress(libX11, "XHeightOfScreen");
	XPlanesOfScreen = (int (*)(Screen*))getProcAddress(libX11, "XPlanesOfScreen");
	XDefaultGC = (GC (*)(Display*, int))getProcAddress(libX11, "XDefaultGC");
	XDefaultDepth = (int (*)(Display*, int))getProcAddress(libX11, "XDefaultDepth");
	XMatchVisualInfo = (Status (*)(Display*, int, int, int, XVisualInfo*))getProcAddress(libX11, "XMatchVisualInfo");
	XDefaultVisual = (Visual *(*)(Display*, int screen_number))getProcAddress(libX11, "XDefaultVisual");
	XSetErrorHandler = (int (*(*)(int (*)(Display*, XErrorEvent*)))(Display*, XErrorEvent*))getProcAddress(libX11, "XSetErrorHandler");
	XSync = (int (*)(Display*, Bool))getProcAddress(libX11, "XSync");
	XCreateImage = (XImage *(*)(Display*, Visual*, unsigned int, int, int, char*, unsigned int, unsigned int, int, int))getProcAddress(libX11, "XCreateImage");
	XCloseDisplay = (int (*)(Display*))getProcAddress(libX11, "XCloseDisplay");
	XPutImage = (int (*)(Display*, Drawable, GC, XImage*, int, int, int, int, unsigned int, unsigned int))getProcAddress(libX11, "XPutImage");
	XDrawString = (int (*)(Display*, Drawable, GC, int, int, char*, int))getProcAddress(libX11, "XDrawString");

	XShmQueryExtension = (Bool (*)(Display*))getProcAddress(libXext, "XShmQueryExtension");
	XShmCreateImage = (XImage *(*)(Display*, Visual*, unsigned int, int, char*, XShmSegmentInfo*, unsigned int, unsigned int))getProcAddress(libXext, "XShmCreateImage");
	XShmAttach = (Bool (*)(Display*, XShmSegmentInfo*))getProcAddress(libXext, "XShmAttach");
	XShmDetach = (Bool (*)(Display*, XShmSegmentInfo*))getProcAddress(libXext, "XShmDetach");
	XShmPutImage = (int (*)(Display*, Drawable, GC, XImage*, int, int, int, int, unsigned int, unsigned int, bool))getProcAddress(libXext, "XShmPutImage");
}

LibX11exports *LibX11::operator->()
{
	return loadExports();
}

LibX11exports *LibX11::loadExports()
{
	static void *libX11 = nullptr;
	static void *libXext = nullptr;
	static LibX11exports *libX11exports = nullptr;

	if(!libX11)
	{
		if(getProcAddress(RTLD_DEFAULT, "XOpenDisplay"))   // Search the global scope for pre-loaded X11 library.
		{
			libX11exports = new LibX11exports(RTLD_DEFAULT, RTLD_DEFAULT);
			libX11 = (void*)-1;   // No need to load it.
		}
		else
		{
			libX11 = loadLibrary("libX11.so");

			if(libX11)
			{
				libXext = loadLibrary("libXext.so");
				libX11exports = new LibX11exports(libX11, libXext);
			}
			else
			{
				libX11 = (void*)-1;   // Don't attempt loading more than once.
			}
		}
	}

	return libX11exports;
}

LibX11 libX11;
