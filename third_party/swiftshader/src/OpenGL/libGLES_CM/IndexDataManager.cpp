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

// IndexDataManager.cpp: Defines the IndexDataManager, a class that
// runs the Buffer translation process for index buffers.

#include "IndexDataManager.h"

#include "Buffer.h"
#include "common/debug.h"

#include <string.h>
#include <algorithm>

namespace
{
	enum { INITIAL_INDEX_BUFFER_SIZE = 4096 * sizeof(GLuint) };
}

namespace es1
{

IndexDataManager::IndexDataManager()
{
	mStreamingBuffer = new StreamingIndexBuffer(INITIAL_INDEX_BUFFER_SIZE);

	if(!mStreamingBuffer)
	{
		ERR("Failed to allocate the streaming index buffer.");
	}
}

IndexDataManager::~IndexDataManager()
{
	delete mStreamingBuffer;
}

void copyIndices(GLenum type, const void *input, GLsizei count, void *output)
{
	if(type == GL_UNSIGNED_BYTE)
	{
		memcpy(output, input, count * sizeof(GLubyte));
	}
	else if(type == GL_UNSIGNED_SHORT)
	{
		memcpy(output, input, count * sizeof(GLushort));
	}
	else UNREACHABLE(type);
}

template<class IndexType>
void computeRange(const IndexType *indices, GLsizei count, GLuint *minIndex, GLuint *maxIndex)
{
	*minIndex = indices[0];
	*maxIndex = indices[0];

	for(GLsizei i = 0; i < count; i++)
	{
		if(*minIndex > indices[i]) *minIndex = indices[i];
		if(*maxIndex < indices[i]) *maxIndex = indices[i];
	}
}

void computeRange(GLenum type, const void *indices, GLsizei count, GLuint *minIndex, GLuint *maxIndex)
{
	if(type == GL_UNSIGNED_BYTE)
	{
		computeRange(static_cast<const GLubyte*>(indices), count, minIndex, maxIndex);
	}
	else if(type == GL_UNSIGNED_SHORT)
	{
		computeRange(static_cast<const GLushort*>(indices), count, minIndex, maxIndex);
	}
	else UNREACHABLE(type);
}

GLenum IndexDataManager::prepareIndexData(GLenum type, GLsizei count, Buffer *buffer, const void *indices, TranslatedIndexData *translated)
{
	if(!mStreamingBuffer)
	{
		return GL_OUT_OF_MEMORY;
	}

	intptr_t offset = reinterpret_cast<intptr_t>(indices);

	if(buffer != NULL)
	{
		if(typeSize(type) * count + offset > static_cast<std::size_t>(buffer->size()))
		{
			return GL_INVALID_OPERATION;
		}

		indices = static_cast<const GLubyte*>(buffer->data()) + offset;
	}

	StreamingIndexBuffer *streamingBuffer = mStreamingBuffer;

	sw::Resource *staticBuffer = buffer ? buffer->getResource() : NULL;

	if(staticBuffer)
	{
		computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex);

		translated->indexBuffer = staticBuffer;
		translated->indexOffset = offset;
	}
	else
	{
		unsigned int streamOffset = 0;
		int convertCount = count;

		streamingBuffer->reserveSpace(convertCount * typeSize(type), type);
		void *output = streamingBuffer->map(typeSize(type) * convertCount, &streamOffset);

		if(output == NULL)
		{
			ERR("Failed to map index buffer.");
			return GL_OUT_OF_MEMORY;
		}

		copyIndices(type, staticBuffer ? buffer->data() : indices, convertCount, output);
		streamingBuffer->unmap();

		computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex);

		translated->indexBuffer = streamingBuffer->getResource();
		translated->indexOffset = streamOffset;
	}

	return GL_NO_ERROR;
}

std::size_t IndexDataManager::typeSize(GLenum type)
{
	switch(type)
	{
	case GL_UNSIGNED_SHORT: return sizeof(GLushort);
	case GL_UNSIGNED_BYTE:  return sizeof(GLubyte);
	default: UNREACHABLE(type); return sizeof(GLushort);
	}
}

StreamingIndexBuffer::StreamingIndexBuffer(unsigned int initialSize) : mIndexBuffer(nullptr), mBufferSize(initialSize)
{
	if(initialSize > 0)
	{
		mIndexBuffer = new sw::Resource(initialSize + 16);

		if(!mIndexBuffer)
		{
			ERR("Out of memory allocating an index buffer of size %u.", initialSize);
		}
	}

	mWritePosition = 0;
}

StreamingIndexBuffer::~StreamingIndexBuffer()
{
	if(mIndexBuffer)
	{
		mIndexBuffer->destruct();
	}
}

void *StreamingIndexBuffer::map(unsigned int requiredSpace, unsigned int *offset)
{
	void *mapPtr = NULL;

	if(mIndexBuffer)
	{
		mapPtr = (char*)mIndexBuffer->lock(sw::PUBLIC) + mWritePosition;

		if(!mapPtr)
		{
			ERR(" Lock failed");
			return NULL;
		}

		*offset = mWritePosition;
		mWritePosition += requiredSpace;
	}

	return mapPtr;
}

void StreamingIndexBuffer::unmap()
{
	if(mIndexBuffer)
	{
		mIndexBuffer->unlock();
	}
}

void StreamingIndexBuffer::reserveSpace(unsigned int requiredSpace, GLenum type)
{
	if(requiredSpace > mBufferSize)
	{
		if(mIndexBuffer)
		{
			mIndexBuffer->destruct();
			mIndexBuffer = 0;
		}

		mBufferSize = std::max(requiredSpace, 2 * mBufferSize);

		mIndexBuffer = new sw::Resource(mBufferSize + 16);

		if(!mIndexBuffer)
		{
			ERR("Out of memory allocating an index buffer of size %u.", mBufferSize);
		}

		mWritePosition = 0;
	}
	else if(mWritePosition + requiredSpace > mBufferSize)   // Recycle
	{
		if(mIndexBuffer)
		{
			mIndexBuffer->destruct();
			mIndexBuffer = new sw::Resource(mBufferSize + 16);
		}

		mWritePosition = 0;
	}
}

sw::Resource *StreamingIndexBuffer::getResource() const
{
	return mIndexBuffer;
}

}
