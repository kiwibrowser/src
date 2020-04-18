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

#ifndef LIBGLES_CM_INDEXDATAMANAGER_H_
#define LIBGLES_CM_INDEXDATAMANAGER_H_

#include "Context.h"

#include <GLES/gl.h>

namespace es1
{

struct TranslatedIndexData
{
	unsigned int minIndex;
	unsigned int maxIndex;
	unsigned int indexOffset;

	sw::Resource *indexBuffer;
};

class StreamingIndexBuffer
{
public:
	StreamingIndexBuffer(unsigned int initialSize);
	virtual ~StreamingIndexBuffer();

	void *map(unsigned int requiredSpace, unsigned int *offset);
	void unmap();
	void reserveSpace(unsigned int requiredSpace, GLenum type);

	sw::Resource *getResource() const;

private:
	sw::Resource *mIndexBuffer;
	unsigned int mBufferSize;
	unsigned int mWritePosition;
};

class IndexDataManager
{
public:
	IndexDataManager();
	virtual ~IndexDataManager();

	GLenum prepareIndexData(GLenum type, GLsizei count, Buffer *arrayElementBuffer, const void *indices, TranslatedIndexData *translated);

	static std::size_t typeSize(GLenum type);

private:
	StreamingIndexBuffer *mStreamingBuffer;
};

}

#endif   // LIBGLES_CM_INDEXDATAMANAGER_H_
