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

// main.h: Management of thread-local data.

#ifndef LIBGL_MAIN_H_
#define LIBGL_MAIN_H_

#include "Context.h"
#include "Device.hpp"
#include "common/debug.h"
#include "Display.h"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

namespace gl
{
	struct Current
	{
		Context *context;
		Display *display;
		Surface *drawSurface;
		Surface *readSurface;
	};

	void makeCurrent(Context *context, Display *display, Surface *surface);

	Context *getContext();
	Display *getDisplay();
	Device *getDevice();
	Surface *getCurrentDrawSurface();
	Surface *getCurrentReadSurface();

	void setCurrentDisplay(Display *dpy);
	void setCurrentContext(gl::Context *ctx);
	void setCurrentDrawSurface(Surface *surface);
	void setCurrentReadSurface(Surface *surface);
}

void error(GLenum errorCode);

template<class T>
T &error(GLenum errorCode, T &returnValue)
{
	error(errorCode);

	return returnValue;
}

template<class T>
const T &error(GLenum errorCode, const T &returnValue)
{
	error(errorCode);

	return returnValue;
}

extern sw::FrameBuffer *createFrameBuffer(void *display, NativeWindowType window, int width, int height);

#endif   // LIBGL_MAIN_H_
