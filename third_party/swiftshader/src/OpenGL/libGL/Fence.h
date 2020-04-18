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

#ifndef LIBGL_FENCE_H_
#define LIBGL_FENCE_H_

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

namespace gl
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

}

#endif   // LIBGL_FENCE_H_
