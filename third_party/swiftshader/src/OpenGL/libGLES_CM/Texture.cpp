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

// Texture.cpp: Implements the Texture class and its derived classes
// Texture2D and TextureCubeMap. Implements GL texture objects and related
// functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "Texture.h"

#include "main.h"
#include "mathutil.h"
#include "Framebuffer.h"
#include "Device.hpp"
#include "libEGL/Display.h"
#include "common/Surface.hpp"
#include "common/debug.h"

#include <algorithm>

namespace es1
{

Texture::Texture(GLuint name) : egl::Texture(name)
{
	mMinFilter = GL_NEAREST_MIPMAP_LINEAR;
	mMagFilter = GL_LINEAR;
	mWrapS = GL_REPEAT;
	mWrapT = GL_REPEAT;
	mMaxAnisotropy = 1.0f;
	generateMipmap = GL_FALSE;
	cropRectU = 0;
	cropRectV = 0;
	cropRectW = 0;
	cropRectH = 0;

	resource = new sw::Resource(0);
}

Texture::~Texture()
{
	resource->destruct();
}

sw::Resource *Texture::getResource() const
{
	return resource;
}

// Returns true on successful filter state update (valid enum parameter)
bool Texture::setMinFilter(GLenum filter)
{
	switch(filter)
	{
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
	case GL_LINEAR_MIPMAP_LINEAR:
		if(getTarget() == GL_TEXTURE_EXTERNAL_OES)
		{
			return false;
		}
		// Fall through
	case GL_NEAREST:
	case GL_LINEAR:
		mMinFilter = filter;
		return true;
	default:
		return false;
	}
}

// Returns true on successful filter state update (valid enum parameter)
bool Texture::setMagFilter(GLenum filter)
{
	switch(filter)
	{
	case GL_NEAREST:
	case GL_LINEAR:
		mMagFilter = filter;
		return true;
	default:
		return false;
	}
}

// Returns true on successful wrap state update (valid enum parameter)
bool Texture::setWrapS(GLenum wrap)
{
	switch(wrap)
	{
	case GL_REPEAT:
	case GL_MIRRORED_REPEAT_OES:
		if(getTarget() == GL_TEXTURE_EXTERNAL_OES)
		{
			return false;
		}
		// Fall through
	case GL_CLAMP_TO_EDGE:
		mWrapS = wrap;
		return true;
	default:
		return false;
	}
}

// Returns true on successful wrap state update (valid enum parameter)
bool Texture::setWrapT(GLenum wrap)
{
	switch(wrap)
	{
	case GL_REPEAT:
	case GL_MIRRORED_REPEAT_OES:
		if(getTarget() == GL_TEXTURE_EXTERNAL_OES)
		{
			return false;
		}
		// Fall through
	case GL_CLAMP_TO_EDGE:
		 mWrapT = wrap;
		 return true;
	default:
		return false;
	}
}

// Returns true on successful max anisotropy update (valid anisotropy value)
bool Texture::setMaxAnisotropy(float textureMaxAnisotropy)
{
	textureMaxAnisotropy = std::min(textureMaxAnisotropy, MAX_TEXTURE_MAX_ANISOTROPY);

	if(textureMaxAnisotropy < 1.0f)
	{
		return false;
	}

	if(mMaxAnisotropy != textureMaxAnisotropy)
	{
		mMaxAnisotropy = textureMaxAnisotropy;
	}

	return true;
}

void Texture::setGenerateMipmap(GLboolean enable)
{
	generateMipmap = enable;
}

void Texture::setCropRect(GLint u, GLint v, GLint w, GLint h)
{
	cropRectU = u;
	cropRectV = v;
	cropRectW = w;
	cropRectH = h;
}

GLenum Texture::getMinFilter() const
{
	return mMinFilter;
}

GLenum Texture::getMagFilter() const
{
	return mMagFilter;
}

GLenum Texture::getWrapS() const
{
	return mWrapS;
}

GLenum Texture::getWrapT() const
{
	return mWrapT;
}

GLfloat Texture::getMaxAnisotropy() const
{
	return mMaxAnisotropy;
}

GLboolean Texture::getGenerateMipmap() const
{
	return generateMipmap;
}

GLint Texture::getCropRectU() const
{
	return cropRectU;
}

GLint Texture::getCropRectV() const
{
	return cropRectV;
}

GLint Texture::getCropRectW() const
{
	return cropRectW;
}

GLint Texture::getCropRectH() const
{
	return cropRectH;
}

egl::Image *Texture::createSharedImage(GLenum target, unsigned int level)
{
	egl::Image *image = getRenderTarget(target, level);   // Increments reference count

	if(image)
	{
		image->markShared();
	}

	return image;
}

void Texture::setImage(GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, egl::Image *image)
{
	if(pixels && image)
	{
		gl::PixelStorageModes unpackParameters;
		unpackParameters.alignment = unpackAlignment;
		image->loadImageData(0, 0, 0, image->getWidth(), image->getHeight(), 1, format, type, unpackParameters, pixels);
	}
}

void Texture::setCompressedImage(GLsizei imageSize, const void *pixels, egl::Image *image)
{
	if(pixels && image && (imageSize > 0)) // imageSize's correlation to width and height is already validated with gl::ComputeCompressedSize() at the API level
	{
		image->loadCompressedData(0, 0, 0, image->getWidth(), image->getHeight(), 1, imageSize, pixels);
	}
}

void Texture::subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, egl::Image *image)
{
	if(!image)
	{
		return error(GL_INVALID_OPERATION);
	}

	if(pixels)
	{
		gl::PixelStorageModes unpackParameters;
		unpackParameters.alignment = unpackAlignment;
		image->loadImageData(xoffset, yoffset, 0, width, height, 1, format, type, unpackParameters, pixels);
	}
}

void Texture::subImageCompressed(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels, egl::Image *image)
{
	if(!image)
	{
		return error(GL_INVALID_OPERATION);
	}

	if(pixels && (imageSize > 0)) // imageSize's correlation to width and height is already validated with gl::ComputeCompressedSize() at the API level
	{
		image->loadCompressedData(xoffset, yoffset, 0, width, height, 1, imageSize, pixels);
	}
}

bool Texture::copy(egl::Image *source, const sw::Rect &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, egl::Image *dest)
{
	Device *device = getDevice();

	sw::SliceRect destRect(xoffset, yoffset, xoffset + (sourceRect.x1 - sourceRect.x0), yoffset + (sourceRect.y1 - sourceRect.y0), 0);
	sw::SliceRect sourceSliceRect(sourceRect);
	bool success = device->stretchRect(source, &sourceSliceRect, dest, &destRect, false);

	if(!success)
	{
		return error(GL_OUT_OF_MEMORY, false);
	}

	return true;
}

bool Texture::isMipmapFiltered() const
{
	switch(mMinFilter)
	{
	case GL_NEAREST:
	case GL_LINEAR:
		return false;
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
	case GL_LINEAR_MIPMAP_LINEAR:
		return true;
	default: UNREACHABLE(mMinFilter);
	}

	return false;
}

Texture2D::Texture2D(GLuint name) : Texture(name)
{
	for(int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
	{
		image[i] = nullptr;
	}

	mSurface = nullptr;

	mColorbufferProxy = nullptr;
	mProxyRefs = 0;
}

Texture2D::~Texture2D()
{
	for(int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
	{
		if(image[i])
		{
			image[i]->unbind(this);
			image[i] = nullptr;
		}
	}

	if(mSurface)
	{
		mSurface->setBoundTexture(nullptr);
		mSurface = nullptr;
	}

	mColorbufferProxy = nullptr;
}

// We need to maintain a count of references to renderbuffers acting as
// proxies for this texture, so that we do not attempt to use a pointer
// to a renderbuffer proxy which has been deleted.
void Texture2D::addProxyRef(const Renderbuffer *proxy)
{
	mProxyRefs++;
}

void Texture2D::releaseProxy(const Renderbuffer *proxy)
{
	if(mProxyRefs > 0)
	{
		mProxyRefs--;
	}

	if(mProxyRefs == 0)
	{
		mColorbufferProxy = nullptr;
	}
}

void Texture2D::sweep()
{
	int imageCount = 0;

	for(int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
	{
		if(image[i] && image[i]->isChildOf(this))
		{
			if(!image[i]->hasSingleReference())
			{
				return;
			}

			imageCount++;
		}
	}

	if(imageCount == referenceCount)
	{
		destroy();
	}
}

GLenum Texture2D::getTarget() const
{
	return GL_TEXTURE_2D;
}

GLsizei Texture2D::getWidth(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D);
	return image[level] ? image[level]->getWidth() : 0;
}

GLsizei Texture2D::getHeight(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D);
	return image[level] ? image[level]->getHeight() : 0;
}

GLint Texture2D::getFormat(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D);
	return image[level] ? image[level]->getFormat() : GL_NONE;
}

int Texture2D::getTopLevel() const
{
	ASSERT(isSamplerComplete());
	int level = 0;

	while(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && image[level])
	{
		level++;
	}

	return level - 1;
}

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLint internalformat, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
	if(image[level])
	{
		image[level]->release();
	}

	image[level] = egl::Image::create(this, width, height, internalformat);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	Texture::setImage(format, type, unpackAlignment, pixels, image[level]);
}

void Texture2D::bindTexImage(gl::Surface *surface)
{
	for(int level = 0; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
	{
		if(image[level])
		{
			image[level]->release();
			image[level] = nullptr;
		}
	}

	image[0] = surface->getRenderTarget();

	mSurface = surface;
	mSurface->setBoundTexture(this);
}

void Texture2D::releaseTexImage()
{
	for(int level = 0; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
	{
		if(image[level])
		{
			image[level]->release();
			image[level] = nullptr;
		}
	}
}

void Texture2D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
	if(image[level])
	{
		image[level]->release();
	}

	image[level] = egl::Image::create(this, width, height, format);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	Texture::setCompressedImage(imageSize, pixels, image[level]);
}

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
	Texture::subImage(xoffset, yoffset, width, height, format, type, unpackAlignment, pixels, image[level]);
}

void Texture2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
	Texture::subImageCompressed(xoffset, yoffset, width, height, format, imageSize, pixels, image[level]);
}

void Texture2D::copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
	egl::Image *renderTarget = source->getRenderTarget();

	if(!renderTarget)
	{
		ERR("Failed to retrieve the render target.");
		return error(GL_OUT_OF_MEMORY);
	}

	if(image[level])
	{
		image[level]->release();
	}

	image[level] = egl::Image::create(this, width, height, format);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	if(width != 0 && height != 0)
	{
		sw::Rect sourceRect = {x, y, x + width, y + height};
		sourceRect.clip(0, 0, source->getColorbuffer()->getWidth(), source->getColorbuffer()->getHeight());

		copy(renderTarget, sourceRect, format, 0, 0, image[level]);
	}

	renderTarget->release();
}

void Texture2D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
	if(!image[level])
	{
		return error(GL_INVALID_OPERATION);
	}

	if(xoffset + width > image[level]->getWidth() || yoffset + height > image[level]->getHeight())
	{
		return error(GL_INVALID_VALUE);
	}

	egl::Image *renderTarget = source->getRenderTarget();

	if(!renderTarget)
	{
		ERR("Failed to retrieve the render target.");
		return error(GL_OUT_OF_MEMORY);
	}

	sw::Rect sourceRect = {x, y, x + width, y + height};
	sourceRect.clip(0, 0, source->getColorbuffer()->getWidth(), source->getColorbuffer()->getHeight());

	copy(renderTarget, sourceRect, image[level]->getFormat(), xoffset, yoffset, image[level]);

	renderTarget->release();
}

void Texture2D::setSharedImage(egl::Image *sharedImage)
{
	sharedImage->addRef();

	if(image[0])
	{
		image[0]->release();
	}

	image[0] = sharedImage;
}

// Tests for 2D texture sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 85.
bool Texture2D::isSamplerComplete() const
{
	if(!image[0])
	{
		return false;
	}

	GLsizei width = image[0]->getWidth();
	GLsizei height = image[0]->getHeight();

	if(width <= 0 || height <= 0)
	{
		return false;
	}

	if(isMipmapFiltered())
	{
		if(!generateMipmap && !isMipmapComplete())
		{
			return false;
		}
	}

	return true;
}

// Tests for 2D texture (mipmap) completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture2D::isMipmapComplete() const
{
	GLsizei width = image[0]->getWidth();
	GLsizei height = image[0]->getHeight();

	int q = log2(std::max(width, height));

	for(int level = 1; level <= q; level++)
	{
		if(!image[level])
		{
			return false;
		}

		if(image[level]->getFormat() != image[0]->getFormat())
		{
			return false;
		}

		if(image[level]->getWidth() != std::max(1, width >> level))
		{
			return false;
		}

		if(image[level]->getHeight() != std::max(1, height >> level))
		{
			return false;
		}
	}

	return true;
}

bool Texture2D::isCompressed(GLenum target, GLint level) const
{
	return IsCompressed(getFormat(target, level));
}

bool Texture2D::isDepth(GLenum target, GLint level) const
{
	return IsDepthTexture(getFormat(target, level));
}

void Texture2D::generateMipmaps()
{
	if(!image[0])
	{
		return;   // FIXME: error?
	}

	unsigned int q = log2(std::max(image[0]->getWidth(), image[0]->getHeight()));

	for(unsigned int i = 1; i <= q; i++)
	{
		if(image[i])
		{
			image[i]->release();
		}

		image[i] = egl::Image::create(this, std::max(image[0]->getWidth() >> i, 1), std::max(image[0]->getHeight() >> i, 1), image[0]->getFormat());

		if(!image[i])
		{
			return error(GL_OUT_OF_MEMORY);
		}

		getDevice()->stretchRect(image[i - 1], 0, image[i], 0, true);
	}
}

void Texture2D::autoGenerateMipmaps()
{
	if(generateMipmap && image[0]->hasDirtyContents())
	{
		generateMipmaps();
		image[0]->markContentsClean();
	}
}

egl::Image *Texture2D::getImage(unsigned int level)
{
	return image[level];
}

Renderbuffer *Texture2D::getRenderbuffer(GLenum target)
{
	if(target != GL_TEXTURE_2D)
	{
		return error(GL_INVALID_OPERATION, (Renderbuffer*)nullptr);
	}

	if(!mColorbufferProxy)
	{
		mColorbufferProxy = new Renderbuffer(name, new RenderbufferTexture2D(this));
	}

	return mColorbufferProxy;
}

egl::Image *Texture2D::getRenderTarget(GLenum target, unsigned int level)
{
	ASSERT(target == GL_TEXTURE_2D);
	ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

	if(image[level])
	{
		image[level]->addRef();
	}

	return image[level];
}

bool Texture2D::isShared(GLenum target, unsigned int level) const
{
	ASSERT(target == GL_TEXTURE_2D);
	ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

	if(mSurface)   // Bound to an EGLSurface
	{
		return true;
	}

	if(!image[level])
	{
		return false;
	}

	return image[level]->isShared();
}

TextureExternal::TextureExternal(GLuint name) : Texture2D(name)
{
	mMinFilter = GL_LINEAR;
	mMagFilter = GL_LINEAR;
	mWrapS = GL_CLAMP_TO_EDGE;
	mWrapT = GL_CLAMP_TO_EDGE;
}

TextureExternal::~TextureExternal()
{
}

GLenum TextureExternal::getTarget() const
{
	return GL_TEXTURE_EXTERNAL_OES;
}

}

egl::Image *createBackBuffer(int width, int height, sw::Format format, int multiSampleDepth)
{
	if(width > es1::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE || height > es1::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE)
	{
		ERR("Invalid parameters: %dx%d", width, height);
		return nullptr;
	}

	GLenum internalformat = sw2es::ConvertBackBufferFormat(format);

	return egl::Image::create(width, height, internalformat, multiSampleDepth, false);
}

egl::Image *createDepthStencil(int width, int height, sw::Format format, int multiSampleDepth)
{
	if(width > es1::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE || height > es1::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE)
	{
		ERR("Invalid parameters: %dx%d", width, height);
		return nullptr;
	}

	bool lockable = true;

	switch(format)
	{
//	case sw::FORMAT_D15S1:
	case sw::FORMAT_D24S8:
	case sw::FORMAT_D24X8:
//	case sw::FORMAT_D24X4S4:
	case sw::FORMAT_D24FS8:
	case sw::FORMAT_D32:
	case sw::FORMAT_D16:
		lockable = false;
		break;
//	case sw::FORMAT_S8_LOCKABLE:
//	case sw::FORMAT_D16_LOCKABLE:
	case sw::FORMAT_D32F_LOCKABLE:
//	case sw::FORMAT_D32_LOCKABLE:
	case sw::FORMAT_DF24S8:
	case sw::FORMAT_DF16S8:
		lockable = true;
		break;
	default:
		UNREACHABLE(format);
	}

	GLenum internalformat = sw2es::ConvertDepthStencilFormat(format);

	egl::Image *surface = egl::Image::create(width, height, internalformat, multiSampleDepth, lockable);

	if(!surface)
	{
		ERR("Out of memory");
		return nullptr;
	}

	return surface;
}
