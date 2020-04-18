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

#include "VertexArray.h"

namespace es2
{

VertexArray::VertexArray(GLuint name) : gl::NamedObject(name)
{
}

VertexArray::~VertexArray()
{
	for(size_t i = 0; i < MAX_VERTEX_ATTRIBS; i++)
	{
		mVertexAttributes[i].mBoundBuffer = nullptr;
	}
	mElementArrayBuffer = nullptr;
}

void VertexArray::detachBuffer(GLuint bufferName)
{
	for(size_t attribute = 0; attribute < MAX_VERTEX_ATTRIBS; attribute++)
	{
		if(mVertexAttributes[attribute].mBoundBuffer.name() == bufferName)
		{
			mVertexAttributes[attribute].mBoundBuffer = nullptr;
		}
	}

	if(mElementArrayBuffer.name() == bufferName)
	{
		mElementArrayBuffer = nullptr;
	}
}

const VertexAttribute& VertexArray::getVertexAttribute(size_t attributeIndex) const
{
	ASSERT(attributeIndex < MAX_VERTEX_ATTRIBS);
	return mVertexAttributes[attributeIndex];
}

void VertexArray::setVertexAttribDivisor(GLuint index, GLuint divisor)
{
	ASSERT(index < MAX_VERTEX_ATTRIBS);
	mVertexAttributes[index].mDivisor = divisor;
}

void VertexArray::enableAttribute(unsigned int attributeIndex, bool enabledState)
{
	ASSERT(attributeIndex < MAX_VERTEX_ATTRIBS);
	mVertexAttributes[attributeIndex].mArrayEnabled = enabledState;
}

void VertexArray::setAttributeState(unsigned int attributeIndex, Buffer *boundBuffer, GLint size, GLenum type,
                                    bool normalized, bool pureInteger, GLsizei stride, const void *pointer)
{
	ASSERT(attributeIndex < MAX_VERTEX_ATTRIBS);
	mVertexAttributes[attributeIndex].mBoundBuffer = boundBuffer;
	mVertexAttributes[attributeIndex].mSize = size;
	mVertexAttributes[attributeIndex].mType = type;
	mVertexAttributes[attributeIndex].mNormalized = normalized;
	mVertexAttributes[attributeIndex].mPureInteger = pureInteger;
	mVertexAttributes[attributeIndex].mStride = stride;
	mVertexAttributes[attributeIndex].mPointer = pointer;
}

void VertexArray::setElementArrayBuffer(Buffer *buffer)
{
	mElementArrayBuffer = buffer;
}

}
