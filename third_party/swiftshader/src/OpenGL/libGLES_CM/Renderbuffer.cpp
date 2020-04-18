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

// Renderbuffer.cpp: the Renderbuffer class and its derived classes
// Colorbuffer, Depthbuffer and Stencilbuffer. Implements GL renderbuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#include "Renderbuffer.h"

#include "main.h"
#include "Texture.h"
#include "utilities.h"

namespace es1
{
RenderbufferInterface::RenderbufferInterface()
{
}

// The default case for classes inherited from RenderbufferInterface is not to
// need to do anything upon the reference count to the parent Renderbuffer incrementing
// or decrementing.
void RenderbufferInterface::addProxyRef(const Renderbuffer *proxy)
{
}

void RenderbufferInterface::releaseProxy(const Renderbuffer *proxy)
{
}

GLuint RenderbufferInterface::getRedSize() const
{
	return GetRedSize(getFormat());
}

GLuint RenderbufferInterface::getGreenSize() const
{
	return GetGreenSize(getFormat());
}

GLuint RenderbufferInterface::getBlueSize() const
{
	return GetBlueSize(getFormat());
}

GLuint RenderbufferInterface::getAlphaSize() const
{
	return GetAlphaSize(getFormat());
}

GLuint RenderbufferInterface::getDepthSize() const
{
	return GetDepthSize(getFormat());
}

GLuint RenderbufferInterface::getStencilSize() const
{
	return GetStencilSize(getFormat());
}

///// RenderbufferTexture2D Implementation ////////

RenderbufferTexture2D::RenderbufferTexture2D(Texture2D *texture)
{
	mTexture2D = texture;
}

RenderbufferTexture2D::~RenderbufferTexture2D()
{
	mTexture2D = nullptr;
}

// Textures need to maintain their own reference count for references via
// Renderbuffers acting as proxies. Here, we notify the texture of a reference.
void RenderbufferTexture2D::addProxyRef(const Renderbuffer *proxy)
{
	mTexture2D->addProxyRef(proxy);
}

void RenderbufferTexture2D::releaseProxy(const Renderbuffer *proxy)
{
	mTexture2D->releaseProxy(proxy);
}

// Increments refcount on image.
// caller must release() the returned image
egl::Image *RenderbufferTexture2D::getRenderTarget()
{
	return mTexture2D->getRenderTarget(GL_TEXTURE_2D, 0);
}

// Increments refcount on image.
// caller must release() the returned image
egl::Image *RenderbufferTexture2D::createSharedImage()
{
	return mTexture2D->createSharedImage(GL_TEXTURE_2D, 0);
}

bool RenderbufferTexture2D::isShared() const
{
	return mTexture2D->isShared(GL_TEXTURE_2D, 0);
}

GLsizei RenderbufferTexture2D::getWidth() const
{
	return mTexture2D->getWidth(GL_TEXTURE_2D, 0);
}

GLsizei RenderbufferTexture2D::getHeight() const
{
	return mTexture2D->getHeight(GL_TEXTURE_2D, 0);
}

GLint RenderbufferTexture2D::getFormat() const
{
	return mTexture2D->getFormat(GL_TEXTURE_2D, 0);
}

GLsizei RenderbufferTexture2D::getSamples() const
{
	return 0;
}

////// Renderbuffer Implementation //////

Renderbuffer::Renderbuffer(GLuint name, RenderbufferInterface *instance) : NamedObject(name)
{
	ASSERT(instance);
	mInstance = instance;
}

Renderbuffer::~Renderbuffer()
{
	delete mInstance;
}

// The RenderbufferInterface contained in this Renderbuffer may need to maintain
// its own reference count, so we pass it on here.
void Renderbuffer::addRef()
{
	mInstance->addProxyRef(this);

	Object::addRef();
}

void Renderbuffer::release()
{
	mInstance->releaseProxy(this);

	Object::release();
}

// Increments refcount on image.
// caller must Release() the returned image
egl::Image *Renderbuffer::getRenderTarget()
{
	return mInstance->getRenderTarget();
}

// Increments refcount on image.
// caller must Release() the returned image
egl::Image *Renderbuffer::createSharedImage()
{
	return mInstance->createSharedImage();
}

bool Renderbuffer::isShared() const
{
	return mInstance->isShared();
}

GLsizei Renderbuffer::getWidth() const
{
	return mInstance->getWidth();
}

GLsizei Renderbuffer::getHeight() const
{
	return mInstance->getHeight();
}

GLenum Renderbuffer::getFormat() const
{
	return mInstance->getFormat();
}

GLuint Renderbuffer::getRedSize() const
{
	return mInstance->getRedSize();
}

GLuint Renderbuffer::getGreenSize() const
{
	return mInstance->getGreenSize();
}

GLuint Renderbuffer::getBlueSize() const
{
	return mInstance->getBlueSize();
}

GLuint Renderbuffer::getAlphaSize() const
{
	return mInstance->getAlphaSize();
}

GLuint Renderbuffer::getDepthSize() const
{
	return mInstance->getDepthSize();
}

GLuint Renderbuffer::getStencilSize() const
{
	return mInstance->getStencilSize();
}

GLsizei Renderbuffer::getSamples() const
{
	return mInstance->getSamples();
}

void Renderbuffer::setStorage(RenderbufferStorage *newStorage)
{
	ASSERT(newStorage);

	delete mInstance;
	mInstance = newStorage;
}

RenderbufferStorage::RenderbufferStorage()
{
	mWidth = 0;
	mHeight = 0;
	format = GL_NONE_OES;
	mSamples = 0;
}

RenderbufferStorage::~RenderbufferStorage()
{
}

GLsizei RenderbufferStorage::getWidth() const
{
	return mWidth;
}

GLsizei RenderbufferStorage::getHeight() const
{
	return mHeight;
}

GLint RenderbufferStorage::getFormat() const
{
	return format;
}

GLsizei RenderbufferStorage::getSamples() const
{
	return mSamples;
}

Colorbuffer::Colorbuffer(egl::Image *renderTarget) : mRenderTarget(renderTarget)
{
	if(renderTarget)
	{
		renderTarget->addRef();

		mWidth = renderTarget->getWidth();
		mHeight = renderTarget->getHeight();
		format = renderTarget->getFormat();
		mSamples = renderTarget->getDepth() & ~1;
	}
}

Colorbuffer::Colorbuffer(int width, int height, GLenum internalformat, GLsizei samples) : mRenderTarget(nullptr)
{
	int supportedSamples = Context::getSupportedMultisampleCount(samples);

	if(width > 0 && height > 0)
	{
		if(height > sw::OUTLINE_RESOLUTION)
		{
			error(GL_OUT_OF_MEMORY);
			return;
		}

		mRenderTarget = egl::Image::create(width, height, internalformat, supportedSamples, false);

		if(!mRenderTarget)
		{
			error(GL_OUT_OF_MEMORY);
			return;
		}
	}

	mWidth = width;
	mHeight = height;
	format = internalformat;
	mSamples = supportedSamples;
}

Colorbuffer::~Colorbuffer()
{
	if(mRenderTarget)
	{
		mRenderTarget->release();
	}
}

// Increments refcount on image.
// caller must release() the returned image
egl::Image *Colorbuffer::getRenderTarget()
{
	if(mRenderTarget)
	{
		mRenderTarget->addRef();
	}

	return mRenderTarget;
}

// Increments refcount on image.
// caller must release() the returned image
egl::Image *Colorbuffer::createSharedImage()
{
	if(mRenderTarget)
	{
		mRenderTarget->addRef();
		mRenderTarget->markShared();
	}

	return mRenderTarget;
}

bool Colorbuffer::isShared() const
{
	return mRenderTarget->isShared();
}

DepthStencilbuffer::DepthStencilbuffer(egl::Image *depthStencil) : mDepthStencil(depthStencil)
{
	if(depthStencil)
	{
		depthStencil->addRef();

		mWidth = depthStencil->getWidth();
		mHeight = depthStencil->getHeight();
		format = depthStencil->getFormat();
		mSamples = depthStencil->getDepth() & ~1;
	}
}

DepthStencilbuffer::DepthStencilbuffer(int width, int height, GLenum internalformat, GLsizei samples) : mDepthStencil(nullptr)
{
	int supportedSamples = Context::getSupportedMultisampleCount(samples);

	if(width > 0 && height > 0)
	{
		if(height > sw::OUTLINE_RESOLUTION)
		{
			error(GL_OUT_OF_MEMORY);
			return;
		}

		mDepthStencil = egl::Image::create(width, height, internalformat, supportedSamples, false);

		if(!mDepthStencil)
		{
			error(GL_OUT_OF_MEMORY);
			return;
		}
	}

	mWidth = width;
	mHeight = height;
	format = internalformat;
	mSamples = supportedSamples;
}

DepthStencilbuffer::~DepthStencilbuffer()
{
	if(mDepthStencil)
	{
		mDepthStencil->release();
	}
}

// Increments refcount on image.
// caller must release() the returned image
egl::Image *DepthStencilbuffer::getRenderTarget()
{
	if(mDepthStencil)
	{
		mDepthStencil->addRef();
	}

	return mDepthStencil;
}

// Increments refcount on image.
// caller must release() the returned image
egl::Image *DepthStencilbuffer::createSharedImage()
{
	if(mDepthStencil)
	{
		mDepthStencil->addRef();
		mDepthStencil->markShared();
	}

	return mDepthStencil;
}

bool DepthStencilbuffer::isShared() const
{
	return mDepthStencil->isShared();
}

Depthbuffer::Depthbuffer(egl::Image *depthStencil) : DepthStencilbuffer(depthStencil)
{
}

Depthbuffer::Depthbuffer(int width, int height, GLenum internalformat, GLsizei samples) : DepthStencilbuffer(width, height, internalformat, samples)
{
}

Depthbuffer::~Depthbuffer()
{
}

Stencilbuffer::Stencilbuffer(egl::Image *depthStencil) : DepthStencilbuffer(depthStencil)
{
}

Stencilbuffer::Stencilbuffer(int width, int height, GLsizei samples) : DepthStencilbuffer(width, height, GL_STENCIL_INDEX8_OES, samples)
{
}

Stencilbuffer::~Stencilbuffer()
{
}

}
