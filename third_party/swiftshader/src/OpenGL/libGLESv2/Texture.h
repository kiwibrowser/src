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

#ifndef LIBGLESV2_TEXTURE_H_
#define LIBGLESV2_TEXTURE_H_

#include "Renderbuffer.h"
#include "common/Object.hpp"
#include "utilities.h"
#include "libEGL/Texture.hpp"
#include "common/debug.h"

#include <GLES2/gl2.h>

#include <vector>

namespace gl { class Surface; }

namespace es2
{
enum
{
	IMPLEMENTATION_MAX_TEXTURE_LEVELS = sw::MIPMAP_LEVELS,
	IMPLEMENTATION_MAX_TEXTURE_SIZE = 1 << (IMPLEMENTATION_MAX_TEXTURE_LEVELS - 1),
	IMPLEMENTATION_MAX_3D_TEXTURE_SIZE = IMPLEMENTATION_MAX_TEXTURE_SIZE,
	IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE = IMPLEMENTATION_MAX_TEXTURE_SIZE,
	IMPLEMENTATION_MAX_ARRAY_TEXTURE_LAYERS = IMPLEMENTATION_MAX_TEXTURE_SIZE,
	IMPLEMENTATION_MAX_RENDERBUFFER_SIZE = sw::OUTLINE_RESOLUTION,
};

class Texture : public egl::Texture
{
public:
	explicit Texture(GLuint name);

	sw::Resource *getResource() const override;

	virtual void addProxyRef(const Renderbuffer *proxy) = 0;
	virtual void releaseProxy(const Renderbuffer *proxy) = 0;

	virtual GLenum getTarget() const = 0;

	bool setMinFilter(GLenum filter);
	bool setMagFilter(GLenum filter);
	bool setWrapS(GLenum wrap);
	bool setWrapT(GLenum wrap);
	bool setWrapR(GLenum wrap);
	bool setMaxAnisotropy(GLfloat textureMaxAnisotropy);
	bool setBaseLevel(GLint baseLevel);
	bool setCompareFunc(GLenum compareFunc);
	bool setCompareMode(GLenum compareMode);
	void makeImmutable(GLsizei levels);
	bool setMaxLevel(GLint maxLevel);
	bool setMaxLOD(GLfloat maxLOD);
	bool setMinLOD(GLfloat minLOD);
	bool setSwizzleR(GLenum swizzleR);
	bool setSwizzleG(GLenum swizzleG);
	bool setSwizzleB(GLenum swizzleB);
	bool setSwizzleA(GLenum swizzleA);

	GLenum getMinFilter() const { return mMinFilter; }
	GLenum getMagFilter() const { return mMagFilter; }
	GLenum getWrapS() const { return mWrapS; }
	GLenum getWrapT() const { return mWrapT; }
	GLenum getWrapR() const { return mWrapR; }
	GLfloat getMaxAnisotropy() const { return mMaxAnisotropy; }
	GLint getBaseLevel() const { return mBaseLevel; }
	GLenum getCompareFunc() const { return mCompareFunc; }
	GLenum getCompareMode() const { return mCompareMode; }
	GLboolean getImmutableFormat() const { return mImmutableFormat; }
	GLsizei getImmutableLevels() const { return mImmutableLevels; }
	GLint getMaxLevel() const { return mMaxLevel; }
	GLfloat getMaxLOD() const { return mMaxLOD; }
	GLfloat getMinLOD() const { return mMinLOD; }
	GLenum getSwizzleR() const { return mSwizzleR; }
	GLenum getSwizzleG() const { return mSwizzleG; }
	GLenum getSwizzleB() const { return mSwizzleB; }
	GLenum getSwizzleA() const { return mSwizzleA; }

	virtual GLsizei getWidth(GLenum target, GLint level) const = 0;
	virtual GLsizei getHeight(GLenum target, GLint level) const = 0;
	virtual GLsizei getDepth(GLenum target, GLint level) const;
	virtual GLint getFormat(GLenum target, GLint level) const = 0;
	virtual int getTopLevel() const = 0;

	virtual bool isSamplerComplete() const = 0;
	virtual bool isCompressed(GLenum target, GLint level) const = 0;
	virtual bool isDepth(GLenum target, GLint level) const = 0;

	virtual Renderbuffer *getRenderbuffer(GLenum target, GLint level) = 0;
	virtual egl::Image *getRenderTarget(GLenum target, unsigned int level) = 0;
	egl::Image *createSharedImage(GLenum target, unsigned int level);
	virtual bool isShared(GLenum target, unsigned int level) const = 0;

	virtual void generateMipmaps() = 0;
	virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source) = 0;

protected:
	~Texture() override;

	void setImage(GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels, egl::Image *image);
	void subImage(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels, egl::Image *image);
	void setCompressedImage(GLsizei imageSize, const void *pixels, egl::Image *image);
	void subImageCompressed(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels, egl::Image *image);

	bool copy(egl::Image *source, const sw::SliceRect &sourceRect, GLint xoffset, GLint yoffset, GLint zoffset, egl::Image *dest);

	bool isMipmapFiltered() const;

	GLenum mMinFilter;
	GLenum mMagFilter;
	GLenum mWrapS;
	GLenum mWrapT;
	GLenum mWrapR;
	GLfloat mMaxAnisotropy;
	GLint mBaseLevel;
	GLenum mCompareFunc;
	GLenum mCompareMode;
	GLboolean mImmutableFormat;
	GLsizei mImmutableLevels;
	GLint mMaxLevel;
	GLfloat mMaxLOD;
	GLfloat mMinLOD;
	GLenum mSwizzleR;
	GLenum mSwizzleG;
	GLenum mSwizzleB;
	GLenum mSwizzleA;

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

	void setImage(GLint level, GLsizei width, GLsizei height, GLint internalformat, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels);
	void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels);
	void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
	void copyImage(GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source);
	void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source) override;

	void setSharedImage(egl::Image *image);

	bool isSamplerComplete() const override;
	bool isCompressed(GLenum target, GLint level) const override;
	bool isDepth(GLenum target, GLint level) const override;
	void bindTexImage(gl::Surface *surface);
	void releaseTexImage() override;

	void generateMipmaps() override;

	Renderbuffer *getRenderbuffer(GLenum target, GLint level) override;
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

class Texture2DRect : public Texture2D
{
public:
	explicit Texture2DRect(GLuint name);

	GLenum getTarget() const override;

	Renderbuffer *getRenderbuffer(GLenum target, GLint level) override;
};

class TextureCubeMap : public Texture
{
public:
	explicit TextureCubeMap(GLuint name);

	void addProxyRef(const Renderbuffer *proxy) override;
	void releaseProxy(const Renderbuffer *proxy) override;
	void sweep() override;

	GLenum getTarget() const override;

	GLsizei getWidth(GLenum target, GLint level) const override;
	GLsizei getHeight(GLenum target, GLint level) const override;
	GLint getFormat(GLenum target, GLint level) const override;
	int getTopLevel() const override;

	void setImage(GLenum target, GLint level, GLsizei width, GLsizei height, GLint internalformat, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels);
	void setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);

	void subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels);
	void subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
	void copyImage(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source);
	void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source) override;

	bool isSamplerComplete() const override;
	bool isCompressed(GLenum target, GLint level) const override;
	bool isDepth(GLenum target, GLint level) const override;
	void releaseTexImage() override;

	void generateMipmaps() override;
	void updateBorders(int level);

	Renderbuffer *getRenderbuffer(GLenum target, GLint level) override;
	egl::Image *getRenderTarget(GLenum target, unsigned int level) override;
	bool isShared(GLenum target, unsigned int level) const override;

	egl::Image *getImage(int face, unsigned int level);

	bool isCubeComplete() const;

protected:
	~TextureCubeMap() override;

private:
	bool isMipmapCubeComplete() const;

	// face is one of the GL_TEXTURE_CUBE_MAP_* enumerants. Returns nullptr on failure.
	egl::Image *getImage(GLenum face, unsigned int level);

	egl::Image *image[6][IMPLEMENTATION_MAX_TEXTURE_LEVELS];

	// A specific internal reference count is kept for colorbuffer proxy references,
	// because, as the renderbuffer acting as proxy will maintain a binding pointer
	// back to this texture, there would be a circular reference if we used a binding
	// pointer here. This reference count will cause the pointer to be set to null if
	// the count drops to zero, but will not cause deletion of the Renderbuffer.
	Renderbuffer *mFaceProxies[6];
	unsigned int mFaceProxyRefs[6];
};

class Texture3D : public Texture
{
public:
	explicit Texture3D(GLuint name);

	void addProxyRef(const Renderbuffer *proxy) override;
	void releaseProxy(const Renderbuffer *proxy) override;
	void sweep() override;

	GLenum getTarget() const override;

	GLsizei getWidth(GLenum target, GLint level) const override;
	GLsizei getHeight(GLenum target, GLint level) const override;
	GLsizei getDepth(GLenum target, GLint level) const override;
	GLint getFormat(GLenum target, GLint level) const override;
	int getTopLevel() const override;

	void setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLint internalformat, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels);
	void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels);
	void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelStorageModes &unpackParameters, const void *pixels);
	void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels);
	void copyImage(GLint level, GLenum internalformat, GLint x, GLint y, GLint z, GLsizei width, GLsizei height, GLsizei depth, Renderbuffer *source);
	void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source) override;

	void setSharedImage(egl::Image *image);

	bool isSamplerComplete() const override;
	bool isCompressed(GLenum target, GLint level) const override;
	bool isDepth(GLenum target, GLint level) const override;
	void releaseTexImage() override;

	void generateMipmaps() override;

	Renderbuffer *getRenderbuffer(GLenum target, GLint level) override;
	egl::Image *getRenderTarget(GLenum target, unsigned int level) override;
	bool isShared(GLenum target, unsigned int level) const override;

	egl::Image *getImage(unsigned int level);

protected:
	~Texture3D() override;

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

class Texture2DArray : public Texture3D
{
public:
	explicit Texture2DArray(GLuint name);

	GLenum getTarget() const override;
	void generateMipmaps() override;

protected:
	~Texture2DArray() override;
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

#endif   // LIBGLESV2_TEXTURE_H_
