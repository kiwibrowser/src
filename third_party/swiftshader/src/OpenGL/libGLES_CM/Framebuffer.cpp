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

// Framebuffer.cpp: Implements the Framebuffer class. Implements GL framebuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4 page 105.

#include "Framebuffer.h"

#include "main.h"
#include "Renderbuffer.h"
#include "Texture.h"
#include "utilities.h"

namespace es1
{

Framebuffer::Framebuffer()
{
	mColorbufferType = GL_NONE_OES;
	mDepthbufferType = GL_NONE_OES;
	mStencilbufferType = GL_NONE_OES;
}

Framebuffer::~Framebuffer()
{
	mColorbufferPointer = nullptr;
	mDepthbufferPointer = nullptr;
	mStencilbufferPointer = nullptr;
}

Renderbuffer *Framebuffer::lookupRenderbuffer(GLenum type, GLuint handle) const
{
	Context *context = getContext();
	Renderbuffer *buffer = nullptr;

	if(type == GL_NONE_OES)
	{
		buffer = nullptr;
	}
	else if(type == GL_RENDERBUFFER_OES)
	{
		buffer = context->getRenderbuffer(handle);
	}
	else if(IsTextureTarget(type))
	{
		buffer = context->getTexture(handle)->getRenderbuffer(type);
	}
	else UNREACHABLE(type);

	return buffer;
}

void Framebuffer::setColorbuffer(GLenum type, GLuint colorbuffer)
{
	mColorbufferType = (colorbuffer != 0) ? type : GL_NONE_OES;
	mColorbufferPointer = lookupRenderbuffer(type, colorbuffer);
}

void Framebuffer::setDepthbuffer(GLenum type, GLuint depthbuffer)
{
	mDepthbufferType = (depthbuffer != 0) ? type : GL_NONE_OES;
	mDepthbufferPointer = lookupRenderbuffer(type, depthbuffer);
}

void Framebuffer::setStencilbuffer(GLenum type, GLuint stencilbuffer)
{
	mStencilbufferType = (stencilbuffer != 0) ? type : GL_NONE_OES;
	mStencilbufferPointer = lookupRenderbuffer(type, stencilbuffer);
}

void Framebuffer::detachTexture(GLuint texture)
{
	if(mColorbufferPointer.name() == texture && IsTextureTarget(mColorbufferType))
	{
		mColorbufferType = GL_NONE_OES;
		mColorbufferPointer = nullptr;
	}

	if(mDepthbufferPointer.name() == texture && IsTextureTarget(mDepthbufferType))
	{
		mDepthbufferType = GL_NONE_OES;
		mDepthbufferPointer = nullptr;
	}

	if(mStencilbufferPointer.name() == texture && IsTextureTarget(mStencilbufferType))
	{
		mStencilbufferType = GL_NONE_OES;
		mStencilbufferPointer = nullptr;
	}
}

void Framebuffer::detachRenderbuffer(GLuint renderbuffer)
{
	if(mColorbufferPointer.name() == renderbuffer && mColorbufferType == GL_RENDERBUFFER_OES)
	{
		mColorbufferType = GL_NONE_OES;
		mColorbufferPointer = nullptr;
	}

	if(mDepthbufferPointer.name() == renderbuffer && mDepthbufferType == GL_RENDERBUFFER_OES)
	{
		mDepthbufferType = GL_NONE_OES;
		mDepthbufferPointer = nullptr;
	}

	if(mStencilbufferPointer.name() == renderbuffer && mStencilbufferType == GL_RENDERBUFFER_OES)
	{
		mStencilbufferType = GL_NONE_OES;
		mStencilbufferPointer = nullptr;
	}
}

// Increments refcount on surface.
// caller must Release() the returned surface
egl::Image *Framebuffer::getRenderTarget()
{
	Renderbuffer *colorbuffer = mColorbufferPointer;

	if(colorbuffer)
	{
		return colorbuffer->getRenderTarget();
	}

	return nullptr;
}

// Increments refcount on surface.
// caller must Release() the returned surface
egl::Image *Framebuffer::getDepthBuffer()
{
	Renderbuffer *depthbuffer = mDepthbufferPointer;

	if(depthbuffer)
	{
		return depthbuffer->getRenderTarget();
	}

	return nullptr;
}

// Increments refcount on surface.
// caller must Release() the returned surface
egl::Image *Framebuffer::getStencilBuffer()
{
	Renderbuffer *stencilbuffer = mStencilbufferPointer;

	if(stencilbuffer)
	{
		return stencilbuffer->getRenderTarget();
	}

	return nullptr;
}

Renderbuffer *Framebuffer::getColorbuffer()
{
	return mColorbufferPointer;
}

Renderbuffer *Framebuffer::getDepthbuffer()
{
	return mDepthbufferPointer;
}

Renderbuffer *Framebuffer::getStencilbuffer()
{
	return mStencilbufferPointer;
}

GLenum Framebuffer::getColorbufferType()
{
	return mColorbufferType;
}

GLenum Framebuffer::getDepthbufferType()
{
	return mDepthbufferType;
}

GLenum Framebuffer::getStencilbufferType()
{
	return mStencilbufferType;
}

GLuint Framebuffer::getColorbufferName()
{
	return mColorbufferPointer.name();
}

GLuint Framebuffer::getDepthbufferName()
{
	return mDepthbufferPointer.name();
}

GLuint Framebuffer::getStencilbufferName()
{
	return mStencilbufferPointer.name();
}

bool Framebuffer::hasStencil()
{
	if(mStencilbufferType != GL_NONE_OES)
	{
		Renderbuffer *stencilbufferObject = getStencilbuffer();

		if(stencilbufferObject)
		{
			return stencilbufferObject->getStencilSize() > 0;
		}
	}

	return false;
}

GLenum Framebuffer::completeness()
{
	int width;
	int height;
	int samples;

	return completeness(width, height, samples);
}

GLenum Framebuffer::completeness(int &width, int &height, int &samples)
{
	width = -1;
	height = -1;
	samples = -1;

	if(mColorbufferType != GL_NONE_OES)
	{
		Renderbuffer *colorbuffer = getColorbuffer();

		if(!colorbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(colorbuffer->getWidth() == 0 || colorbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(mColorbufferType == GL_RENDERBUFFER_OES)
		{
			if(!IsColorRenderable(colorbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else if(IsTextureTarget(mColorbufferType))
		{
			GLenum format = colorbuffer->getFormat();

			if(!IsColorRenderable(format))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}

			if(IsDepthTexture(format) || IsStencilTexture(format))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else
		{
			UNREACHABLE(mColorbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		width = colorbuffer->getWidth();
		height = colorbuffer->getHeight();
		samples = colorbuffer->getSamples();
	}

	Renderbuffer *depthbuffer = nullptr;
	Renderbuffer *stencilbuffer = nullptr;

	if(mDepthbufferType != GL_NONE_OES)
	{
		depthbuffer = getDepthbuffer();

		if(!depthbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(depthbuffer->getWidth() == 0 || depthbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(mDepthbufferType == GL_RENDERBUFFER_OES)
		{
			if(!es1::IsDepthRenderable(depthbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else if(IsTextureTarget(mDepthbufferType))
		{
			if(!es1::IsDepthTexture(depthbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else
		{
			UNREACHABLE(mDepthbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(width == -1 || height == -1)
		{
			width = depthbuffer->getWidth();
			height = depthbuffer->getHeight();
			samples = depthbuffer->getSamples();
		}
		else if(width != depthbuffer->getWidth() || height != depthbuffer->getHeight())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES;
		}
		else if(samples != depthbuffer->getSamples())
		{
			UNREACHABLE(0);
		}
	}

	if(mStencilbufferType != GL_NONE_OES)
	{
		stencilbuffer = getStencilbuffer();

		if(!stencilbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(stencilbuffer->getWidth() == 0 || stencilbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(mStencilbufferType == GL_RENDERBUFFER_OES)
		{
			if(!es1::IsStencilRenderable(stencilbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else if(IsTextureTarget(mStencilbufferType))
		{
			GLenum internalformat = stencilbuffer->getFormat();

			if(!es1::IsStencilTexture(internalformat))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			}
		}
		else
		{
			UNREACHABLE(mStencilbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
		}

		if(width == -1 || height == -1)
		{
			width = stencilbuffer->getWidth();
			height = stencilbuffer->getHeight();
			samples = stencilbuffer->getSamples();
		}
		else if(width != stencilbuffer->getWidth() || height != stencilbuffer->getHeight())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES;
		}
		else if(samples != stencilbuffer->getSamples())
		{
			UNREACHABLE(0);
			return GL_FRAMEBUFFER_UNSUPPORTED_OES;   // GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_OES;
		}
	}

	// We need to have at least one attachment to be complete
	if(width == -1 || height == -1)
	{
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES;
	}

	return GL_FRAMEBUFFER_COMPLETE_OES;
}

GLenum Framebuffer::getImplementationColorReadFormat()
{
	Renderbuffer *colorbuffer = mColorbufferPointer;

	if(colorbuffer)
	{
		switch(colorbuffer->getFormat())
		{
		case GL_BGRA8_EXT:   return GL_BGRA_EXT;
		case GL_RGBA4_OES:   return GL_RGBA;
		case GL_RGB5_A1_OES: return GL_RGBA;
		case GL_RGBA8_OES:   return GL_RGBA;
		case GL_RGB565_OES:  return GL_RGBA;
		case GL_RGB8_OES:    return GL_RGB;
		default:
			UNREACHABLE(colorbuffer->getFormat());
		}
	}

	return GL_RGBA;
}

GLenum Framebuffer::getImplementationColorReadType()
{
	Renderbuffer *colorbuffer = mColorbufferPointer;

	if(colorbuffer)
	{
		switch(colorbuffer->getFormat())
		{
		case GL_BGRA8_EXT:   return GL_UNSIGNED_BYTE;
		case GL_RGBA4_OES:   return GL_UNSIGNED_SHORT_4_4_4_4;
		case GL_RGB5_A1_OES: return GL_UNSIGNED_SHORT_5_5_5_1;
		case GL_RGBA8_OES:   return GL_UNSIGNED_BYTE;
		case GL_RGB565_OES:  return GL_UNSIGNED_SHORT_5_6_5;
		case GL_RGB8_OES:    return GL_UNSIGNED_BYTE;
		default:
			UNREACHABLE(colorbuffer->getFormat());
		}
	}

	return GL_UNSIGNED_BYTE;
}

DefaultFramebuffer::DefaultFramebuffer(Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil)
{
	mColorbufferPointer = new Renderbuffer(0, colorbuffer);

	Renderbuffer *depthStencilRenderbuffer = new Renderbuffer(0, depthStencil);
	mDepthbufferPointer = depthStencilRenderbuffer;
	mStencilbufferPointer = depthStencilRenderbuffer;

	mColorbufferType = GL_RENDERBUFFER_OES;
	mDepthbufferType = (depthStencilRenderbuffer->getDepthSize() != 0) ? GL_RENDERBUFFER_OES : GL_NONE_OES;
	mStencilbufferType = (depthStencilRenderbuffer->getStencilSize() != 0) ? GL_RENDERBUFFER_OES : GL_NONE_OES;
}

}
