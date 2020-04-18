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
// objects and related functionality.

#ifndef LIBGL_RENDERBUFFER_H_
#define LIBGL_RENDERBUFFER_H_

#include "common/Object.hpp"
#include "Image.hpp"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

namespace gl
{
class Texture2D;
class TextureCubeMap;
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

	virtual Image *getRenderTarget() = 0;

	virtual GLsizei getWidth() const = 0;
	virtual GLsizei getHeight() const = 0;
	virtual GLenum getFormat() const = 0;
	virtual sw::Format getInternalFormat() const = 0;
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

	Image *getRenderTarget();

	virtual GLsizei getWidth() const;
	virtual GLsizei getHeight() const;
	virtual GLenum getFormat() const;
	virtual sw::Format getInternalFormat() const;
	virtual GLsizei getSamples() const;

private:
	BindingPointer<Texture2D> mTexture2D;
};

class RenderbufferTextureCubeMap : public RenderbufferInterface
{
public:
	RenderbufferTextureCubeMap(TextureCubeMap *texture, GLenum target);

	virtual ~RenderbufferTextureCubeMap();

	virtual void addProxyRef(const Renderbuffer *proxy);
	virtual void releaseProxy(const Renderbuffer *proxy);

	Image *getRenderTarget();

	virtual GLsizei getWidth() const;
	virtual GLsizei getHeight() const;
	virtual GLenum getFormat() const;
	virtual sw::Format getInternalFormat() const;
	virtual GLsizei getSamples() const;

private:
	BindingPointer<TextureCubeMap> mTextureCubeMap;
	GLenum mTarget;
};

// A class derived from RenderbufferStorage is created whenever glRenderbufferStorage
// is called. The specific concrete type depends on whether the internal format is
// colour depth, stencil or packed depth/stencil.
class RenderbufferStorage : public RenderbufferInterface
{
public:
	RenderbufferStorage();

	virtual ~RenderbufferStorage() = 0;

	virtual Image *getRenderTarget();

	virtual GLsizei getWidth() const;
	virtual GLsizei getHeight() const;
	virtual GLenum getFormat() const;
	virtual sw::Format getInternalFormat() const;
	virtual GLsizei getSamples() const;

protected:
	GLsizei mWidth;
	GLsizei mHeight;
	GLenum format;
	sw::Format internalFormat;
	GLsizei mSamples;
};

// Renderbuffer implements the GL renderbuffer object.
// It's only a proxy for a RenderbufferInterface instance; the internal object
// can change whenever glRenderbufferStorage is called.
class Renderbuffer : public NamedObject
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

	Image *getRenderTarget();

	GLsizei getWidth() const;
	GLsizei getHeight() const;
	GLenum getFormat() const;
	sw::Format getInternalFormat() const;
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
	explicit Colorbuffer(Image *renderTarget);
	Colorbuffer(GLsizei width, GLsizei height, GLenum format, GLsizei samples);

	virtual ~Colorbuffer();

	virtual Image *getRenderTarget();

private:
	Image *mRenderTarget;
};

class DepthStencilbuffer : public RenderbufferStorage
{
public:
	explicit DepthStencilbuffer(Image *depthStencil);
	DepthStencilbuffer(GLsizei width, GLsizei height, GLsizei samples);

	~DepthStencilbuffer();

	virtual Image *getRenderTarget();

protected:
	Image *mDepthStencil;
};

class Depthbuffer : public DepthStencilbuffer
{
public:
	explicit Depthbuffer(Image *depthStencil);
	Depthbuffer(GLsizei width, GLsizei height, GLsizei samples);

	virtual ~Depthbuffer();
};

class Stencilbuffer : public DepthStencilbuffer
{
public:
	explicit Stencilbuffer(Image *depthStencil);
	Stencilbuffer(GLsizei width, GLsizei height, GLsizei samples);

	virtual ~Stencilbuffer();
};
}

#endif   // LIBGL_RENDERBUFFER_H_
