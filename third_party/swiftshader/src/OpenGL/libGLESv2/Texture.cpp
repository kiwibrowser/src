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
// Texture2D, TextureCubeMap, Texture3D and Texture2DArray. Implements GL texture objects
// and related functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "Texture.h"

#include "main.h"
#include "mathutil.h"
#include "Framebuffer.h"
#include "Device.hpp"
#include "Shader.h"
#include "libEGL/Display.h"
#include "common/Surface.hpp"
#include "common/debug.h"

#include <algorithm>

namespace es2
{

Texture::Texture(GLuint name) : egl::Texture(name)
{
	mMinFilter = GL_NEAREST_MIPMAP_LINEAR;
	mMagFilter = GL_LINEAR;
	mWrapS = GL_REPEAT;
	mWrapT = GL_REPEAT;
	mWrapR = GL_REPEAT;
	mMaxAnisotropy = 1.0f;
	mBaseLevel = 0;
	mCompareFunc = GL_LEQUAL;
	mCompareMode = GL_NONE;
	mImmutableFormat = GL_FALSE;
	mImmutableLevels = 0;
	mMaxLevel = 1000;
	mMaxLOD = 1000;
	mMinLOD = -1000;
	mSwizzleR = GL_RED;
	mSwizzleG = GL_GREEN;
	mSwizzleB = GL_BLUE;
	mSwizzleA = GL_ALPHA;

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
		if((getTarget() == GL_TEXTURE_EXTERNAL_OES) || (getTarget() == GL_TEXTURE_RECTANGLE_ARB))
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
	case GL_MIRRORED_REPEAT:
		if((getTarget() == GL_TEXTURE_EXTERNAL_OES) || (getTarget() == GL_TEXTURE_RECTANGLE_ARB))
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
	case GL_MIRRORED_REPEAT:
		if((getTarget() == GL_TEXTURE_EXTERNAL_OES) || (getTarget() == GL_TEXTURE_RECTANGLE_ARB))
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

// Returns true on successful wrap state update (valid enum parameter)
bool Texture::setWrapR(GLenum wrap)
{
	switch(wrap)
	{
	case GL_REPEAT:
	case GL_MIRRORED_REPEAT:
		if((getTarget() == GL_TEXTURE_EXTERNAL_OES) || (getTarget() == GL_TEXTURE_RECTANGLE_ARB))
		{
			return false;
		}
		// Fall through
	case GL_CLAMP_TO_EDGE:
		mWrapR = wrap;
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

bool Texture::setBaseLevel(GLint baseLevel)
{
	if(baseLevel < 0)
	{
		return false;
	}

	mBaseLevel = baseLevel;
	return true;
}

bool Texture::setCompareFunc(GLenum compareFunc)
{
	switch(compareFunc)
	{
	case GL_LEQUAL:
	case GL_GEQUAL:
	case GL_LESS:
	case GL_GREATER:
	case GL_EQUAL:
	case GL_NOTEQUAL:
	case GL_ALWAYS:
	case GL_NEVER:
		mCompareFunc = compareFunc;
		return true;
	default:
		return false;
	}
}

bool Texture::setCompareMode(GLenum compareMode)
{
	switch(compareMode)
	{
	case GL_COMPARE_REF_TO_TEXTURE:
	case GL_NONE:
		mCompareMode = compareMode;
		return true;
	default:
		return false;
	}
}

void Texture::makeImmutable(GLsizei levels)
{
	mImmutableFormat = GL_TRUE;
	mImmutableLevels = levels;
}

bool Texture::setMaxLevel(GLint maxLevel)
{
	mMaxLevel = maxLevel;
	return true;
}

bool Texture::setMaxLOD(GLfloat maxLOD)
{
	mMaxLOD = maxLOD;
	return true;
}

bool Texture::setMinLOD(GLfloat minLOD)
{
	mMinLOD = minLOD;
	return true;
}

bool Texture::setSwizzleR(GLenum swizzleR)
{
	switch(swizzleR)
	{
	case GL_RED:
	case GL_GREEN:
	case GL_BLUE:
	case GL_ALPHA:
	case GL_ZERO:
	case GL_ONE:
		mSwizzleR = swizzleR;
		return true;
	default:
		return false;
	}
}

bool Texture::setSwizzleG(GLenum swizzleG)
{
	switch(swizzleG)
	{
	case GL_RED:
	case GL_GREEN:
	case GL_BLUE:
	case GL_ALPHA:
	case GL_ZERO:
	case GL_ONE:
		mSwizzleG = swizzleG;
		return true;
	default:
		return false;
	}
}

bool Texture::setSwizzleB(GLenum swizzleB)
{
	switch(swizzleB)
	{
	case GL_RED:
	case GL_GREEN:
	case GL_BLUE:
	case GL_ALPHA:
	case GL_ZERO:
	case GL_ONE:
		mSwizzleB = swizzleB;
		return true;
	default:
		return false;
	}
}

bool Texture::setSwizzleA(GLenum swizzleA)
{
	switch(swizzleA)
	{
	case GL_RED:
	case GL_GREEN:
	case GL_BLUE:
	case GL_ALPHA:
	case GL_ZERO:
	case GL_ONE:
		mSwizzleA = swizzleA;
		return true;
	default:
		return false;
	}
}

GLsizei Texture::getDepth(GLenum target, GLint level) const
{
	return 1;
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

void Texture::setImage(GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels, egl::Image *image)
{
	if(pixels && image)
	{
		GLsizei depth = (getTarget() == GL_TEXTURE_3D_OES || getTarget() == GL_TEXTURE_2D_ARRAY) ? image->getDepth() : 1;
		image->loadImageData(0, 0, 0, image->getWidth(), image->getHeight(), depth, format, type, unpackParameters, pixels);
	}
}

void Texture::setCompressedImage(GLsizei imageSize, const void *pixels, egl::Image *image)
{
	if(pixels && image && (imageSize > 0)) // imageSize's correlation to width and height is already validated with gl::ComputeCompressedSize() at the API level
	{
		GLsizei depth = (getTarget() == GL_TEXTURE_3D_OES || getTarget() == GL_TEXTURE_2D_ARRAY) ? image->getDepth() : 1;
		image->loadCompressedData(0, 0, 0, image->getWidth(), image->getHeight(), depth, imageSize, pixels);
	}
}

void Texture::subImage(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels, egl::Image *image)
{
	if(!image)
	{
		return error(GL_INVALID_OPERATION);
	}

	if(pixels && width > 0 && height > 0 && depth > 0)
	{
		image->loadImageData(xoffset, yoffset, zoffset, width, height, depth, format, type, unpackParameters, pixels);
	}
}

void Texture::subImageCompressed(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels, egl::Image *image)
{
	if(!image)
	{
		return error(GL_INVALID_OPERATION);
	}

	if(pixels && (imageSize > 0)) // imageSize's correlation to width and height is already validated with gl::ComputeCompressedSize() at the API level
	{
		image->loadCompressedData(xoffset, yoffset, zoffset, width, height, depth, imageSize, pixels);
	}
}

bool Texture::copy(egl::Image *source, const sw::SliceRect &sourceRect, GLint xoffset, GLint yoffset, GLint zoffset, egl::Image *dest)
{
	Device *device = getDevice();

	sw::SliceRect destRect(xoffset, yoffset, xoffset + (sourceRect.x1 - sourceRect.x0), yoffset + (sourceRect.y1 - sourceRect.y0), zoffset);
	sw::SliceRectF sourceRectF(static_cast<float>(sourceRect.x0),
	                           static_cast<float>(sourceRect.y0),
	                           static_cast<float>(sourceRect.x1),
	                           static_cast<float>(sourceRect.y1),
	                           sourceRect.slice);
	bool success = device->stretchRect(source, &sourceRectF, dest, &destRect, Device::ALL_BUFFERS);

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
	ASSERT(target == getTarget());
	return image[level] ? image[level]->getWidth() : 0;
}

GLsizei Texture2D::getHeight(GLenum target, GLint level) const
{
	ASSERT(target == getTarget());
	return image[level] ? image[level]->getHeight() : 0;
}

GLint Texture2D::getFormat(GLenum target, GLint level) const
{
	ASSERT(target == getTarget());
	return image[level] ? image[level]->getFormat() : GL_NONE;
}

int Texture2D::getTopLevel() const
{
	ASSERT(isSamplerComplete());
	int level = mBaseLevel;

	while(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && image[level])
	{
		level++;
	}

	return level - 1;
}

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLint internalformat, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels)
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

	Texture::setImage(format, type, unpackParameters, pixels, image[level]);
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

	if(mSurface)
	{
		mSurface->setBoundTexture(nullptr);
		mSurface = nullptr;
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

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels)
{
	Texture::subImage(xoffset, yoffset, 0, width, height, 1, format, type, unpackParameters, pixels, image[level]);
}

void Texture2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
	Texture::subImageCompressed(xoffset, yoffset, 0, width, height, 1, format, imageSize, pixels, image[level]);
}

void Texture2D::copyImage(GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source)
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

	image[level] = egl::Image::create(this, width, height, internalformat);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	if(width != 0 && height != 0)
	{
		sw::SliceRect sourceRect(x, y, x + width, y + height, 0);
		sourceRect.clip(0, 0, renderTarget->getWidth(), renderTarget->getHeight());

		copy(renderTarget, sourceRect, 0, 0, 0, image[level]);
	}

	renderTarget->release();
}

void Texture2D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source)
{
	if(!image[level])
	{
		return error(GL_INVALID_OPERATION);
	}

	if(xoffset + width > image[level]->getWidth() || yoffset + height > image[level]->getHeight() || zoffset != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(width > 0 && height > 0)
	{
		egl::Image *renderTarget = source->getRenderTarget();

		if(!renderTarget)
		{
			ERR("Failed to retrieve the render target.");
			return error(GL_OUT_OF_MEMORY);
		}

		sw::SliceRect sourceRect(x, y, x + width, y + height, 0);
		sourceRect.clip(0, 0, renderTarget->getWidth(), renderTarget->getHeight());

		copy(renderTarget, sourceRect, xoffset, yoffset, zoffset, image[level]);

		renderTarget->release();
	}
}

void Texture2D::setSharedImage(egl::Image *sharedImage)
{
	if(sharedImage == image[0])
	{
		return;
	}

	sharedImage->addRef();

	if(image[0])
	{
		image[0]->release();
	}

	image[0] = sharedImage;
}

// Tests for 2D texture sampling completeness. [OpenGL ES 3.0.5] section 3.8.13 page 160.
bool Texture2D::isSamplerComplete() const
{
	if(!image[mBaseLevel])
	{
		return false;
	}

	GLsizei width = image[mBaseLevel]->getWidth();
	GLsizei height = image[mBaseLevel]->getHeight();

	if(width <= 0 || height <= 0)
	{
		return false;
	}

	if(isMipmapFiltered())
	{
		if(!isMipmapComplete())
		{
			return false;
		}
	}

	return true;
}

// Tests for 2D texture (mipmap) completeness. [OpenGL ES 3.0.5] section 3.8.13 page 160.
bool Texture2D::isMipmapComplete() const
{
	if(mBaseLevel > mMaxLevel)
	{
		return false;
	}

	GLsizei width = image[mBaseLevel]->getWidth();
	GLsizei height = image[mBaseLevel]->getHeight();
	int maxsize = std::max(width, height);
	int p = log2(maxsize) + mBaseLevel;
	int q = std::min(p, mMaxLevel);

	for(int level = mBaseLevel + 1; level <= q; level++)
	{
		if(!image[level])
		{
			return false;
		}

		if(image[level]->getFormat() != image[mBaseLevel]->getFormat())
		{
			return false;
		}

		int i = level - mBaseLevel;

		if(image[level]->getWidth() != std::max(1, width >> i))
		{
			return false;
		}

		if(image[level]->getHeight() != std::max(1, height >> i))
		{
			return false;
		}
	}

	return true;
}

bool Texture2D::isCompressed(GLenum target, GLint level) const
{
	return IsCompressed(getFormat(target, level), egl::getClientVersion());
}

bool Texture2D::isDepth(GLenum target, GLint level) const
{
	return IsDepthTexture(getFormat(target, level));
}

void Texture2D::generateMipmaps()
{
	if(!image[mBaseLevel])
	{
		return;   // Image unspecified. Not an error.
	}

	if(image[mBaseLevel]->getWidth() == 0 || image[mBaseLevel]->getHeight() == 0)
	{
		return;   // Zero dimension. Not an error.
	}

	int maxsize = std::max(image[mBaseLevel]->getWidth(), image[mBaseLevel]->getHeight());
	int p = log2(maxsize) + mBaseLevel;
	int q = std::min(p, mMaxLevel);

	for(int i = mBaseLevel + 1; i <= q; i++)
	{
		if(image[i])
		{
			image[i]->release();
		}

		image[i] = egl::Image::create(this, std::max(image[mBaseLevel]->getWidth() >> i, 1), std::max(image[mBaseLevel]->getHeight() >> i, 1), image[mBaseLevel]->getFormat());

		if(!image[i])
		{
			return error(GL_OUT_OF_MEMORY);
		}

		getDevice()->stretchRect(image[i - 1], 0, image[i], 0, Device::ALL_BUFFERS | Device::USE_FILTER);
	}
}

egl::Image *Texture2D::getImage(unsigned int level)
{
	return image[level];
}

Renderbuffer *Texture2D::getRenderbuffer(GLenum target, GLint level)
{
	if(target != getTarget())
	{
		return error(GL_INVALID_OPERATION, (Renderbuffer*)nullptr);
	}

	if(!mColorbufferProxy)
	{
		mColorbufferProxy = new Renderbuffer(name, new RenderbufferTexture2D(this, level));
	}
	else
	{
		mColorbufferProxy->setLevel(level);
	}

	return mColorbufferProxy;
}

egl::Image *Texture2D::getRenderTarget(GLenum target, unsigned int level)
{
	ASSERT(target == getTarget());
	ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

	if(image[level])
	{
		image[level]->addRef();
	}

	return image[level];
}

bool Texture2D::isShared(GLenum target, unsigned int level) const
{
	ASSERT(target == getTarget());
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

Texture2DRect::Texture2DRect(GLuint name) : Texture2D(name)
{
	mMinFilter = GL_LINEAR;
	mMagFilter = GL_LINEAR;
	mWrapS = GL_CLAMP_TO_EDGE;
	mWrapT = GL_CLAMP_TO_EDGE;
	mWrapR = GL_CLAMP_TO_EDGE;
}

GLenum Texture2DRect::getTarget() const
{
	return GL_TEXTURE_RECTANGLE_ARB;
}

Renderbuffer *Texture2DRect::getRenderbuffer(GLenum target, GLint level)
{
	if((target != getTarget()) || (level != 0))
	{
		return error(GL_INVALID_OPERATION, (Renderbuffer*)nullptr);
	}

	if(!mColorbufferProxy)
	{
		mColorbufferProxy = new Renderbuffer(name, new RenderbufferTexture2DRect(this));
	}

	return mColorbufferProxy;
}

TextureCubeMap::TextureCubeMap(GLuint name) : Texture(name)
{
	for(int f = 0; f < 6; f++)
	{
		for(int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
		{
			image[f][i] = nullptr;
		}
	}

	for(int f = 0; f < 6; f++)
	{
		mFaceProxies[f] = nullptr;
		mFaceProxyRefs[f] = 0;
	}
}

TextureCubeMap::~TextureCubeMap()
{
	for(int f = 0; f < 6; f++)
	{
		for(int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
		{
			if(image[f][i])
			{
				image[f][i]->unbind(this);
				image[f][i] = nullptr;
			}
		}
	}

	for(int i = 0; i < 6; i++)
	{
		mFaceProxies[i] = nullptr;
	}
}

// We need to maintain a count of references to renderbuffers acting as
// proxies for this texture, so that the texture is not deleted while
// proxy references still exist. If the reference count drops to zero,
// we set our proxy pointer null, so that a new attempt at referencing
// will cause recreation.
void TextureCubeMap::addProxyRef(const Renderbuffer *proxy)
{
	for(int f = 0; f < 6; f++)
	{
		if(mFaceProxies[f] == proxy)
		{
			mFaceProxyRefs[f]++;
		}
	}
}

void TextureCubeMap::releaseProxy(const Renderbuffer *proxy)
{
	for(int f = 0; f < 6; f++)
	{
		if(mFaceProxies[f] == proxy)
		{
			if(mFaceProxyRefs[f] > 0)
			{
				mFaceProxyRefs[f]--;
			}

			if(mFaceProxyRefs[f] == 0)
			{
				mFaceProxies[f] = nullptr;
			}
		}
	}
}

void TextureCubeMap::sweep()
{
	int imageCount = 0;

	for(int f = 0; f < 6; f++)
	{
		for(int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
		{
			if(image[f][i] && image[f][i]->isChildOf(this))
			{
				if(!image[f][i]->hasSingleReference())
				{
					return;
				}

				imageCount++;
			}
		}
	}

	if(imageCount == referenceCount)
	{
		destroy();
	}
}

GLenum TextureCubeMap::getTarget() const
{
	return GL_TEXTURE_CUBE_MAP;
}

GLsizei TextureCubeMap::getWidth(GLenum target, GLint level) const
{
	int face = CubeFaceIndex(target);
	return image[face][level] ? image[face][level]->getWidth() : 0;
}

GLsizei TextureCubeMap::getHeight(GLenum target, GLint level) const
{
	int face = CubeFaceIndex(target);
	return image[face][level] ? image[face][level]->getHeight() : 0;
}

GLint TextureCubeMap::getFormat(GLenum target, GLint level) const
{
	int face = CubeFaceIndex(target);
	return image[face][level] ? image[face][level]->getFormat() : 0;
}

int TextureCubeMap::getTopLevel() const
{
	ASSERT(isSamplerComplete());
	int level = mBaseLevel;

	while(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && image[0][level])
	{
		level++;
	}

	return level - 1;
}

void TextureCubeMap::setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
	int face = CubeFaceIndex(target);

	if(image[face][level])
	{
		image[face][level]->release();
	}

	int border = (egl::getClientVersion() >= 3) ? 1 : 0;
	image[face][level] = egl::Image::create(this, width, height, 1, border, format);

	if(!image[face][level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	Texture::setCompressedImage(imageSize, pixels, image[face][level]);
}

void TextureCubeMap::subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels)
{
	Texture::subImage(xoffset, yoffset, 0, width, height, 1, format, type, unpackParameters, pixels, image[CubeFaceIndex(target)][level]);
}

void TextureCubeMap::subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
	Texture::subImageCompressed(xoffset, yoffset, 0, width, height, 1, format, imageSize, pixels, image[CubeFaceIndex(target)][level]);
}

// Tests for cube map sampling completeness. [OpenGL ES 3.0.5] section 3.8.13 page 161.
bool TextureCubeMap::isSamplerComplete() const
{
	for(int face = 0; face < 6; face++)
	{
		if(!image[face][mBaseLevel])
		{
			return false;
		}
	}

	int size = image[0][mBaseLevel]->getWidth();

	if(size <= 0)
	{
		return false;
	}

	if(!isMipmapFiltered())
	{
		if(!isCubeComplete())
		{
			return false;
		}
	}
	else
	{
		if(!isMipmapCubeComplete())   // Also tests for isCubeComplete()
		{
			return false;
		}
	}

	return true;
}

// Tests for cube texture completeness. [OpenGL ES 3.0.5] section 3.8.13 page 160.
bool TextureCubeMap::isCubeComplete() const
{
	if(image[0][mBaseLevel]->getWidth() <= 0 || image[0][mBaseLevel]->getHeight() != image[0][mBaseLevel]->getWidth())
	{
		return false;
	}

	for(unsigned int face = 1; face < 6; face++)
	{
		if(image[face][mBaseLevel]->getWidth()  != image[0][mBaseLevel]->getWidth() ||
		   image[face][mBaseLevel]->getWidth()  != image[0][mBaseLevel]->getHeight() ||
		   image[face][mBaseLevel]->getFormat() != image[0][mBaseLevel]->getFormat())
		{
			return false;
		}
	}

	return true;
}

bool TextureCubeMap::isMipmapCubeComplete() const
{
	if(mBaseLevel > mMaxLevel)
	{
		return false;
	}

	if(!isCubeComplete())
	{
		return false;
	}

	GLsizei size = image[0][mBaseLevel]->getWidth();
	int p = log2(size) + mBaseLevel;
	int q = std::min(p, mMaxLevel);

	for(int face = 0; face < 6; face++)
	{
		for(int level = mBaseLevel + 1; level <= q; level++)
		{
			if(!image[face][level])
			{
				return false;
			}

			if(image[face][level]->getFormat() != image[0][mBaseLevel]->getFormat())
			{
				return false;
			}

			int i = level - mBaseLevel;

			if(image[face][level]->getWidth() != std::max(1, size >> i))
			{
				return false;
			}
		}
	}

	return true;
}

void TextureCubeMap::updateBorders(int level)
{
	egl::Image *posX = image[CubeFaceIndex(GL_TEXTURE_CUBE_MAP_POSITIVE_X)][level];
	egl::Image *negX = image[CubeFaceIndex(GL_TEXTURE_CUBE_MAP_NEGATIVE_X)][level];
	egl::Image *posY = image[CubeFaceIndex(GL_TEXTURE_CUBE_MAP_POSITIVE_Y)][level];
	egl::Image *negY = image[CubeFaceIndex(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y)][level];
	egl::Image *posZ = image[CubeFaceIndex(GL_TEXTURE_CUBE_MAP_POSITIVE_Z)][level];
	egl::Image *negZ = image[CubeFaceIndex(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)][level];

	if(!posX || !negX || !posY || !negY || !posZ || !negZ)
	{
		return;
	}

	if(posX->getBorder() == 0)   // Non-seamless cube map.
	{
		return;
	}

	if(!posX->hasDirtyContents() || !posY->hasDirtyContents() || !posZ->hasDirtyContents() || !negX->hasDirtyContents() || !negY->hasDirtyContents() || !negZ->hasDirtyContents())
	{
		return;
	}

	// Copy top / bottom first.
	posX->copyCubeEdge(sw::Surface::BOTTOM, negY, sw::Surface::RIGHT);
	posY->copyCubeEdge(sw::Surface::BOTTOM, posZ, sw::Surface::TOP);
	posZ->copyCubeEdge(sw::Surface::BOTTOM, negY, sw::Surface::TOP);
	negX->copyCubeEdge(sw::Surface::BOTTOM, negY, sw::Surface::LEFT);
	negY->copyCubeEdge(sw::Surface::BOTTOM, negZ, sw::Surface::BOTTOM);
	negZ->copyCubeEdge(sw::Surface::BOTTOM, negY, sw::Surface::BOTTOM);

	posX->copyCubeEdge(sw::Surface::TOP, posY, sw::Surface::RIGHT);
	posY->copyCubeEdge(sw::Surface::TOP, negZ, sw::Surface::TOP);
	posZ->copyCubeEdge(sw::Surface::TOP, posY, sw::Surface::BOTTOM);
	negX->copyCubeEdge(sw::Surface::TOP, posY, sw::Surface::LEFT);
	negY->copyCubeEdge(sw::Surface::TOP, posZ, sw::Surface::BOTTOM);
	negZ->copyCubeEdge(sw::Surface::TOP, posY, sw::Surface::TOP);

	// Copy left / right after top and bottom are done.
	// The corner colors will be computed assuming top / bottom are already set.
	posX->copyCubeEdge(sw::Surface::RIGHT, negZ, sw::Surface::LEFT);
	posY->copyCubeEdge(sw::Surface::RIGHT, posX, sw::Surface::TOP);
	posZ->copyCubeEdge(sw::Surface::RIGHT, posX, sw::Surface::LEFT);
	negX->copyCubeEdge(sw::Surface::RIGHT, posZ, sw::Surface::LEFT);
	negY->copyCubeEdge(sw::Surface::RIGHT, posX, sw::Surface::BOTTOM);
	negZ->copyCubeEdge(sw::Surface::RIGHT, negX, sw::Surface::LEFT);

	posX->copyCubeEdge(sw::Surface::LEFT, posZ, sw::Surface::RIGHT);
	posY->copyCubeEdge(sw::Surface::LEFT, negX, sw::Surface::TOP);
	posZ->copyCubeEdge(sw::Surface::LEFT, negX, sw::Surface::RIGHT);
	negX->copyCubeEdge(sw::Surface::LEFT, negZ, sw::Surface::RIGHT);
	negY->copyCubeEdge(sw::Surface::LEFT, negX, sw::Surface::BOTTOM);
	negZ->copyCubeEdge(sw::Surface::LEFT, posX, sw::Surface::RIGHT);

	posX->markContentsClean();
	posY->markContentsClean();
	posZ->markContentsClean();
	negX->markContentsClean();
	negY->markContentsClean();
	negZ->markContentsClean();
}

bool TextureCubeMap::isCompressed(GLenum target, GLint level) const
{
	return IsCompressed(getFormat(target, level), egl::getClientVersion());
}

bool TextureCubeMap::isDepth(GLenum target, GLint level) const
{
	return IsDepthTexture(getFormat(target, level));
}

void TextureCubeMap::releaseTexImage()
{
	UNREACHABLE(0);   // Cube maps cannot have an EGL surface bound as an image
}

void TextureCubeMap::setImage(GLenum target, GLint level, GLsizei width, GLsizei height, GLint internalformat, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels)
{
	int face = CubeFaceIndex(target);

	if(image[face][level])
	{
		image[face][level]->release();
	}

	int border = (egl::getClientVersion() >= 3) ? 1 : 0;
	image[face][level] = egl::Image::create(this, width, height, 1, border, internalformat);

	if(!image[face][level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	Texture::setImage(format, type, unpackParameters, pixels, image[face][level]);
}

void TextureCubeMap::copyImage(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source)
{
	egl::Image *renderTarget = source->getRenderTarget();

	if(!renderTarget)
	{
		ERR("Failed to retrieve the render target.");
		return error(GL_OUT_OF_MEMORY);
	}

	int face = CubeFaceIndex(target);

	if(image[face][level])
	{
		image[face][level]->release();
	}

	int border = (egl::getClientVersion() >= 3) ? 1 : 0;
	image[face][level] = egl::Image::create(this, width, height, 1, border, internalformat);

	if(!image[face][level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	if(width != 0 && height != 0)
	{
		sw::SliceRect sourceRect(x, y, x + width, y + height, 0);
		sourceRect.clip(0, 0, renderTarget->getWidth(), renderTarget->getHeight());

		copy(renderTarget, sourceRect, 0, 0, 0, image[face][level]);
	}

	renderTarget->release();
}

egl::Image *TextureCubeMap::getImage(int face, unsigned int level)
{
	return image[face][level];
}

egl::Image *TextureCubeMap::getImage(GLenum face, unsigned int level)
{
	return image[CubeFaceIndex(face)][level];
}

void TextureCubeMap::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source)
{
	int face = CubeFaceIndex(target);

	if(!image[face][level])
	{
		return error(GL_INVALID_OPERATION);
	}

	GLsizei size = image[face][level]->getWidth();

	if(xoffset + width > size || yoffset + height > size || zoffset != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(width > 0 && height > 0)
	{
		egl::Image *renderTarget = source->getRenderTarget();

		if(!renderTarget)
		{
			ERR("Failed to retrieve the render target.");
			return error(GL_OUT_OF_MEMORY);
		}

		sw::SliceRect sourceRect(x, y, x + width, y + height, 0);
		sourceRect.clip(0, 0, renderTarget->getWidth(), renderTarget->getHeight());

		copy(renderTarget, sourceRect, xoffset, yoffset, zoffset, image[face][level]);

		renderTarget->release();
	}
}

void TextureCubeMap::generateMipmaps()
{
	if(!isCubeComplete())
	{
		return error(GL_INVALID_OPERATION);
	}

	int p = log2(image[0][mBaseLevel]->getWidth()) + mBaseLevel;
	int q = std::min(p, mMaxLevel);

	for(int f = 0; f < 6; f++)
	{
		ASSERT(image[f][mBaseLevel]);

		for(int i = mBaseLevel + 1; i <= q; i++)
		{
			if(image[f][i])
			{
				image[f][i]->release();
			}

			int border = (egl::getClientVersion() >= 3) ? 1 : 0;
			image[f][i] = egl::Image::create(this, std::max(image[f][mBaseLevel]->getWidth() >> i, 1), std::max(image[f][mBaseLevel]->getHeight() >> i, 1), 1, border, image[f][mBaseLevel]->getFormat());

			if(!image[f][i])
			{
				return error(GL_OUT_OF_MEMORY);
			}

			getDevice()->stretchRect(image[f][i - 1], 0, image[f][i], 0, Device::ALL_BUFFERS | Device::USE_FILTER);
		}
	}
}

Renderbuffer *TextureCubeMap::getRenderbuffer(GLenum target, GLint level)
{
	if(!IsCubemapTextureTarget(target))
	{
		return error(GL_INVALID_OPERATION, (Renderbuffer*)nullptr);
	}

	int face = CubeFaceIndex(target);

	if(!mFaceProxies[face])
	{
		mFaceProxies[face] = new Renderbuffer(name, new RenderbufferTextureCubeMap(this, target, level));
	}
	else
	{
		mFaceProxies[face]->setLevel(level);
	}

	return mFaceProxies[face];
}

egl::Image *TextureCubeMap::getRenderTarget(GLenum target, unsigned int level)
{
	ASSERT(IsCubemapTextureTarget(target));
	ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

	int face = CubeFaceIndex(target);

	if(image[face][level])
	{
		image[face][level]->addRef();
	}

	return image[face][level];
}

bool TextureCubeMap::isShared(GLenum target, unsigned int level) const
{
	ASSERT(IsCubemapTextureTarget(target));
	ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

	int face = CubeFaceIndex(target);

	if(!image[face][level])
	{
		return false;
	}

	return image[face][level]->isShared();
}

Texture3D::Texture3D(GLuint name) : Texture(name)
{
	for(int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
	{
		image[i] = nullptr;
	}

	mSurface = nullptr;

	mColorbufferProxy = nullptr;
	mProxyRefs = 0;
}

Texture3D::~Texture3D()
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
void Texture3D::addProxyRef(const Renderbuffer *proxy)
{
	mProxyRefs++;
}

void Texture3D::releaseProxy(const Renderbuffer *proxy)
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

void Texture3D::sweep()
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

GLenum Texture3D::getTarget() const
{
	return GL_TEXTURE_3D_OES;
}

GLsizei Texture3D::getWidth(GLenum target, GLint level) const
{
	ASSERT(target == getTarget());
	return image[level] ? image[level]->getWidth() : 0;
}

GLsizei Texture3D::getHeight(GLenum target, GLint level) const
{
	ASSERT(target == getTarget());
	return image[level] ? image[level]->getHeight() : 0;
}

GLsizei Texture3D::getDepth(GLenum target, GLint level) const
{
	ASSERT(target == getTarget());
	return image[level] ? image[level]->getDepth() : 0;
}

GLint Texture3D::getFormat(GLenum target, GLint level) const
{
	ASSERT(target == getTarget());
	return image[level] ? image[level]->getFormat() : GL_NONE;
}

int Texture3D::getTopLevel() const
{
	ASSERT(isSamplerComplete());
	int level = mBaseLevel;

	while(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && image[level])
	{
		level++;
	}

	return level - 1;
}

void Texture3D::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLint internalformat, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels)
{
	if(image[level])
	{
		image[level]->release();
	}

	image[level] = egl::Image::create(this, width, height, depth, 0, internalformat);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	Texture::setImage(format, type, unpackParameters, pixels, image[level]);
}

void Texture3D::releaseTexImage()
{
	UNREACHABLE(0);   // 3D textures cannot have an EGL surface bound as an image
}

void Texture3D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
	if(image[level])
	{
		image[level]->release();
	}

	image[level] = egl::Image::create(this, width, height, depth, 0, format);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	Texture::setCompressedImage(imageSize, pixels, image[level]);
}

void Texture3D::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels)
{
	Texture::subImage(xoffset, yoffset, zoffset, width, height, depth, format, type, unpackParameters, pixels, image[level]);
}

void Texture3D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels)
{
	Texture::subImageCompressed(xoffset, yoffset, zoffset, width, height, depth, format, imageSize, pixels, image[level]);
}

void Texture3D::copyImage(GLint level, GLenum internalformat, GLint x, GLint y, GLint z, GLsizei width, GLsizei height, GLsizei depth, Renderbuffer *source)
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

	image[level] = egl::Image::create(this, width, height, depth, 0, internalformat);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	if(width != 0 && height != 0 && depth != 0)
	{
		sw::SliceRect sourceRect(x, y, x + width, y + height, z);
		sourceRect.clip(0, 0, renderTarget->getWidth(), renderTarget->getHeight());

		for(GLint sliceZ = 0; sliceZ < depth; sliceZ++, sourceRect.slice++)
		{
			copy(renderTarget, sourceRect, 0, 0, sliceZ, image[level]);
		}
	}

	renderTarget->release();
}

void Texture3D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source)
{
	if(!image[level])
	{
		return error(GL_INVALID_OPERATION);
	}

	if(xoffset + width > image[level]->getWidth() || yoffset + height > image[level]->getHeight() || zoffset >= image[level]->getDepth())
	{
		return error(GL_INVALID_VALUE);
	}

	if(width > 0 && height > 0)
	{
		egl::Image *renderTarget = source->getRenderTarget();

		if(!renderTarget)
		{
			ERR("Failed to retrieve the render target.");
			return error(GL_OUT_OF_MEMORY);
		}

		sw::SliceRect sourceRect = {x, y, x + width, y + height, 0};
		sourceRect.clip(0, 0, renderTarget->getWidth(), renderTarget->getHeight());

		copy(renderTarget, sourceRect, xoffset, yoffset, zoffset, image[level]);

		renderTarget->release();
	}
}

void Texture3D::setSharedImage(egl::Image *sharedImage)
{
	sharedImage->addRef();

	if(image[0])
	{
		image[0]->release();
	}

	image[0] = sharedImage;
}

// Tests for 3D texture sampling completeness. [OpenGL ES 3.0.5] section 3.8.13 page 160.
bool Texture3D::isSamplerComplete() const
{
	if(!image[mBaseLevel])
	{
		return false;
	}

	GLsizei width = image[mBaseLevel]->getWidth();
	GLsizei height = image[mBaseLevel]->getHeight();
	GLsizei depth = image[mBaseLevel]->getDepth();

	if(width <= 0 || height <= 0 || depth <= 0)
	{
		return false;
	}

	if(isMipmapFiltered())
	{
		if(!isMipmapComplete())
		{
			return false;
		}
	}

	return true;
}

// Tests for 3D texture (mipmap) completeness. [OpenGL ES 3.0.5] section 3.8.13 page 160.
bool Texture3D::isMipmapComplete() const
{
	if(mBaseLevel > mMaxLevel)
	{
		return false;
	}

	GLsizei width = image[mBaseLevel]->getWidth();
	GLsizei height = image[mBaseLevel]->getHeight();
	GLsizei depth = image[mBaseLevel]->getDepth();
	bool isTexture2DArray = getTarget() == GL_TEXTURE_2D_ARRAY;

	int maxsize = isTexture2DArray ? std::max(width, height) : std::max(std::max(width, height), depth);
	int p = log2(maxsize) + mBaseLevel;
	int q = std::min(p, mMaxLevel);

	for(int level = mBaseLevel + 1; level <= q; level++)
	{
		if(!image[level])
		{
			return false;
		}

		if(image[level]->getFormat() != image[mBaseLevel]->getFormat())
		{
			return false;
		}

		int i = level - mBaseLevel;

		if(image[level]->getWidth() != std::max(1, width >> i))
		{
			return false;
		}

		if(image[level]->getHeight() != std::max(1, height >> i))
		{
			return false;
		}

		int levelDepth = isTexture2DArray ? depth : std::max(1, depth >> i);
		if(image[level]->getDepth() != levelDepth)
		{
			return false;
		}
	}

	return true;
}

bool Texture3D::isCompressed(GLenum target, GLint level) const
{
	return IsCompressed(getFormat(target, level), egl::getClientVersion());
}

bool Texture3D::isDepth(GLenum target, GLint level) const
{
	return IsDepthTexture(getFormat(target, level));
}

void Texture3D::generateMipmaps()
{
	if(!image[mBaseLevel])
	{
		return;   // Image unspecified. Not an error.
	}

	if(image[mBaseLevel]->getWidth() == 0 || image[mBaseLevel]->getHeight() == 0 || image[mBaseLevel]->getDepth() == 0)
	{
		return;   // Zero dimension. Not an error.
	}

	int maxsize = std::max(std::max(image[mBaseLevel]->getWidth(), image[mBaseLevel]->getHeight()), image[mBaseLevel]->getDepth());
	int p = log2(maxsize) + mBaseLevel;
	int q = std::min(p, mMaxLevel);

	for(int i = mBaseLevel + 1; i <= q; i++)
	{
		if(image[i])
		{
			image[i]->release();
		}

		image[i] = egl::Image::create(this, std::max(image[mBaseLevel]->getWidth() >> i, 1), std::max(image[mBaseLevel]->getHeight() >> i, 1), std::max(image[mBaseLevel]->getDepth() >> i, 1), 0, image[mBaseLevel]->getFormat());

		if(!image[i])
		{
			return error(GL_OUT_OF_MEMORY);
		}

		getDevice()->stretchCube(image[i - 1], image[i]);
	}
}

egl::Image *Texture3D::getImage(unsigned int level)
{
	return image[level];
}

Renderbuffer *Texture3D::getRenderbuffer(GLenum target, GLint level)
{
	if(target != getTarget())
	{
		return error(GL_INVALID_OPERATION, (Renderbuffer*)nullptr);
	}

	if(!mColorbufferProxy)
	{
		mColorbufferProxy = new Renderbuffer(name, new RenderbufferTexture3D(this, level));
	}
	else
	{
		mColorbufferProxy->setLevel(level);
	}

	return mColorbufferProxy;
}

egl::Image *Texture3D::getRenderTarget(GLenum target, unsigned int level)
{
	ASSERT(target == getTarget());
	ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

	if(image[level])
	{
		image[level]->addRef();
	}

	return image[level];
}

bool Texture3D::isShared(GLenum target, unsigned int level) const
{
	ASSERT(target == getTarget());
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

Texture2DArray::Texture2DArray(GLuint name) : Texture3D(name)
{
}

Texture2DArray::~Texture2DArray()
{
}

GLenum Texture2DArray::getTarget() const
{
	return GL_TEXTURE_2D_ARRAY;
}

void Texture2DArray::generateMipmaps()
{
	if(!image[mBaseLevel])
	{
		return;   // Image unspecified. Not an error.
	}

	if(image[mBaseLevel]->getWidth() == 0 || image[mBaseLevel]->getHeight() == 0 || image[mBaseLevel]->getDepth() == 0)
	{
		return;   // Zero dimension. Not an error.
	}

	int depth = image[mBaseLevel]->getDepth();
	int maxsize = std::max(image[mBaseLevel]->getWidth(), image[mBaseLevel]->getHeight());
	int p = log2(maxsize) + mBaseLevel;
	int q = std::min(p, mMaxLevel);

	for(int i = mBaseLevel + 1; i <= q; i++)
	{
		if(image[i])
		{
			image[i]->release();
		}

		GLsizei w = std::max(image[mBaseLevel]->getWidth() >> i, 1);
		GLsizei h = std::max(image[mBaseLevel]->getHeight() >> i, 1);
		image[i] = egl::Image::create(this, w, h, depth, 0, image[mBaseLevel]->getFormat());

		if(!image[i])
		{
			return error(GL_OUT_OF_MEMORY);
		}

		GLsizei srcw = image[i - 1]->getWidth();
		GLsizei srch = image[i - 1]->getHeight();
		for(int z = 0; z < depth; ++z)
		{
			sw::SliceRectF srcRect(0.0f, 0.0f, static_cast<float>(srcw), static_cast<float>(srch), z);
			sw::SliceRect dstRect(0, 0, w, h, z);
			getDevice()->stretchRect(image[i - 1], &srcRect, image[i], &dstRect, Device::ALL_BUFFERS | Device::USE_FILTER);
		}
	}
}

TextureExternal::TextureExternal(GLuint name) : Texture2D(name)
{
	mMinFilter = GL_LINEAR;
	mMagFilter = GL_LINEAR;
	mWrapS = GL_CLAMP_TO_EDGE;
	mWrapT = GL_CLAMP_TO_EDGE;
	mWrapR = GL_CLAMP_TO_EDGE;
}

TextureExternal::~TextureExternal()
{
}

GLenum TextureExternal::getTarget() const
{
	return GL_TEXTURE_EXTERNAL_OES;
}

}

NO_SANITIZE_FUNCTION egl::Image *createBackBuffer(int width, int height, sw::Format format, int multiSampleDepth)
{
	if(width > es2::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE || height > es2::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE)
	{
		ERR("Invalid parameters: %dx%d", width, height);
		return nullptr;
	}

	GLenum internalformat = sw2es::ConvertBackBufferFormat(format);

	return egl::Image::create(width, height, internalformat, multiSampleDepth, false);
}

NO_SANITIZE_FUNCTION egl::Image *createBackBufferFromClientBuffer(const egl::ClientBuffer& clientBuffer)
{
	if(clientBuffer.getWidth() > es2::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE ||
	   clientBuffer.getHeight() > es2::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE)
	{
		ERR("Invalid parameters: %dx%d", clientBuffer.getWidth(), clientBuffer.getHeight());
		return nullptr;
	}

	return egl::Image::create(clientBuffer);
}

NO_SANITIZE_FUNCTION egl::Image *createDepthStencil(int width, int height, sw::Format format, int multiSampleDepth)
{
	if(width > es2::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE || height > es2::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE)
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
