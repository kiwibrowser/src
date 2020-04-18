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

// IndexDataManager.h: Defines the IndexDataManager, a class that
// runs the Buffer translation process for index buffers.

#ifndef LIBGLESV2_INDEXDATAMANAGER_H_
#define LIBGLESV2_INDEXDATAMANAGER_H_

#include "Context.h"

#include <GLES2/gl2.h>

namespace es2
{

struct TranslatedIndexData
{
	TranslatedIndexData(unsigned int primitiveCount) : primitiveCount(primitiveCount) {}

	unsigned int minIndex;
	unsigned int maxIndex;
	unsigned int indexOffset;
	unsigned int primitiveCount;

	sw::Resource *indexBuffer;
};

class StreamingIndexBuffer
{
public:
	StreamingIndexBuffer(size_t initialSize);
	virtual ~StreamingIndexBuffer();

	void *map(size_t requiredSpace, size_t *offset);
	void unmap();
	void reserveSpace(size_t requiredSpace, GLenum type);

	sw::Resource *getResource() const;

private:
	sw::Resource *mIndexBuffer;
	size_t mBufferSize;
	size_t mWritePosition;
};

class IndexDataManager
{
public:
	IndexDataManager();
	virtual ~IndexDataManager();

	GLenum prepareIndexData(GLenum mode, GLenum type, GLuint start, GLuint end, GLsizei count, Buffer *arrayElementBuffer, const void *indices, TranslatedIndexData *translated, bool primitiveRestart);

	static std::size_t typeSize(GLenum type);

private:
	StreamingIndexBuffer *mStreamingBuffer;
};

}

#endif   // LIBGLESV2_INDEXDATAMANAGER_H_
