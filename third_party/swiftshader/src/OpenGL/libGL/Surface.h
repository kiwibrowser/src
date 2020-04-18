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

// Surface.h: Defines the Surface class, representing a drawing surface
// such as the client area of a window, including any back buffers.

#ifndef INCLUDE_SURFACE_H_
#define INCLUDE_SURFACE_H_

#include "Main/FrameBuffer.hpp"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

#if defined(_WIN32)
typedef HDC     NativeDisplayType;
typedef HBITMAP NativePixmapType;
typedef HWND    NativeWindowType;
#else
#error
#endif

namespace gl
{
class Image;
class Display;

class Surface
{
public:
	Surface(Display *display, NativeWindowType window);
	Surface(Display *display, GLint width, GLint height, GLenum textureFormat, GLenum textureTarget);

	virtual ~Surface();

	bool initialize();
	void swap();

	virtual Image *getRenderTarget();
	virtual Image *getDepthStencil();

	void setSwapInterval(GLint interval);

	virtual GLint getWidth() const;
	virtual GLint getHeight() const;
	virtual GLenum getTextureFormat() const;
	virtual GLenum getTextureTarget() const;

	bool checkForResize();   // Returns true if surface changed due to resize

private:
	void release();
	bool reset();

	Display *const mDisplay;
	Image *mDepthStencil;
	sw::FrameBuffer *frameBuffer;
	Image *backBuffer;

	bool reset(int backbufferWidth, int backbufferHeight);

	const NativeWindowType mWindow;   // Window that the surface is created for.
	bool mWindowSubclassed;           // Indicates whether we successfully subclassed mWindow for WM_RESIZE hooking
	GLint mWidth;                     // Width of surface
	GLint mHeight;                    // Height of surface
	GLenum mTextureFormat;            // Format of texture: RGB, RGBA, or no texture
	GLenum mTextureTarget;            // Type of texture: 2D or no texture
	GLint mSwapInterval;
};
}

#endif   // INCLUDE_SURFACE_H_
