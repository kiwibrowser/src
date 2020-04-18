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

#ifndef LIBGLES_CM_FRAMEBUFFER_H_
#define LIBGLES_CM_FRAMEBUFFER_H_

#include "common/Object.hpp"
#include "common/Image.hpp"

#include <GLES/gl.h>

namespace es1
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

	egl::Image *getRenderTarget();
	egl::Image *getDepthBuffer();
	egl::Image *getStencilBuffer();

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

	GLenum getImplementationColorReadFormat();
	GLenum getImplementationColorReadType();

protected:
	GLenum mColorbufferType;
	gl::BindingPointer<Renderbuffer> mColorbufferPointer;

	GLenum mDepthbufferType;
	gl::BindingPointer<Renderbuffer> mDepthbufferPointer;

	GLenum mStencilbufferType;
	gl::BindingPointer<Renderbuffer> mStencilbufferPointer;

private:
	Renderbuffer *lookupRenderbuffer(GLenum type, GLuint handle) const;
};

class DefaultFramebuffer : public Framebuffer
{
public:
	DefaultFramebuffer(Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil);
};

}

#endif   // LIBGLES_CM_FRAMEBUFFER_H_
