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

// VertexDataManager.h: Defines the VertexDataManager, a class that
// runs the Buffer translation process.

#ifndef LIBGLES_CM_VERTEXDATAMANAGER_H_
#define LIBGLES_CM_VERTEXDATAMANAGER_H_

#include "Context.h"
#include "Device.hpp"

#include <GLES/gl.h>

namespace es1
{

struct TranslatedAttribute
{
	sw::StreamType type;
	int count;
	bool normalized;

	unsigned int offset;
	unsigned int stride;   // 0 means not to advance the read pointer at all

	sw::Resource *vertexBuffer;
};

class VertexBuffer
{
public:
	VertexBuffer(unsigned int size);
	virtual ~VertexBuffer();

	void unmap();

	sw::Resource *getResource() const;

protected:
	sw::Resource *mVertexBuffer;
};

class ConstantVertexBuffer : public VertexBuffer
{
public:
	ConstantVertexBuffer(float x, float y, float z, float w);
	~ConstantVertexBuffer();
};

class StreamingVertexBuffer : public VertexBuffer
{
public:
	StreamingVertexBuffer(unsigned int size);
	~StreamingVertexBuffer();

	void *map(const VertexAttribute &attribute, unsigned int requiredSpace, unsigned int *streamOffset);
	void reserveRequiredSpace();
	void addRequiredSpace(unsigned int requiredSpace);

protected:
	unsigned int mBufferSize;
	unsigned int mWritePosition;
	unsigned int mRequiredSpace;
};

class VertexDataManager
{
public:
	VertexDataManager(Context *context);
	virtual ~VertexDataManager();

	void dirtyCurrentValue(int index) { mDirtyCurrentValue[index] = true; }

	GLenum prepareVertexData(GLint start, GLsizei count, TranslatedAttribute *outAttribs);

private:
	unsigned int writeAttributeData(StreamingVertexBuffer *vertexBuffer, GLint start, GLsizei count, const VertexAttribute &attribute);

	Context *const mContext;

	StreamingVertexBuffer *mStreamingBuffer;

	bool mDirtyCurrentValue[MAX_VERTEX_ATTRIBS];
	ConstantVertexBuffer *mCurrentValueBuffer[MAX_VERTEX_ATTRIBS];
};

}

#endif   // LIBGLES_CM_VERTEXDATAMANAGER_H_
