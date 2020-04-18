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

// Framebuffer.h: Defines the Framebuffer class. Implements GL framebuffer
// objects and related functionality.

#ifndef LIBGL_FRAMEBUFFER_H_
#define LIBGL_FRAMEBUFFER_H_

#include "common/Object.hpp"
#include "Image.hpp"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

namespace gl
{
class Renderbuffer;
class Colorbuffer;
class Depthbuffer;
class Stencilbuffer;
class DepthStencilbuffer;

class Framebuffer
{
public:
	Framebuffer();

	virtual ~Framebuffer();

	void setColorbuffer(GLenum type, GLuint colorbuffer);
	void setDepthbuffer(GLenum type, GLuint depthbuffer);
	void setStencilbuffer(GLenum type, GLuint stencilbuffer);

	void detachTexture(GLuint texture);
	void detachRenderbuffer(GLuint renderbuffer);

	Image *getRenderTarget();
	Image *getDepthStencil();

	Renderbuffer *getColorbuffer();
	Renderbuffer *getDepthbuffer();
	Renderbuffer *getStencilbuffer();

	GLenum getColorbufferType();
	GLenum getDepthbufferType();
	GLenum getStencilbufferType();

	GLuint getColorbufferName();
	GLuint getDepthbufferName();
	GLuint getStencilbufferName();

	bool hasStencil();

	virtual GLenum completeness();
	GLenum completeness(int &width, int &height, int &samples);

protected:
	GLenum mColorbufferType;
	BindingPointer<Renderbuffer> mColorbufferPointer;

	GLenum mDepthbufferType;
	BindingPointer<Renderbuffer> mDepthbufferPointer;

	GLenum mStencilbufferType;
	BindingPointer<Renderbuffer> mStencilbufferPointer;

private:
	Renderbuffer *lookupRenderbuffer(GLenum type, GLuint handle) const;
};

class DefaultFramebuffer : public Framebuffer
{
public:
	DefaultFramebuffer(Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil);

	virtual GLenum completeness();
};

}

#endif   // LIBGL_FRAMEBUFFER_H_
