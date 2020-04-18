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

// Renderbuffer.h: Defines the wrapper class Renderbuffer, as well as the
// class hierarchy used to store its contents: RenderbufferStorage, Colorbuffer,
// DepthStencilbuffer, Depthbuffer and Stencilbuffer. Implements GL renderbuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#ifndef LIBGLES_CM_RENDERBUFFER_H_
#define LIBGLES_CM_RENDERBUFFER_H_

#include "common/Object.hpp"
#include "common/Image.hpp"

#include <GLES/gl.h>

namespace es1
{
class Texture2D;
class Renderbuffer;
class Colorbuffer;
class DepthStencilbuffer;

class RenderbufferInterface
{
public:
	RenderbufferInterface();

	virtual ~RenderbufferInterface() {};

	virtual void addProxyRef(const Renderbuffer *proxy);
    virtual void releaseProxy(const Renderbuffer *proxy);

	virtual egl::Image *getRenderTarget() = 0;
    virtual egl::Image *createSharedImage() = 0;
    virtual bool isShared() const = 0;

	virtual GLsizei getWidth() const = 0;
	virtual GLsizei getHeight() const = 0;
	virtual GLint getFormat() const = 0;
	virtual GLsizei getSamples() const = 0;

	GLuint getRedSize() const;
	GLuint getGreenSize() const;
	GLuint getBlueSize() const;
	GLuint getAlphaSize() const;
	GLuint getDepthSize() const;
	GLuint getStencilSize() const;
};

class RenderbufferTexture2D : public RenderbufferInterface
{
public:
	RenderbufferTexture2D(Texture2D *texture);

	virtual ~RenderbufferTexture2D();

	virtual void addProxyRef(const Renderbuffer *proxy);
    virtual void releaseProxy(const Renderbuffer *proxy);

	virtual egl::Image *getRenderTarget();
    virtual egl::Image *createSharedImage();
    virtual bool isShared() const;

	virtual GLsizei getWidth() const;
	virtual GLsizei getHeight() const;
	virtual GLint getFormat() const;
	virtual GLsizei getSamples() const;

private:
	gl::BindingPointer<Texture2D> mTexture2D;
};

// A class derived from RenderbufferStorage is created whenever glRenderbufferStorage
// is called. The specific concrete type depends on whether the internal format is
// colour depth, stencil or packed depth/stencil.
class RenderbufferStorage : public RenderbufferInterface
{
public:
	RenderbufferStorage();

	virtual ~RenderbufferStorage() = 0;

	virtual egl::Image *getRenderTarget() = 0;
    virtual egl::Image *createSharedImage() = 0;
    virtual bool isShared() const = 0;

	virtual GLsizei getWidth() const;
	virtual GLsizei getHeight() const;
	virtual GLint getFormat() const;
	virtual GLsizei getSamples() const;

protected:
	GLsizei mWidth;
	GLsizei mHeight;
	GLenum format;
	GLsizei mSamples;
};

// Renderbuffer implements the GL renderbuffer object.
// It's only a proxy for a RenderbufferInterface instance; the internal object
// can change whenever glRenderbufferStorage is called.
class Renderbuffer : public gl::NamedObject
{
public:
	Renderbuffer(GLuint name, RenderbufferInterface *storage);

	virtual ~Renderbuffer();

	// These functions from Object are overloaded here because
    // Textures need to maintain their own count of references to them via
    // Renderbuffers/RenderbufferTextures. These functions invoke those
    // reference counting functions on the RenderbufferInterface.
    virtual void addRef();
    virtual void release();

	egl::Image *getRenderTarget();
    virtual egl::Image *createSharedImage();
    virtual bool isShared() const;

	GLsizei getWidth() const;
	GLsizei getHeight() const;
	GLenum getFormat() const;
	GLuint getRedSize() const;
	GLuint getGreenSize() const;
	GLuint getBlueSize() const;
	GLuint getAlphaSize() const;
	GLuint getDepthSize() const;
	GLuint getStencilSize() const;
	GLsizei getSamples() const;

	void setStorage(RenderbufferStorage *newStorage);

private:
	RenderbufferInterface *mInstance;
};

class Colorbuffer : public RenderbufferStorage
{
public:
	explicit Colorbuffer(egl::Image *renderTarget);
	Colorbuffer(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples);

	virtual ~Colorbuffer();

	virtual egl::Image *getRenderTarget();
    virtual egl::Image *createSharedImage();
    virtual bool isShared() const;

private:
	egl::Image *mRenderTarget;
};

class DepthStencilbuffer : public RenderbufferStorage
{
public:
	explicit DepthStencilbuffer(egl::Image *depthStencil);
	DepthStencilbuffer(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples);

	~DepthStencilbuffer();

	virtual egl::Image *getRenderTarget();
    virtual egl::Image *createSharedImage();
    virtual bool isShared() const;

protected:
	egl::Image *mDepthStencil;
};

class Depthbuffer : public DepthStencilbuffer
{
public:
	explicit Depthbuffer(egl::Image *depthStencil);
	Depthbuffer(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples);

	virtual ~Depthbuffer();
};

class Stencilbuffer : public DepthStencilbuffer
{
public:
	explicit Stencilbuffer(egl::Image *depthStencil);
	Stencilbuffer(GLsizei width, GLsizei height, GLsizei samples);

	virtual ~Stencilbuffer();
};
}

#endif   // LIBGLES_CM_RENDERBUFFER_H_
