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

#include <algorithm>

namespace es2
{

bool Framebuffer::IsRenderbuffer(GLenum type)
{
	return type == GL_RENDERBUFFER || type == GL_FRAMEBUFFER_DEFAULT;
}

Framebuffer::Framebuffer()
{
	readBuffer = GL_COLOR_ATTACHMENT0;
	drawBuffer[0] = GL_COLOR_ATTACHMENT0;
	for(int i = 1; i < MAX_COLOR_ATTACHMENTS; i++)
	{
		drawBuffer[i] = GL_NONE;
	}

	for(int i = 0; i < MAX_COLOR_ATTACHMENTS; i++)
	{
		mColorbufferType[i] = GL_NONE;
		mColorbufferLayer[i] = 0;
	}

	mDepthbufferType = GL_NONE;
	mDepthbufferLayer = 0;
	mStencilbufferType = GL_NONE;
	mStencilbufferLayer = 0;
}

Framebuffer::~Framebuffer()
{
	for(int i = 0; i < MAX_COLOR_ATTACHMENTS; i++)
	{
		mColorbufferPointer[i] = nullptr;
	}
	mDepthbufferPointer = nullptr;
	mStencilbufferPointer = nullptr;
}

Renderbuffer *Framebuffer::lookupRenderbuffer(GLenum type, GLuint handle, GLint level) const
{
	Context *context = getContext();
	Renderbuffer *buffer = nullptr;

	if(type == GL_NONE)
	{
		buffer = nullptr;
	}
	else if(IsRenderbuffer(type))
	{
		buffer = context->getRenderbuffer(handle);
	}
	else if(IsTextureTarget(type))
	{
		buffer = context->getTexture(handle)->getRenderbuffer(type, level);
	}
	else UNREACHABLE(type);

	return buffer;
}

void Framebuffer::setColorbuffer(GLenum type, GLuint colorbuffer, GLuint index, GLint level, GLint layer)
{
	mColorbufferType[index] = (colorbuffer != 0) ? type : GL_NONE;
	mColorbufferPointer[index] = lookupRenderbuffer(type, colorbuffer, level);
	mColorbufferLayer[index] = layer;
}

void Framebuffer::setDepthbuffer(GLenum type, GLuint depthbuffer, GLint level, GLint layer)
{
	mDepthbufferType = (depthbuffer != 0) ? type : GL_NONE;
	mDepthbufferPointer = lookupRenderbuffer(type, depthbuffer, level);
	mDepthbufferLayer = layer;
}

void Framebuffer::setStencilbuffer(GLenum type, GLuint stencilbuffer, GLint level, GLint layer)
{
	mStencilbufferType = (stencilbuffer != 0) ? type : GL_NONE;
	mStencilbufferPointer = lookupRenderbuffer(type, stencilbuffer, level);
	mStencilbufferLayer = layer;
}

void Framebuffer::setReadBuffer(GLenum buf)
{
	readBuffer = buf;
}

void Framebuffer::setDrawBuffer(GLuint index, GLenum buf)
{
	drawBuffer[index] = buf;
}

GLenum Framebuffer::getReadBuffer() const
{
	return readBuffer;
}

GLenum Framebuffer::getDrawBuffer(GLuint index) const
{
	return drawBuffer[index];
}

void Framebuffer::detachTexture(GLuint texture)
{
	for(int i = 0; i < MAX_COLOR_ATTACHMENTS; i++)
	{
		if(mColorbufferPointer[i].name() == texture && IsTextureTarget(mColorbufferType[i]))
		{
			mColorbufferType[i] = GL_NONE;
			mColorbufferPointer[i] = nullptr;
		}
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
	for(int i = 0; i < MAX_COLOR_ATTACHMENTS; i++)
	{
		if(mColorbufferPointer[i].name() == renderbuffer && IsRenderbuffer(mColorbufferType[i]))
		{
			mColorbufferType[i] = GL_NONE;
			mColorbufferPointer[i] = nullptr;
		}
	}

	if(mDepthbufferPointer.name() == renderbuffer && IsRenderbuffer(mDepthbufferType))
	{
		mDepthbufferType = GL_NONE;
		mDepthbufferPointer = nullptr;
	}

	if(mStencilbufferPointer.name() == renderbuffer && IsRenderbuffer(mStencilbufferType))
	{
		mStencilbufferType = GL_NONE;
		mStencilbufferPointer = nullptr;
	}
}

// Increments refcount on surface.
// caller must Release() the returned surface
egl::Image *Framebuffer::getRenderTarget(GLuint index)
{
	if(index < MAX_COLOR_ATTACHMENTS)
	{
		Renderbuffer *colorbuffer = mColorbufferPointer[index];

		if(colorbuffer)
		{
			return colorbuffer->getRenderTarget();
		}
	}

	return nullptr;
}

egl::Image *Framebuffer::getReadRenderTarget()
{
	return getRenderTarget(getReadBufferIndex());
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

Renderbuffer *Framebuffer::getColorbuffer(GLuint index) const
{
	return (index < MAX_COLOR_ATTACHMENTS) ? mColorbufferPointer[index] : (Renderbuffer*)nullptr;
}

Renderbuffer *Framebuffer::getReadColorbuffer() const
{
	return getColorbuffer(getReadBufferIndex());
}

Renderbuffer *Framebuffer::getDepthbuffer() const
{
	return mDepthbufferPointer;
}

Renderbuffer *Framebuffer::getStencilbuffer() const
{
	return mStencilbufferPointer;
}

GLenum Framebuffer::getReadBufferType()
{
	if(readBuffer == GL_NONE)
	{
		return GL_NONE;
	}

	return mColorbufferType[getReadBufferIndex()];
}

GLenum Framebuffer::getColorbufferType(GLuint index)
{
	return mColorbufferType[index];
}

GLenum Framebuffer::getDepthbufferType()
{
	return mDepthbufferType;
}

GLenum Framebuffer::getStencilbufferType()
{
	return mStencilbufferType;
}

GLuint Framebuffer::getColorbufferName(GLuint index)
{
	return mColorbufferPointer[index].name();
}

GLuint Framebuffer::getDepthbufferName()
{
	return mDepthbufferPointer.name();
}

GLuint Framebuffer::getStencilbufferName()
{
	return mStencilbufferPointer.name();
}

GLint Framebuffer::getColorbufferLayer(GLuint index)
{
	return mColorbufferLayer[index];
}

GLint Framebuffer::getDepthbufferLayer()
{
	return mDepthbufferLayer;
}

GLint Framebuffer::getStencilbufferLayer()
{
	return mStencilbufferLayer;
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

	GLint version = egl::getClientVersion();

	for(int i = 0; i < MAX_COLOR_ATTACHMENTS; i++)
	{
		if(mColorbufferType[i] != GL_NONE)
		{
			Renderbuffer *colorbuffer = getColorbuffer(i);

			if(!colorbuffer)
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}

			if(colorbuffer->getWidth() == 0 || colorbuffer->getHeight() == 0 || (colorbuffer->getDepth() <= mColorbufferLayer[i]))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}

			if(IsRenderbuffer(mColorbufferType[i]))
			{
				if(!IsColorRenderable(colorbuffer->getFormat(), version))
				{
					return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
				}
			}
			else if(IsTextureTarget(mColorbufferType[i]))
			{
				GLenum format = colorbuffer->getFormat();

				if(!IsColorRenderable(format, version))
				{
					return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
				}

				if(IsDepthTexture(format) || IsStencilTexture(format))
				{
					return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
				}
			}
			else
			{
				UNREACHABLE(mColorbufferType[i]);
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}

			if(width == -1 || height == -1)
			{
				width = colorbuffer->getWidth();
				height = colorbuffer->getHeight();
				samples = colorbuffer->getSamples();
			}
			else
			{
				if(version < 3 && (width != colorbuffer->getWidth() || height != colorbuffer->getHeight()))
				{
					return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
				}

				if(samples != colorbuffer->getSamples())
				{
					return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE;
				}

				width = std::min(width, colorbuffer->getWidth());
				height = std::min(height, colorbuffer->getHeight());
			}
		}
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

		if(depthbuffer->getWidth() == 0 || depthbuffer->getHeight() == 0 || (depthbuffer->getDepth() <= mDepthbufferLayer))
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(IsRenderbuffer(mDepthbufferType))
		{
			if(!es2::IsDepthRenderable(depthbuffer->getFormat(), version))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else if(IsTextureTarget(mDepthbufferType))
		{
			if(!es2::IsDepthTexture(depthbuffer->getFormat()))
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
		else
		{
			if(version < 3 && (width != depthbuffer->getWidth() || height != depthbuffer->getHeight()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
			}

			if(samples != depthbuffer->getSamples())
			{
				return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE;
			}

			width = std::min(width, depthbuffer->getWidth());
			height = std::min(height, depthbuffer->getHeight());
		}
	}

	if(mStencilbufferType != GL_NONE)
	{
		stencilbuffer = getStencilbuffer();

		if(!stencilbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(stencilbuffer->getWidth() == 0 || stencilbuffer->getHeight() == 0 || (stencilbuffer->getDepth() <= mStencilbufferLayer))
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(IsRenderbuffer(mStencilbufferType))
		{
			if(!es2::IsStencilRenderable(stencilbuffer->getFormat(), version))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else if(IsTextureTarget(mStencilbufferType))
		{
			GLenum internalformat = stencilbuffer->getFormat();

			if(!es2::IsStencilTexture(internalformat))
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
		else
		{
			if(version < 3 && (width != stencilbuffer->getWidth() || height != stencilbuffer->getHeight()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
			}

			if(samples != stencilbuffer->getSamples())
			{
				return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE;
			}

			width = std::min(width, stencilbuffer->getWidth());
			height = std::min(height, stencilbuffer->getHeight());
		}
	}

	if((version >= 3) && depthbuffer && stencilbuffer && (depthbuffer != stencilbuffer))
	{
		// In the GLES 3.0 spec, section 4.4.4, Framebuffer Completeness:
		// "The framebuffer object target is said to be framebuffer complete if all the following conditions are true:
		//  [...]
		//  Depth and stencil attachments, if present, are the same image.
		//  { FRAMEBUFFER_UNSUPPORTED }"
		return GL_FRAMEBUFFER_UNSUPPORTED;
	}

	// We need to have at least one attachment to be complete
	if(width == -1 || height == -1)
	{
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
	}

	return GL_FRAMEBUFFER_COMPLETE;
}

GLenum Framebuffer::getImplementationColorReadFormat() const
{
	Renderbuffer *colorbuffer = getReadColorbuffer();

	if(colorbuffer)
	{
		switch(colorbuffer->getFormat())
		{
		case GL_BGRA8_EXT:      return GL_BGRA_EXT;
		case GL_RGBA4:          return GL_RGBA;
		case GL_RGB5_A1:        return GL_RGBA;
		case GL_RGBA8:          return GL_RGBA;
		case GL_RGB565:         return GL_RGBA;
		case GL_RGB8:           return GL_RGB;
		case GL_R8:             return GL_RED;
		case GL_RG8:            return GL_RG;
		case GL_R8I:            return GL_RED_INTEGER;
		case GL_RG8I:           return GL_RG_INTEGER;
		case GL_RGB8I:          return GL_RGB_INTEGER;
		case GL_RGBA8I:         return GL_RGBA_INTEGER;
		case GL_R8UI:           return GL_RED_INTEGER;
		case GL_RG8UI:          return GL_RG_INTEGER;
		case GL_RGB8UI:         return GL_RGB_INTEGER;
		case GL_RGBA8UI:        return GL_RGBA_INTEGER;
		case GL_R16I:           return GL_RED_INTEGER;
		case GL_RG16I:          return GL_RG_INTEGER;
		case GL_RGB16I:         return GL_RGB_INTEGER;
		case GL_RGBA16I:        return GL_RGBA_INTEGER;
		case GL_R16UI:          return GL_RED_INTEGER;
		case GL_RG16UI:         return GL_RG_INTEGER;
		case GL_RGB16UI:        return GL_RGB_INTEGER;
		case GL_RGB10_A2UI:     return GL_RGBA_INTEGER;
		case GL_RGBA16UI:       return GL_RGBA_INTEGER;
		case GL_R32I:           return GL_RED_INTEGER;
		case GL_RG32I:          return GL_RG_INTEGER;
		case GL_RGB32I:         return GL_RGB_INTEGER;
		case GL_RGBA32I:        return GL_RGBA_INTEGER;
		case GL_R32UI:          return GL_RED_INTEGER;
		case GL_RG32UI:         return GL_RG_INTEGER;
		case GL_RGB32UI:        return GL_RGB_INTEGER;
		case GL_RGBA32UI:       return GL_RGBA_INTEGER;
		case GL_R16F:           return GL_RED;
		case GL_RG16F:          return GL_RG;
		case GL_R11F_G11F_B10F: return GL_RGB;
		case GL_RGB16F:         return GL_RGB;
		case GL_RGBA16F:        return GL_RGBA;
		case GL_R32F:           return GL_RED;
		case GL_RG32F:          return GL_RG;
		case GL_RGB32F:         return GL_RGB;
		case GL_RGBA32F:        return GL_RGBA;
		case GL_RGB10_A2:       return GL_RGBA;
		case GL_SRGB8:          return GL_RGB;
		case GL_SRGB8_ALPHA8:   return GL_RGBA;
		default:
			UNREACHABLE(colorbuffer->getFormat());
		}
	}

	return GL_RGBA;
}

GLenum Framebuffer::getImplementationColorReadType() const
{
	Renderbuffer *colorbuffer = getReadColorbuffer();

	if(colorbuffer)
	{
		switch(colorbuffer->getFormat())
		{
		case GL_BGRA8_EXT:      return GL_UNSIGNED_BYTE;
		case GL_RGBA4:          return GL_UNSIGNED_SHORT_4_4_4_4;
		case GL_RGB5_A1:        return GL_UNSIGNED_SHORT_5_5_5_1;
		case GL_RGBA8:          return GL_UNSIGNED_BYTE;
		case GL_RGB565:         return GL_UNSIGNED_SHORT_5_6_5;
		case GL_RGB8:           return GL_UNSIGNED_BYTE;
		case GL_R8:             return GL_UNSIGNED_BYTE;
		case GL_RG8:            return GL_UNSIGNED_BYTE;
		case GL_R8I:            return GL_INT;
		case GL_RG8I:           return GL_INT;
		case GL_RGB8I:          return GL_INT;
		case GL_RGBA8I:         return GL_INT;
		case GL_R8UI:           return GL_UNSIGNED_BYTE;
		case GL_RG8UI:          return GL_UNSIGNED_BYTE;
		case GL_RGB8UI:         return GL_UNSIGNED_BYTE;
		case GL_RGBA8UI:        return GL_UNSIGNED_BYTE;
		case GL_R16I:           return GL_INT;
		case GL_RG16I:          return GL_INT;
		case GL_RGB16I:         return GL_INT;
		case GL_RGBA16I:        return GL_INT;
		case GL_R16UI:          return GL_UNSIGNED_INT;
		case GL_RG16UI:         return GL_UNSIGNED_INT;
		case GL_RGB16UI:        return GL_UNSIGNED_INT;
		case GL_RGB10_A2UI:     return GL_UNSIGNED_INT_2_10_10_10_REV;
		case GL_RGBA16UI:       return GL_UNSIGNED_INT;
		case GL_R32I:           return GL_INT;
		case GL_RG32I:          return GL_INT;
		case GL_RGB32I:         return GL_INT;
		case GL_RGBA32I:        return GL_INT;
		case GL_R32UI:          return GL_UNSIGNED_INT;
		case GL_RG32UI:         return GL_UNSIGNED_INT;
		case GL_RGB32UI:        return GL_UNSIGNED_INT;
		case GL_RGBA32UI:       return GL_UNSIGNED_INT;
		case GL_R16F:           return GL_FLOAT;
		case GL_RG16F:          return GL_FLOAT;
		case GL_R11F_G11F_B10F: return GL_FLOAT;
		case GL_RGB16F:         return GL_FLOAT;
		case GL_RGBA16F:        return GL_FLOAT;
		case GL_R32F:           return GL_FLOAT;
		case GL_RG32F:          return GL_FLOAT;
		case GL_RGB32F:         return GL_FLOAT;
		case GL_RGBA32F:        return GL_FLOAT;
		case GL_RGB10_A2:       return GL_UNSIGNED_INT_2_10_10_10_REV;
		case GL_SRGB8:          return GL_UNSIGNED_BYTE;
		case GL_SRGB8_ALPHA8:   return GL_UNSIGNED_BYTE;
		default:
			UNREACHABLE(colorbuffer->getFormat());
		}
	}

	return GL_UNSIGNED_BYTE;
}

GLenum Framebuffer::getDepthReadFormat() const
{
	Renderbuffer *depthbuffer = getDepthbuffer();

	if(depthbuffer)
	{
		// There is only one depth read format.
		return GL_DEPTH_COMPONENT;
	}

	// If there is no depth buffer, GL_INVALID_OPERATION occurs.
	return GL_NONE;
}

GLenum Framebuffer::getDepthReadType() const
{
	Renderbuffer *depthbuffer = getDepthbuffer();

	if(depthbuffer)
	{
		switch(depthbuffer->getFormat())
		{
		case GL_DEPTH_COMPONENT16:     return GL_UNSIGNED_SHORT;
		case GL_DEPTH_COMPONENT24:     return GL_UNSIGNED_INT;
		case GL_DEPTH_COMPONENT32_OES: return GL_UNSIGNED_INT;
		case GL_DEPTH_COMPONENT32F:    return GL_FLOAT;
		case GL_DEPTH24_STENCIL8:      return GL_UNSIGNED_INT_24_8_OES;
		case GL_DEPTH32F_STENCIL8:     return GL_FLOAT;
		default:
			UNREACHABLE(depthbuffer->getFormat());
		}
	}

	// If there is no depth buffer, GL_INVALID_OPERATION occurs.
	return GL_NONE;
}

GLuint Framebuffer::getReadBufferIndex() const
{
	switch(readBuffer)
	{
	case GL_BACK:
		return 0;
	case GL_NONE:
		return GL_INVALID_INDEX;
	default:
		return readBuffer - GL_COLOR_ATTACHMENT0;
	}
}

DefaultFramebuffer::DefaultFramebuffer()
{
	readBuffer = GL_BACK;
	drawBuffer[0] = GL_BACK;
}

DefaultFramebuffer::DefaultFramebuffer(Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil)
{
	GLenum defaultRenderbufferType = egl::getClientVersion() < 3 ? GL_RENDERBUFFER : GL_FRAMEBUFFER_DEFAULT;
	mColorbufferPointer[0] = new Renderbuffer(0, colorbuffer);
	mColorbufferType[0] = defaultRenderbufferType;

	readBuffer = GL_BACK;
	drawBuffer[0] = GL_BACK;
	for(int i = 1; i < MAX_COLOR_ATTACHMENTS; i++)
	{
		mColorbufferPointer[i] = nullptr;
		mColorbufferType[i] = GL_NONE;
	}

	Renderbuffer *depthStencilRenderbuffer = new Renderbuffer(0, depthStencil);
	mDepthbufferPointer = depthStencilRenderbuffer;
	mStencilbufferPointer = depthStencilRenderbuffer;

	mDepthbufferType = (depthStencilRenderbuffer->getDepthSize() != 0) ? defaultRenderbufferType : GL_NONE;
	mStencilbufferType = (depthStencilRenderbuffer->getStencilSize() != 0) ? defaultRenderbufferType : GL_NONE;
}

}
