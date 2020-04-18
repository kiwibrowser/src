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
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4 page 105.

#ifndef LIBGLESV2_FRAMEBUFFER_H_
#define LIBGLESV2_FRAMEBUFFER_H_

#include "Context.h"
#include "common/Object.hpp"
#include "common/Image.hpp"

#include <GLES2/gl2.h>

namespace es2
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

	void setColorbuffer(GLenum type, GLuint colorbuffer, GLuint index, GLint level = 0, GLint layer = 0);
	void setDepthbuffer(GLenum type, GLuint depthbuffer, GLint level = 0, GLint layer = 0);
	void setStencilbuffer(GLenum type, GLuint stencilbuffer, GLint level = 0, GLint layer = 0);

	void setReadBuffer(GLenum buf);
	void setDrawBuffer(GLuint index, GLenum buf);
	GLenum getReadBuffer() const;
	GLenum getDrawBuffer(GLuint index) const;

	void detachTexture(GLuint texture);
	void detachRenderbuffer(GLuint renderbuffer);

	egl::Image *getRenderTarget(GLuint index);
	egl::Image *getReadRenderTarget();
	egl::Image *getDepthBuffer();
	egl::Image *getStencilBuffer();

	Renderbuffer *getColorbuffer(GLuint index) const;
	Renderbuffer *getReadColorbuffer() const;
	Renderbuffer *getDepthbuffer() const;
	Renderbuffer *getStencilbuffer() const;

	GLenum getReadBufferType();
	GLenum getColorbufferType(GLuint index);
	GLenum getDepthbufferType();
	GLenum getStencilbufferType();

	GLuint getColorbufferName(GLuint index);
	GLuint getDepthbufferName();
	GLuint getStencilbufferName();

	GLint getColorbufferLayer(GLuint index);
	GLint getDepthbufferLayer();
	GLint getStencilbufferLayer();

	bool hasStencil();

	GLenum completeness();
	GLenum completeness(int &width, int &height, int &samples);

	GLenum getImplementationColorReadFormat() const;
	GLenum getImplementationColorReadType() const;
	GLenum getDepthReadFormat() const;
	GLenum getDepthReadType() const;

	virtual bool isDefaultFramebuffer() const { return false; }

	static bool IsRenderbuffer(GLenum type);

protected:
	GLuint getReadBufferIndex() const;

	GLenum readBuffer;
	GLenum drawBuffer[MAX_COLOR_ATTACHMENTS];

	GLenum mColorbufferType[MAX_COLOR_ATTACHMENTS];
	gl::BindingPointer<Renderbuffer> mColorbufferPointer[MAX_COLOR_ATTACHMENTS];
	GLint mColorbufferLayer[MAX_COLOR_ATTACHMENTS];

	GLenum mDepthbufferType;
	gl::BindingPointer<Renderbuffer> mDepthbufferPointer;
	GLint mDepthbufferLayer;

	GLenum mStencilbufferType;
	gl::BindingPointer<Renderbuffer> mStencilbufferPointer;
	GLint mStencilbufferLayer;

private:
	Renderbuffer *lookupRenderbuffer(GLenum type, GLuint handle, GLint level) const;
};

class DefaultFramebuffer : public Framebuffer
{
public:
	DefaultFramebuffer();
	DefaultFramebuffer(Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil);

	bool isDefaultFramebuffer() const override { return true; }
};

}

#endif   // LIBGLESV2_FRAMEBUFFER_H_
