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

// Buffer.h: Defines the Buffer class, representing storage of vertex and/or
// index data. Implements GL buffer objects and related functionality.
// [OpenGL ES 2.0.24] section 2.9 page 21.

#ifndef LIBGLES_CM_BUFFER_H_
#define LIBGLES_CM_BUFFER_H_

#include "common/Object.hpp"
#include "Common/Resource.hpp"

#include <GLES/gl.h>

#include <cstddef>
#include <vector>

namespace es1
{
class Buffer : public gl::NamedObject
{
public:
	explicit Buffer(GLuint name);

	virtual ~Buffer();

	void bufferData(const void *data, GLsizeiptr size, GLenum usage);
	void bufferSubData(const void *data, GLsizeiptr size, GLintptr offset);

	const void *data() { return mContents ? mContents->data() : 0; }
	size_t size() const { return mSize; }
	GLenum usage() const { return mUsage; }

	sw::Resource *getResource();

private:
	sw::Resource *mContents;
	size_t mSize;
	GLenum mUsage;
};

}

#endif   // LIBGLES_CM_BUFFER_H_
