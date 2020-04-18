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

// TransformFeedback.cpp: Implements the es2::TransformFeedback class

#include "TransformFeedback.h"

namespace es2
{

TransformFeedback::TransformFeedback(GLuint name) : NamedObject(name), mActive(false), mPaused(false), mVertexOffset(0)
{
	mGenericBuffer = nullptr;
}

TransformFeedback::~TransformFeedback()
{
	mGenericBuffer = nullptr;
	for(int i = 0; i < MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS; ++i)
	{
		mBuffer[i].set(nullptr);
	}
}

Buffer* TransformFeedback::getGenericBuffer() const
{
	return mGenericBuffer;
}

Buffer* TransformFeedback::getBuffer(GLuint index) const
{
	return mBuffer[index].get();
}

GLuint TransformFeedback::getGenericBufferName() const
{
	return mGenericBuffer.name();
}

GLuint TransformFeedback::getBufferName(GLuint index) const
{
	return mBuffer[index].get().name();
}

int TransformFeedback::getOffset(GLuint index) const
{
	return mBuffer[index].getOffset();
}

int TransformFeedback::getSize(GLuint index) const
{
	return mBuffer[index].getSize();
}

void TransformFeedback::addVertexOffset(int count)
{
	if(isActive() && !isPaused())
	{
		mVertexOffset += count;
	}
}

int TransformFeedback::vertexOffset() const
{
	return mVertexOffset;
}

bool TransformFeedback::isActive() const
{
	return mActive;
}

bool TransformFeedback::isPaused() const
{
	return mPaused;
}

GLenum TransformFeedback::primitiveMode() const
{
	return mPrimitiveMode;
}

void TransformFeedback::begin(GLenum primitiveMode)
{
	mActive = true; mPrimitiveMode = primitiveMode;
}

void TransformFeedback::end()
{
	mActive = false;
	mPaused = false;
	mVertexOffset = 0;
}

void TransformFeedback::setPaused(bool paused)
{
	mPaused = paused;
}

void TransformFeedback::setGenericBuffer(Buffer* buffer)
{
	mGenericBuffer = buffer;
}

void TransformFeedback::setBuffer(GLuint index, Buffer* buffer)
{
	mBuffer[index].set(buffer);
}

void TransformFeedback::setBuffer(GLuint index, Buffer* buffer, GLintptr offset, GLsizeiptr size)
{
	mBuffer[index].set(buffer, static_cast<int>(offset), static_cast<int>(size));
}

void TransformFeedback::detachBuffer(GLuint buffer)
{
	if(mGenericBuffer.name() == buffer)
	{
		mGenericBuffer = nullptr;
	}

	for(int i = 0; i < MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS; ++i)
	{
		if(mBuffer[i].get().name() == buffer)
		{
			mBuffer[i].set(nullptr);
		}
	}
}

}
