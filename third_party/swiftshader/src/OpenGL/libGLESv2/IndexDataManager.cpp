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

namespace es2
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
	else if(type == GL_UNSIGNED_INT)
	{
		memcpy(output, input, count * sizeof(GLuint));
	}
	else if(type == GL_UNSIGNED_SHORT)
	{
		memcpy(output, input, count * sizeof(GLushort));
	}
	else UNREACHABLE(type);
}

inline GLsizei getNumIndices(const std::vector<GLsizei>& restartIndices, size_t i, GLsizei count)
{
	return restartIndices.empty() ? count :
	       ((i == 0) ? restartIndices[0] : ((i == restartIndices.size()) ? (count - restartIndices[i - 1] - 1) : (restartIndices[i] - restartIndices[i - 1] - 1)));
}

void copyIndices(GLenum mode, GLenum type, const std::vector<GLsizei>& restartIndices, const void *input, GLsizei count, void* output)
{
	size_t bytesPerIndex = 0;
	const unsigned char* inPtr = static_cast<const unsigned char*>(input);
	unsigned char* outPtr = static_cast<unsigned char*>(output);
	switch(type)
	{
	case GL_UNSIGNED_BYTE:
		bytesPerIndex = sizeof(GLubyte);
		break;
	case GL_UNSIGNED_INT:
		bytesPerIndex = sizeof(GLuint);
		break;
	case GL_UNSIGNED_SHORT:
		bytesPerIndex = sizeof(GLushort);
		break;
	default:
		UNREACHABLE(type);
	}

	size_t numRestarts = restartIndices.size();
	switch(mode)
	{
	case GL_TRIANGLES:
	case GL_LINES:
	case GL_POINTS:
	{
		GLsizei verticesPerPrimitive = (mode == GL_TRIANGLES) ? 3 : ((mode == GL_LINES) ? 2 : 1);
		for(size_t i = 0; i <= numRestarts; ++i)
		{
			GLsizei numIndices = getNumIndices(restartIndices, i, count);
			size_t numBytes = (numIndices / verticesPerPrimitive) * verticesPerPrimitive * bytesPerIndex;
			if(numBytes > 0)
			{
				memcpy(outPtr, inPtr, numBytes);
				outPtr += numBytes;
			}
			inPtr += (numIndices + 1) * bytesPerIndex;
		}
	}
		break;
	case GL_TRIANGLE_FAN:
		for(size_t i = 0; i <= numRestarts; ++i)
		{
			GLsizei numIndices = getNumIndices(restartIndices, i, count);
			GLsizei numTriangles = (numIndices - 2);
			for(GLsizei tri = 0; tri < numTriangles; ++tri)
			{
				memcpy(outPtr, inPtr, bytesPerIndex);
				outPtr += bytesPerIndex;
				memcpy(outPtr, inPtr + ((tri + 1) * bytesPerIndex), bytesPerIndex + bytesPerIndex);
				outPtr += bytesPerIndex + bytesPerIndex;
			}
			inPtr += (numIndices + 1) * bytesPerIndex;
		}
		break;
	case GL_TRIANGLE_STRIP:
		for(size_t i = 0; i <= numRestarts; ++i)
		{
			GLsizei numIndices = getNumIndices(restartIndices, i, count);
			GLsizei numTriangles = (numIndices - 2);
			for(GLsizei tri = 0; tri < numTriangles; ++tri)
			{
				if(tri & 1) // Reverse odd triangles
				{
					memcpy(outPtr, inPtr + ((tri + 1) * bytesPerIndex), bytesPerIndex);
					outPtr += bytesPerIndex;
					memcpy(outPtr, inPtr + ((tri + 0) * bytesPerIndex), bytesPerIndex);
					outPtr += bytesPerIndex;
					memcpy(outPtr, inPtr + ((tri + 2) * bytesPerIndex), bytesPerIndex);
					outPtr += bytesPerIndex;
				}
				else
				{
					size_t numBytes = 3 * bytesPerIndex;
					memcpy(outPtr, inPtr + (tri * bytesPerIndex), numBytes);
					outPtr += numBytes;
				}
			}
			inPtr += (numIndices + 1) * bytesPerIndex;
		}
		break;
	case GL_LINE_LOOP:
		for(size_t i = 0; i <= numRestarts; ++i)
		{
			GLsizei numIndices = getNumIndices(restartIndices, i, count);
			if(numIndices >= 2)
			{
				GLsizei numLines = numIndices;
				memcpy(outPtr, inPtr + (numIndices - 1) * bytesPerIndex, bytesPerIndex); // Last vertex
				outPtr += bytesPerIndex;
				memcpy(outPtr, inPtr, bytesPerIndex); // First vertex
				outPtr += bytesPerIndex;
				size_t bytesPerLine = 2 * bytesPerIndex;
				for(GLsizei tri = 0; tri < (numLines - 1); ++tri)
				{
					memcpy(outPtr, inPtr + tri * bytesPerIndex, bytesPerLine);
					outPtr += bytesPerLine;
				}
			}
			inPtr += (numIndices + 1) * bytesPerIndex;
		}
		break;
	case GL_LINE_STRIP:
		for(size_t i = 0; i <= numRestarts; ++i)
		{
			GLsizei numIndices = getNumIndices(restartIndices, i, count);
			GLsizei numLines = numIndices - 1;
			size_t bytesPerLine = 2 * bytesPerIndex;
			for(GLsizei tri = 0; tri < numLines; ++tri)
			{
				memcpy(outPtr, inPtr + tri * bytesPerIndex, bytesPerLine);
				outPtr += bytesPerLine;
			}
			inPtr += (numIndices + 1) * bytesPerIndex;
		}
		break;
	default:
		UNREACHABLE(mode);
		break;
	}
}

template<class IndexType>
void computeRange(const IndexType *indices, GLsizei count, GLuint *minIndex, GLuint *maxIndex, std::vector<GLsizei>* restartIndices)
{
	*maxIndex = 0;
	*minIndex = MAX_ELEMENTS_INDICES;

	for(GLsizei i = 0; i < count; i++)
	{
		if(restartIndices && indices[i] == IndexType(-1))
		{
			restartIndices->push_back(i);
			continue;
		}
		if(*minIndex > indices[i]) *minIndex = indices[i];
		if(*maxIndex < indices[i]) *maxIndex = indices[i];
	}
}

void computeRange(GLenum type, const void *indices, GLsizei count, GLuint *minIndex, GLuint *maxIndex, std::vector<GLsizei>* restartIndices)
{
	if(type == GL_UNSIGNED_BYTE)
	{
		computeRange(static_cast<const GLubyte*>(indices), count, minIndex, maxIndex, restartIndices);
	}
	else if(type == GL_UNSIGNED_INT)
	{
		computeRange(static_cast<const GLuint*>(indices), count, minIndex, maxIndex, restartIndices);
	}
	else if(type == GL_UNSIGNED_SHORT)
	{
		computeRange(static_cast<const GLushort*>(indices), count, minIndex, maxIndex, restartIndices);
	}
	else UNREACHABLE(type);
}

int recomputePrimitiveCount(GLenum mode, GLsizei count, const std::vector<GLsizei>& restartIndices, unsigned int* primitiveCount)
{
	size_t numRestarts = restartIndices.size();
	*primitiveCount = 0;

	unsigned int countOffset = 0;
	unsigned int vertexPerPrimitive = 0;

	switch(mode)
	{
	case GL_TRIANGLES: // 3 vertex per primitive
		++vertexPerPrimitive;
	case GL_LINES: //  2 vertex per primitive
		vertexPerPrimitive += 2;
		for(size_t i = 0; i <= numRestarts; ++i)
		{
			unsigned int nbIndices = getNumIndices(restartIndices, i, count);
			*primitiveCount += nbIndices / vertexPerPrimitive;
		}
		return vertexPerPrimitive;
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLE_STRIP: // (N - 2) polygons, 3 vertex per primitive
		++vertexPerPrimitive;
		--countOffset;
	case GL_LINE_STRIP: // (N - 1) polygons, 2 vertex per primitive
		--countOffset;
	case GL_LINE_LOOP: // N polygons, 2 vertex per primitive
		vertexPerPrimitive += 2;
		for(size_t i = 0; i <= numRestarts; ++i)
		{
			unsigned int nbIndices = getNumIndices(restartIndices, i, count);
			*primitiveCount += (nbIndices >= vertexPerPrimitive) ? (nbIndices + countOffset) : 0;
		}
		return vertexPerPrimitive;
	case GL_POINTS:
		*primitiveCount = static_cast<unsigned int>(count - restartIndices.size());
		return 1;
	default:
		UNREACHABLE(mode);
		return -1;
	}
}

GLenum IndexDataManager::prepareIndexData(GLenum mode, GLenum type, GLuint start, GLuint end, GLsizei count, Buffer *buffer, const void *indices, TranslatedIndexData *translated, bool primitiveRestart)
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

	std::vector<GLsizei>* restartIndices = primitiveRestart ? new std::vector<GLsizei>() : nullptr;
	computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex, restartIndices);

	StreamingIndexBuffer *streamingBuffer = mStreamingBuffer;

	sw::Resource *staticBuffer = buffer ? buffer->getResource() : NULL;

	if(restartIndices)
	{
		int vertexPerPrimitive = recomputePrimitiveCount(mode, count, *restartIndices, &translated->primitiveCount);
		if(vertexPerPrimitive == -1)
		{
			delete restartIndices;
			return GL_INVALID_ENUM;
		}

		size_t streamOffset = 0;
		int convertCount = translated->primitiveCount * vertexPerPrimitive;

		streamingBuffer->reserveSpace(convertCount * typeSize(type), type);
		void *output = streamingBuffer->map(typeSize(type) * convertCount, &streamOffset);

		if(output == NULL)
		{
			delete restartIndices;
			ERR("Failed to map index buffer.");
			return GL_OUT_OF_MEMORY;
		}

		copyIndices(mode, type, *restartIndices, indices, count, output);
		streamingBuffer->unmap();

		translated->indexBuffer = streamingBuffer->getResource();
		translated->indexOffset = static_cast<unsigned int>(streamOffset);
		delete restartIndices;
	}
	else if(staticBuffer)
	{
		translated->indexBuffer = staticBuffer;
		translated->indexOffset = static_cast<unsigned int>(offset);
	}
	else
	{
		size_t streamOffset = 0;
		int convertCount = count;

		streamingBuffer->reserveSpace(convertCount * typeSize(type), type);
		void *output = streamingBuffer->map(typeSize(type) * convertCount, &streamOffset);

		if(output == NULL)
		{
			ERR("Failed to map index buffer.");
			return GL_OUT_OF_MEMORY;
		}

		copyIndices(type, indices, convertCount, output);
		streamingBuffer->unmap();

		translated->indexBuffer = streamingBuffer->getResource();
		translated->indexOffset = static_cast<unsigned int>(streamOffset);
	}

	if(translated->minIndex < start || translated->maxIndex > end)
	{
		ERR("glDrawRangeElements: out of range access. Range provided: [%d -> %d]. Range used: [%d -> %d].", start, end, translated->minIndex, translated->maxIndex);
	}

	return GL_NO_ERROR;
}

std::size_t IndexDataManager::typeSize(GLenum type)
{
	switch(type)
	{
	case GL_UNSIGNED_INT:   return sizeof(GLuint);
	case GL_UNSIGNED_SHORT: return sizeof(GLushort);
	case GL_UNSIGNED_BYTE:  return sizeof(GLubyte);
	default: UNREACHABLE(type); return sizeof(GLushort);
	}
}

StreamingIndexBuffer::StreamingIndexBuffer(size_t initialSize) : mIndexBuffer(NULL), mBufferSize(initialSize)
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

void *StreamingIndexBuffer::map(size_t requiredSpace, size_t *offset)
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

void StreamingIndexBuffer::reserveSpace(size_t requiredSpace, GLenum type)
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
