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
// objects and related functionality.

#include "Framebuffer.h"

#include "main.h"
#include "Renderbuffer.h"
#include "Texture.h"
#include "utilities.h"

namespace gl
{

Framebuffer::Framebuffer()
{
	mColorbufferType = GL_NONE;
	mDepthbufferType = GL_NONE;
	mStencilbufferType = GL_NONE;
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

	if(type == GL_NONE)
	{
		buffer = nullptr;
	}
	else if(type == GL_RENDERBUFFER)
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
	mColorbufferType = (colorbuffer != 0) ? type : GL_NONE;
	mColorbufferPointer = lookupRenderbuffer(type, colorbuffer);
}

void Framebuffer::setDepthbuffer(GLenum type, GLuint depthbuffer)
{
	mDepthbufferType = (depthbuffer != 0) ? type : GL_NONE;
	mDepthbufferPointer = lookupRenderbuffer(type, depthbuffer);
}

void Framebuffer::setStencilbuffer(GLenum type, GLuint stencilbuffer)
{
	mStencilbufferType = (stencilbuffer != 0) ? type : GL_NONE;
	mStencilbufferPointer = lookupRenderbuffer(type, stencilbuffer);
}

void Framebuffer::detachTexture(GLuint texture)
{
	if(mColorbufferPointer.name() == texture && IsTextureTarget(mColorbufferType))
	{
		mColorbufferType = GL_NONE;
		mColorbufferPointer = nullptr;
	}

	if(mDepthbufferPointer.name() == texture && IsTextureTarget(mDepthbufferType))
	{
		mDepthbufferType = GL_NONE;
		mDepthbufferPointer = nullptr;
	}

	if(mStencilbufferPointer.name() == texture && IsTextureTarget(mStencilbufferType))
	{
		mStencilbufferType = GL_NONE;
		mStencilbufferPointer = nullptr;
	}
}

void Framebuffer::detachRenderbuffer(GLuint renderbuffer)
{
	if(mColorbufferPointer.name() == renderbuffer && mColorbufferType == GL_RENDERBUFFER)
	{
		mColorbufferType = GL_NONE;
		mColorbufferPointer = nullptr;
	}

	if(mDepthbufferPointer.name() == renderbuffer && mDepthbufferType == GL_RENDERBUFFER)
	{
		mDepthbufferType = GL_NONE;
		mDepthbufferPointer = nullptr;
	}

	if(mStencilbufferPointer.name() == renderbuffer && mStencilbufferType == GL_RENDERBUFFER)
	{
		mStencilbufferType = GL_NONE;
		mStencilbufferPointer = nullptr;
	}
}

// Increments refcount on surface.
// caller must Release() the returned surface
Image *Framebuffer::getRenderTarget()
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
Image *Framebuffer::getDepthStencil()
{
	Renderbuffer *depthstencilbuffer = mDepthbufferPointer;

	if(!depthstencilbuffer)
	{
		depthstencilbuffer = mStencilbufferPointer;
	}

	if(depthstencilbuffer)
	{
		return depthstencilbuffer->getRenderTarget();
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
	if(mStencilbufferType != GL_NONE)
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

	if(mColorbufferType != GL_NONE)
	{
		Renderbuffer *colorbuffer = getColorbuffer();

		if(!colorbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(colorbuffer->getWidth() == 0 || colorbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(mColorbufferType == GL_RENDERBUFFER)
		{
			if(!gl::IsColorRenderable(colorbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else if(IsTextureTarget(mColorbufferType))
		{
			GLenum format = colorbuffer->getFormat();

			if(IsCompressed(format) ||
			   format == GL_ALPHA ||
			   format == GL_LUMINANCE ||
			   format == GL_LUMINANCE_ALPHA)
			{
				return GL_FRAMEBUFFER_UNSUPPORTED;
			}

			if(gl::IsDepthTexture(format) || gl::IsStencilTexture(format))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else
		{
			UNREACHABLE(mColorbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		width = colorbuffer->getWidth();
		height = colorbuffer->getHeight();
		samples = colorbuffer->getSamples();
	}

	Renderbuffer *depthbuffer = nullptr;
	Renderbuffer *stencilbuffer = nullptr;

	if(mDepthbufferType != GL_NONE)
	{
		depthbuffer = getDepthbuffer();

		if(!depthbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(depthbuffer->getWidth() == 0 || depthbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(mDepthbufferType == GL_RENDERBUFFER)
		{
			if(!gl::IsDepthRenderable(depthbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else if(IsTextureTarget(mDepthbufferType))
		{
			if(!gl::IsDepthTexture(depthbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else
		{
			UNREACHABLE(mDepthbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(width == -1 || height == -1)
		{
			width = depthbuffer->getWidth();
			height = depthbuffer->getHeight();
			samples = depthbuffer->getSamples();
		}
		else if(width != depthbuffer->getWidth() || height != depthbuffer->getHeight())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT;
		}
		else if(samples != depthbuffer->getSamples())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT;
		}
	}

	if(mStencilbufferType != GL_NONE)
	{
		stencilbuffer = getStencilbuffer();

		if(!stencilbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(stencilbuffer->getWidth() == 0 || stencilbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(mStencilbufferType == GL_RENDERBUFFER)
		{
			if(!gl::IsStencilRenderable(stencilbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else if(IsTextureTarget(mStencilbufferType))
		{
			GLenum internalformat = stencilbuffer->getFormat();

			if(!gl::IsStencilTexture(internalformat))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else
		{
			UNREACHABLE(mStencilbufferType);
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(width == -1 || height == -1)
		{
			width = stencilbuffer->getWidth();
			height = stencilbuffer->getHeight();
			samples = stencilbuffer->getSamples();
		}
		else if(width != stencilbuffer->getWidth() || height != stencilbuffer->getHeight())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT;
		}
		else if(samples != stencilbuffer->getSamples())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT;
		}
	}

	// If we have both a depth and stencil buffer, they must refer to the same object
	// since we only support packed_depth_stencil and not separate depth and stencil
	if(depthbuffer && stencilbuffer && (depthbuffer != stencilbuffer))
	{
		return GL_FRAMEBUFFER_UNSUPPORTED;
	}

	// We need to have at least one attachment to be complete
	if(width == -1 || height == -1)
	{
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
	}

	return GL_FRAMEBUFFER_COMPLETE;
}

DefaultFramebuffer::DefaultFramebuffer(Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil)
{
	mColorbufferPointer = new Renderbuffer(0, colorbuffer);

	Renderbuffer *depthStencilRenderbuffer = new Renderbuffer(0, depthStencil);
	mDepthbufferPointer = depthStencilRenderbuffer;
	mStencilbufferPointer = depthStencilRenderbuffer;

	mColorbufferType = GL_RENDERBUFFER;
	mDepthbufferType = (depthStencilRenderbuffer->getDepthSize() != 0) ? GL_RENDERBUFFER : GL_NONE;
	mStencilbufferType = (depthStencilRenderbuffer->getStencilSize() != 0) ? GL_RENDERBUFFER : GL_NONE;
}

GLenum DefaultFramebuffer::completeness()
{
	// The default framebuffer should always be complete
	ASSERT(Framebuffer::completeness() == GL_FRAMEBUFFER_COMPLETE);

	return GL_FRAMEBUFFER_COMPLETE;
}

}
