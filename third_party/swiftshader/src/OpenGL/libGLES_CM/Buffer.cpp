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

// Buffer.cpp: Implements the Buffer class, representing storage of vertex and/or
// index data. Implements GL buffer objects and related functionality.
// [OpenGL ES 2.0.24] section 2.9 page 21.

#include "Buffer.h"

#include "main.h"
#include "VertexDataManager.h"
#include "IndexDataManager.h"

namespace es1
{

Buffer::Buffer(GLuint name) : NamedObject(name)
{
    mContents = 0;
    mSize = 0;
    mUsage = GL_DYNAMIC_DRAW;
}

Buffer::~Buffer()
{
    if(mContents)
	{
		mContents->destruct();
	}
}

void Buffer::bufferData(const void *data, GLsizeiptr size, GLenum usage)
{
	if(mContents)
	{
		mContents->destruct();
		mContents = 0;
	}

	mSize = size;
	mUsage = usage;

	if(size > 0)
	{
		const int padding = 1024;   // For SIMD processing of vertices
		mContents = new sw::Resource(size + padding);

		if(!mContents)
		{
			return error(GL_OUT_OF_MEMORY);
		}

		if(data)
		{
			memcpy((void*)mContents->data(), data, size);
		}
	}
}

void Buffer::bufferSubData(const void *data, GLsizeiptr size, GLintptr offset)
{
	if(mContents)
	{
		char *buffer = (char*)mContents->lock(sw::PUBLIC);
		memcpy(buffer + offset, data, size);
		mContents->unlock();
	}
}

sw::Resource *Buffer::getResource()
{
	return mContents;
}

}
