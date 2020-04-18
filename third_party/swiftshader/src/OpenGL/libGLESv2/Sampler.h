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

// Sampler.h: Defines the es2::Sampler class

#ifndef LIBGLESV2_SAMPLER_H_
#define LIBGLESV2_SAMPLER_H_

#include "common/Object.hpp"
#include "Renderer/Renderer.hpp"

#include <GLES2/gl2.h>

namespace es2
{

class Sampler : public gl::NamedObject
{
public:
	Sampler(GLuint name) : NamedObject(name)
	{
		mMinFilter = GL_NEAREST_MIPMAP_LINEAR;
		mMagFilter = GL_LINEAR;

		mWrapModeS = GL_REPEAT;
		mWrapModeT = GL_REPEAT;
		mWrapModeR = GL_REPEAT;

		mMinLod = -1000.0f;
		mMaxLod = 1000.0f;
		mCompareMode = GL_NONE;
		mCompareFunc = GL_LEQUAL;
		mMaxAnisotropy = 1.0f;
	}

	void setMinFilter(GLenum minFilter) { mMinFilter = minFilter; }
	void setMagFilter(GLenum magFilter) { mMagFilter = magFilter; }
	void setWrapS(GLenum wrapS) { mWrapModeS = wrapS; }
	void setWrapT(GLenum wrapT) { mWrapModeT = wrapT; }
	void setWrapR(GLenum wrapR) { mWrapModeR = wrapR; }
	void setMinLod(GLfloat minLod) { mMinLod = minLod; }
	void setMaxLod(GLfloat maxLod) { mMaxLod = maxLod; }
	void setCompareMode(GLenum compareMode) { mCompareMode = compareMode; }
	void setCompareFunc(GLenum compareFunc) { mCompareFunc = compareFunc; }
	void setMaxAnisotropy(GLfloat maxAnisotropy) { mMaxAnisotropy = maxAnisotropy; }

	GLenum getMinFilter() const { return mMinFilter; }
	GLenum getMagFilter() const { return mMagFilter; }
	GLenum getWrapS() const { return mWrapModeS; }
	GLenum getWrapT() const { return mWrapModeT; }
	GLenum getWrapR() const { return mWrapModeR; }
	GLfloat getMinLod() const { return mMinLod; }
	GLfloat getMaxLod() const { return mMaxLod; }
	GLenum getCompareMode() const { return mCompareMode; }
	GLenum getCompareFunc() const { return mCompareFunc; }
	GLfloat getMaxAnisotropy() const { return mMaxAnisotropy; }

private:
	GLenum mMinFilter;
	GLenum mMagFilter;

	GLenum mWrapModeS;
	GLenum mWrapModeT;
	GLenum mWrapModeR;

	GLfloat mMinLod;
	GLfloat mMaxLod;
	GLenum mCompareMode;
	GLenum mCompareFunc;
	GLfloat mMaxAnisotropy;
};

}

#endif // LIBGLESV2_SAMPLER_H_
