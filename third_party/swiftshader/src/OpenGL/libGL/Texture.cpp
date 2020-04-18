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
// functionality.

#include "Texture.h"

#include "main.h"
#include "mathutil.h"
#include "Framebuffer.h"
#include "Device.hpp"
#include "Display.h"
#include "common/debug.h"

#include <algorithm>

namespace gl
{

Texture::Texture(GLuint name) : NamedObject(name)
{
	mMinFilter = GL_NEAREST_MIPMAP_LINEAR;
	mMagFilter = GL_LINEAR;
	mWrapS = GL_REPEAT;
	mWrapT = GL_REPEAT;
	mMaxAnisotropy = 1.0f;
	mMaxLevel = 1000;

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
	case GL_NEAREST:
	case GL_LINEAR:
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
	case GL_LINEAR_MIPMAP_LINEAR:
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
	case GL_CLAMP:
	case GL_REPEAT:
	case GL_CLAMP_TO_EDGE:
	case GL_MIRRORED_REPEAT:
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
	case GL_CLAMP:
	case GL_REPEAT:
	case GL_CLAMP_TO_EDGE:
	case GL_MIRRORED_REPEAT:
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

bool Texture::setMaxLevel(int level)
{
	if(level < 0)
	{
		return false;
	}

	mMaxLevel = level;

	return true;
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

void Texture::setImage(GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, Image *image)
{
	if(pixels && image)
	{
		image->loadImageData(0, 0, 0, image->getWidth(), image->getHeight(), 1, format, type, unpackAlignment, pixels);
	}
}

void Texture::setCompressedImage(GLsizei imageSize, const void *pixels, Image *image)
{
	if(pixels && image && (imageSize > 0)) // imageSize's correlation to width and height is already validated with gl::ComputeCompressedSize() at the API level
	{
		image->loadCompressedData(0, 0, 0, image->getWidth(), image->getHeight(), 1, imageSize, pixels);
	}
}

void Texture::subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, Image *image)
{
	if(!image)
	{
		return error(GL_INVALID_OPERATION);
	}

	if(width + xoffset > image->getWidth() || height + yoffset > image->getHeight())
	{
		return error(GL_INVALID_VALUE);
	}

	if(IsCompressed(image->getFormat()))
	{
		return error(GL_INVALID_OPERATION);
	}

	if(format != image->getFormat())
	{
		return error(GL_INVALID_OPERATION);
	}

	if(pixels)
	{
		image->loadImageData(xoffset, yoffset, 0, width, height, 1, format, type, unpackAlignment, pixels);
	}
}

void Texture::subImageCompressed(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels, Image *image)
{
	if(!image)
	{
		return error(GL_INVALID_OPERATION);
	}

	if(width + xoffset > image->getWidth() || height + yoffset > image->getHeight())
	{
		return error(GL_INVALID_VALUE);
	}

	if(format != image->getFormat())
	{
		return error(GL_INVALID_OPERATION);
	}

	if(pixels && (imageSize > 0)) // imageSize's correlation to width and height is already validated with gl::ComputeCompressedSize() at the API level
	{
		image->loadCompressedData(xoffset, yoffset, 0, width, height, 1, imageSize, pixels);
	}
}

bool Texture::copy(Image *source, const sw::Rect &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, Image *dest)
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
		image[i] = 0;
	}

	mColorbufferProxy = nullptr;
	mProxyRefs = 0;
}

Texture2D::~Texture2D()
{
	for(int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
	{
		if(image[i])
		{
			image[i]->unbind();
			image[i] = 0;
		}
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

GLenum Texture2D::getTarget() const
{
	return GL_TEXTURE_2D;
}

GLsizei Texture2D::getWidth(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D || target == GL_PROXY_TEXTURE_2D);
	return image[level] ? image[level]->getWidth() : 0;
}

GLsizei Texture2D::getHeight(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D || target == GL_PROXY_TEXTURE_2D);
	return image[level] ? image[level]->getHeight() : 0;
}

GLenum Texture2D::getFormat(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D || target == GL_PROXY_TEXTURE_2D);
	return image[level] ? image[level]->getFormat() : GL_NONE;
}

GLenum Texture2D::getType(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D || target == GL_PROXY_TEXTURE_2D);
	return image[level] ? image[level]->getType() : GL_NONE;
}

sw::Format Texture2D::getInternalFormat(GLenum target, GLint level) const
{
	ASSERT(target == GL_TEXTURE_2D || target == GL_PROXY_TEXTURE_2D);
	return image[level] ? image[level]->getInternalFormat() : sw::FORMAT_NULL;
}

int Texture2D::getTopLevel() const
{
	ASSERT(isSamplerComplete());
	int levels = 0;

	while(levels < IMPLEMENTATION_MAX_TEXTURE_LEVELS && image[levels])
	{
		levels++;
	}

	return levels;
}

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
	if(image[level])
	{
		image[level]->unbind();
	}

	image[level] = new Image(this, width, height, format, type);

	if(!image[level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	Texture::setImage(format, type, unpackAlignment, pixels, image[level]);
}

void Texture2D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
	if(image[level])
	{
		image[level]->unbind();
	}

	image[level] = new Image(this, width, height, format, GL_UNSIGNED_BYTE);

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
	Image *renderTarget = source->getRenderTarget();

	if(!renderTarget)
	{
		ERR("Failed to retrieve the render target.");
		return error(GL_OUT_OF_MEMORY);
	}

	if(image[level])
	{
		image[level]->unbind();
	}

	image[level] = new Image(this, width, height, format, GL_UNSIGNED_BYTE);

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

	Image *renderTarget = source->getRenderTarget();

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

void Texture2D::setImage(Image *sharedImage)
{
	sharedImage->addRef();

	if(image[0])
	{
		image[0]->unbind();
	}

	image[0] = sharedImage;
}

// Tests for 2D texture sampling completeness.
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
		if(!isMipmapComplete())
		{
			return false;
		}
	}

	return true;
}

// Tests for 2D texture (mipmap) completeness.
bool Texture2D::isMipmapComplete() const
{
	GLsizei width = image[0]->getWidth();
	GLsizei height = image[0]->getHeight();

	int q = log2(std::max(width, height));

	for(int level = 1; level <= q && level <= mMaxLevel; level++)
	{
		if(!image[level])
		{
			return false;
		}

		if(image[level]->getFormat() != image[0]->getFormat())
		{
			return false;
		}

		if(image[level]->getType() != image[0]->getType())
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
			image[i]->unbind();
		}

		image[i] = new Image(this, std::max(image[0]->getWidth() >> i, 1), std::max(image[0]->getHeight() >> i, 1), image[0]->getFormat(), image[0]->getType());

		if(!image[i])
		{
			return error(GL_OUT_OF_MEMORY);
		}

		getDevice()->stretchRect(image[i - 1], 0, image[i], 0, true);
	}
}

Image *Texture2D::getImage(unsigned int level)
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

Image *Texture2D::getRenderTarget(GLenum target, unsigned int level)
{
	ASSERT(target == GL_TEXTURE_2D);
	ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

	if(image[level])
	{
		image[level]->addRef();
	}

	return image[level];
}

TextureCubeMap::TextureCubeMap(GLuint name) : Texture(name)
{
	for(int f = 0; f < 6; f++)
	{
		for(int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
		{
			image[f][i] = 0;
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
				image[f][i]->unbind();
				image[f][i] = 0;
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

GLenum TextureCubeMap::getFormat(GLenum target, GLint level) const
{
	int face = CubeFaceIndex(target);
	return image[face][level] ? image[face][level]->getFormat() : GL_NONE;
}

GLenum TextureCubeMap::getType(GLenum target, GLint level) const
{
	int face = CubeFaceIndex(target);
	return image[face][level] ? image[face][level]->getType() : GL_NONE;
}

sw::Format TextureCubeMap::getInternalFormat(GLenum target, GLint level) const
{
	int face = CubeFaceIndex(target);
	return image[face][level] ? image[face][level]->getInternalFormat() : sw::FORMAT_NULL;
}

int TextureCubeMap::getTopLevel() const
{
	ASSERT(isSamplerComplete());
	int levels = 0;

	while(levels < IMPLEMENTATION_MAX_TEXTURE_LEVELS && image[0][levels])
	{
		levels++;
	}

	return levels;
}

void TextureCubeMap::setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
	int face = CubeFaceIndex(target);

	if(image[face][level])
	{
		image[face][level]->unbind();
	}

	image[face][level] = new Image(this, width, height, format, GL_UNSIGNED_BYTE);

	if(!image[face][level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	Texture::setCompressedImage(imageSize, pixels, image[face][level]);
}

void TextureCubeMap::subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
	Texture::subImage(xoffset, yoffset, width, height, format, type, unpackAlignment, pixels, image[CubeFaceIndex(target)][level]);
}

void TextureCubeMap::subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
	Texture::subImageCompressed(xoffset, yoffset, width, height, format, imageSize, pixels, image[CubeFaceIndex(target)][level]);
}

// Tests for cube map sampling completeness.
bool TextureCubeMap::isSamplerComplete() const
{
	for(int face = 0; face < 6; face++)
	{
		if(!image[face][0])
		{
			return false;
		}
	}

	int size = image[0][0]->getWidth();

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

// Tests for cube texture completeness.
bool TextureCubeMap::isCubeComplete() const
{
	if(image[0][0]->getWidth() <= 0 || image[0][0]->getHeight() != image[0][0]->getWidth())
	{
		return false;
	}

	for(unsigned int face = 1; face < 6; face++)
	{
		if(image[face][0]->getWidth()  != image[0][0]->getWidth() ||
		   image[face][0]->getWidth()  != image[0][0]->getHeight() ||
		   image[face][0]->getFormat() != image[0][0]->getFormat() ||
		   image[face][0]->getType()   != image[0][0]->getType())
		{
			return false;
		}
	}

	return true;
}

bool TextureCubeMap::isMipmapCubeComplete() const
{
	if(!isCubeComplete())
	{
		return false;
	}

	GLsizei size = image[0][0]->getWidth();
	int q = log2(size);

	for(int face = 0; face < 6; face++)
	{
		for(int level = 1; level <= q; level++)
		{
			if(!image[face][level])
			{
				return false;
			}

			if(image[face][level]->getFormat() != image[0][0]->getFormat())
			{
				return false;
			}

			if(image[face][level]->getType() != image[0][0]->getType())
			{
				return false;
			}

			if(image[face][level]->getWidth() != std::max(1, size >> level))
			{
				return false;
			}
		}
	}

	return true;
}

bool TextureCubeMap::isCompressed(GLenum target, GLint level) const
{
	return IsCompressed(getFormat(target, level));
}

bool TextureCubeMap::isDepth(GLenum target, GLint level) const
{
	return IsDepthTexture(getFormat(target, level));
}

void TextureCubeMap::setImage(GLenum target, GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
	int face = CubeFaceIndex(target);

	if(image[face][level])
	{
		image[face][level]->unbind();
	}

	image[face][level] = new Image(this, width, height, format, type);

	if(!image[face][level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	Texture::setImage(format, type, unpackAlignment, pixels, image[face][level]);
}

void TextureCubeMap::copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
	Image *renderTarget = source->getRenderTarget();

	if(!renderTarget)
	{
		ERR("Failed to retrieve the render target.");
		return error(GL_OUT_OF_MEMORY);
	}

	int face = CubeFaceIndex(target);

	if(image[face][level])
	{
		image[face][level]->unbind();
	}

	image[face][level] = new Image(this, width, height, format, GL_UNSIGNED_BYTE);

	if(!image[face][level])
	{
		return error(GL_OUT_OF_MEMORY);
	}

	if(width != 0 && height != 0)
	{
		sw::Rect sourceRect = {x, y, x + width, y + height};
		sourceRect.clip(0, 0, source->getColorbuffer()->getWidth(), source->getColorbuffer()->getHeight());

		copy(renderTarget, sourceRect, format, 0, 0, image[face][level]);
	}

	renderTarget->release();
}

Image *TextureCubeMap::getImage(int face, unsigned int level)
{
	return image[face][level];
}

Image *TextureCubeMap::getImage(GLenum face, unsigned int level)
{
	return image[CubeFaceIndex(face)][level];
}

void TextureCubeMap::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
	int face = CubeFaceIndex(target);

	if(!image[face][level])
	{
		return error(GL_INVALID_OPERATION);
	}

	GLsizei size = image[face][level]->getWidth();

	if(xoffset + width > size || yoffset + height > size)
	{
		return error(GL_INVALID_VALUE);
	}

	Image *renderTarget = source->getRenderTarget();

	if(!renderTarget)
	{
		ERR("Failed to retrieve the render target.");
		return error(GL_OUT_OF_MEMORY);
	}

	sw::Rect sourceRect = {x, y, x + width, y + height};
	sourceRect.clip(0, 0, source->getColorbuffer()->getWidth(), source->getColorbuffer()->getHeight());

	copy(renderTarget, sourceRect, image[face][level]->getFormat(), xoffset, yoffset, image[face][level]);

	renderTarget->release();
}

void TextureCubeMap::generateMipmaps()
{
	if(!isCubeComplete())
	{
		return error(GL_INVALID_OPERATION);
	}

	unsigned int q = log2(image[0][0]->getWidth());

	for(unsigned int f = 0; f < 6; f++)
	{
		for(unsigned int i = 1; i <= q; i++)
		{
			if(image[f][i])
			{
				image[f][i]->unbind();
			}

			image[f][i] = new Image(this, std::max(image[0][0]->getWidth() >> i, 1), std::max(image[0][0]->getHeight() >> i, 1), image[0][0]->getFormat(), image[0][0]->getType());

			if(!image[f][i])
			{
				return error(GL_OUT_OF_MEMORY);
			}

			getDevice()->stretchRect(image[f][i - 1], 0, image[f][i], 0, true);
		}
	}
}

Renderbuffer *TextureCubeMap::getRenderbuffer(GLenum target)
{
	if(!IsCubemapTextureTarget(target))
	{
		return error(GL_INVALID_OPERATION, (Renderbuffer *)nullptr);
	}

	int face = CubeFaceIndex(target);

	if(!mFaceProxies[face])
	{
		mFaceProxies[face] = new Renderbuffer(name, new RenderbufferTextureCubeMap(this, target));
	}

	return mFaceProxies[face];
}

Image *TextureCubeMap::getRenderTarget(GLenum target, unsigned int level)
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

}
