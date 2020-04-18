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

// Texture.h: Defines the abstract Texture class and its concrete derived
// classes Texture2D and TextureCubeMap. Implements GL texture objects and
// related functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#ifndef LIBGLES_CM_TEXTURE_H_
#define LIBGLES_CM_TEXTURE_H_

#include "Renderbuffer.h"
#include "common/Object.hpp"
#include "utilities.h"
#include "libEGL/Texture.hpp"
#include "common/debug.h"

#include <GLES/gl.h>

#include <vector>

namespace gl { class Surface; }

namespace es1
{
class Framebuffer;

enum
{
	IMPLEMENTATION_MAX_TEXTURE_LEVELS = sw::MIPMAP_LEVELS,
	IMPLEMENTATION_MAX_TEXTURE_SIZE = 1 << (IMPLEMENTATION_MAX_TEXTURE_LEVELS - 1),
	IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE = 1 << (IMPLEMENTATION_MAX_TEXTURE_LEVELS - 1),
	IMPLEMENTATION_MAX_RENDERBUFFER_SIZE = sw::OUTLINE_RESOLUTION,
};

class Texture : public egl::Texture
{
public:
	explicit Texture(GLuint name);

	sw::Resource *getResource() const;

	virtual void addProxyRef(const Renderbuffer *proxy) = 0;
	virtual void releaseProxy(const Renderbuffer *proxy) = 0;

	virtual GLenum getTarget() const = 0;

	bool setMinFilter(GLenum filter);
	bool setMagFilter(GLenum filter);
	bool setWrapS(GLenum wrap);
	bool setWrapT(GLenum wrap);
	bool setMaxAnisotropy(GLfloat textureMaxAnisotropy);
	void setGenerateMipmap(GLboolean enable);
	void setCropRect(GLint u, GLint v, GLint w, GLint h);

	GLenum getMinFilter() const;
	GLenum getMagFilter() const;
	GLenum getWrapS() const;
	GLenum getWrapT() const;
	GLfloat getMaxAnisotropy() const;
	GLboolean getGenerateMipmap() const;
	GLint getCropRectU() const;
	GLint getCropRectV() const;
	GLint getCropRectW() const;
	GLint getCropRectH() const;

	virtual GLsizei getWidth(GLenum target, GLint level) const = 0;
	virtual GLsizei getHeight(GLenum target, GLint level) const = 0;
	virtual GLint getFormat(GLenum target, GLint level) const = 0;
	virtual int getTopLevel() const = 0;

	virtual bool isSamplerComplete() const = 0;
	virtual bool isCompressed(GLenum target, GLint level) const = 0;
	virtual bool isDepth(GLenum target, GLint level) const = 0;

	virtual Renderbuffer *getRenderbuffer(GLenum target) = 0;
	virtual egl::Image *getRenderTarget(GLenum target, unsigned int level) = 0;
	egl::Image *createSharedImage(GLenum target, unsigned int level);
	virtual bool isShared(GLenum target, unsigned int level) const = 0;

	virtual void generateMipmaps() = 0;
	virtual void autoGenerateMipmaps() = 0;

	virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source) = 0;

protected:
	~Texture() override;

	void setImage(GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, egl::Image *image);
	void subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, egl::Image *image);
	void setCompressedImage(GLsizei imageSize, const void *pixels, egl::Image *image);
	void subImageCompressed(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels, egl::Image *image);

	bool copy(egl::Image *source, const sw::Rect &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, egl::Image *dest);

	bool isMipmapFiltered() const;

	GLenum mMinFilter;
	GLenum mMagFilter;
	GLenum mWrapS;
	GLenum mWrapT;
	GLfloat mMaxAnisotropy;
	GLboolean generateMipmap;
	GLint cropRectU;
	GLint cropRectV;
	GLint cropRectW;
	GLint cropRectH;

	sw::Resource *resource;
};

class Texture2D : public Texture
{
public:
	explicit Texture2D(GLuint name);

	void addProxyRef(const Renderbuffer *proxy) override;
	void releaseProxy(const Renderbuffer *proxy) override;
	void sweep() override;

	GLenum getTarget() const override;

	GLsizei getWidth(GLenum target, GLint level) const override;
	GLsizei getHeight(GLenum target, GLint level) const override;
	GLint getFormat(GLenum target, GLint level) const override;
	int getTopLevel() const override;

	void setImage(GLint level, GLsizei width, GLsizei height, GLint internalformat, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
	void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
	void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
	void copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
	void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);

	void setSharedImage(egl::Image *image);

	bool isSamplerComplete() const override;
	bool isCompressed(GLenum target, GLint level) const override;
	bool isDepth(GLenum target, GLint level) const override;
	void bindTexImage(gl::Surface *surface);
	void releaseTexImage() override;

	void generateMipmaps() override;
	void autoGenerateMipmaps() override;

	Renderbuffer *getRenderbuffer(GLenum target) override;
	egl::Image *getRenderTarget(GLenum target, unsigned int level) override;
	bool isShared(GLenum target, unsigned int level) const override;

	egl::Image *getImage(unsigned int level);

protected:
	~Texture2D() override;

	bool isMipmapComplete() const;

	egl::Image *image[IMPLEMENTATION_MAX_TEXTURE_LEVELS];

	gl::Surface *mSurface;

	// A specific internal reference count is kept for colorbuffer proxy references,
	// because, as the renderbuffer acting as proxy will maintain a binding pointer
	// back to this texture, there would be a circular reference if we used a binding
	// pointer here. This reference count will cause the pointer to be set to null if
	// the count drops to zero, but will not cause deletion of the Renderbuffer.
	Renderbuffer *mColorbufferProxy;
	unsigned int mProxyRefs;
};

class TextureExternal : public Texture2D
{
public:
	explicit TextureExternal(GLuint name);

	GLenum getTarget() const override;

protected:
	~TextureExternal() override;
};
}

#endif   // LIBGLES_CM_TEXTURE_H_
