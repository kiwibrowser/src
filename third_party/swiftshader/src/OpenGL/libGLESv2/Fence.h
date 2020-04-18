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

// Fence.h: Defines the Fence class, which supports the GL_NV_fence extension.

#ifndef LIBGLESV2_FENCE_H_
#define LIBGLESV2_FENCE_H_

#include "common/Object.hpp"
#include <GLES2/gl2.h>

namespace es2
{

class Fence
{
public:
	Fence();
	virtual ~Fence();

	GLboolean isFence();
	void setFence(GLenum condition);
	GLboolean testFence();
	void finishFence();
	void getFenceiv(GLenum pname, GLint *params);

private:
	bool mQuery;
	GLenum mCondition;
	GLboolean mStatus;
};

class FenceSync : public gl::NamedObject
{
public:
	FenceSync(GLuint name, GLenum condition, GLbitfield flags);
	virtual ~FenceSync();

	GLenum clientWait(GLbitfield flags, GLuint64 timeout);
	void serverWait(GLbitfield flags, GLuint64 timeout);
	void getSynciv(GLenum pname, GLsizei *length, GLint *values);

	GLenum getCondition() const { return mCondition; }
	GLbitfield getFlags() const { return mFlags; }

private:
	GLenum mCondition;
	GLbitfield mFlags;
};

}

#endif   // LIBGLESV2_FENCE_H_
