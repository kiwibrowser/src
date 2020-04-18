/*-------------------------------------------------------------------------
 * drawElements Stream Library
 * ---------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Thread safe ringbuffer
 *//*--------------------------------------------------------------------*/
#include "deRingbuffer.h"

#include "deInt32.h"
#include "deMemory.h"
#include "deSemaphore.h"

#include <stdlib.h>
#include <stdio.h>

struct deRingbuffer_s
{
	deInt32			blockSize;
	deInt32			blockCount;
	deInt32*		blockUsage;
	deUint8*		buffer;

	deSemaphore		emptyCount;
	deSemaphore		fullCount;

	deInt32			outBlock;
	deInt32			outPos;

	deInt32			inBlock;
	deInt32			inPos;

	deBool			stopNotified;
	deBool			consumerStopping;
};

deRingbuffer* deRingbuffer_create (deInt32 blockSize, deInt32 blockCount)
{
	deRingbuffer* ringbuffer = (deRingbuffer*)deCalloc(sizeof(deRingbuffer));

	DE_ASSERT(ringbuffer);
	DE_ASSERT(blockCount > 0);
	DE_ASSERT(blockSize > 0);

	ringbuffer->blockSize	= blockSize;
	ringbuffer->blockCount	= blockCount;
	ringbuffer->buffer		= (deUint8*)deMalloc(sizeof(deUint8) * (size_t)blockSize * (size_t)blockCount);
	ringbuffer->blockUsage	= (deInt32*)deMalloc(sizeof(deUint32) * (size_t)blockCount);
	ringbuffer->emptyCount	= deSemaphore_create(ringbuffer->blockCount, DE_NULL);
	ringbuffer->fullCount	= deSemaphore_create(0, DE_NULL);

	if (!ringbuffer->buffer		||
		!ringbuffer->blockUsage	||
		!ringbuffer->emptyCount	||
		!ringbuffer->fullCount)
	{
		if (ringbuffer->emptyCount)
			deSemaphore_destroy(ringbuffer->emptyCount);
		if (ringbuffer->fullCount)
			deSemaphore_destroy(ringbuffer->fullCount);
		deFree(ringbuffer->buffer);
		deFree(ringbuffer->blockUsage);
		deFree(ringbuffer);
		return DE_NULL;
	}

	memset(ringbuffer->blockUsage, 0, sizeof(deInt32) * (size_t)blockCount);

	ringbuffer->outBlock	= 0;
	ringbuffer->outPos		= 0;

	ringbuffer->inBlock		= 0;
	ringbuffer->inPos		= 0;

	ringbuffer->stopNotified		= DE_FALSE;
	ringbuffer->consumerStopping	= DE_FALSE;

	return ringbuffer;
}

void deRingbuffer_stop (deRingbuffer* ringbuffer)
{
	/* Set notify to true and increment fullCount to let consumer continue */
	ringbuffer->stopNotified = DE_TRUE;
	deSemaphore_increment(ringbuffer->fullCount);
}

void deRingbuffer_destroy (deRingbuffer* ringbuffer)
{
	deSemaphore_destroy(ringbuffer->emptyCount);
	deSemaphore_destroy(ringbuffer->fullCount);

	free(ringbuffer->buffer);
	free(ringbuffer->blockUsage);
	free(ringbuffer);
}

static deStreamResult producerStream_write (deStreamData* stream, const void* buf, deInt32 bufSize, deInt32* written)
{
	deRingbuffer* ringbuffer = (deRingbuffer*)stream;

	DE_ASSERT(stream);
	/* If ringbuffer is stopping return error on write */
	if (ringbuffer->stopNotified)
	{
		DE_ASSERT(DE_FALSE);
		return DE_STREAMRESULT_ERROR;
	}

	*written = 0;

	/* Write while more data available */
	while (*written < bufSize)
	{
		deInt32		writeSize	= 0;
		deUint8*	src			= DE_NULL;
		deUint8*	dst			= DE_NULL;

		/* If between blocks accuire new block */
		if (ringbuffer->inPos == 0)
		{
			deSemaphore_decrement(ringbuffer->emptyCount);
		}

		writeSize	= deMin32(ringbuffer->blockSize - ringbuffer->inPos, bufSize - *written);
		dst			= ringbuffer->buffer + ringbuffer->blockSize * ringbuffer->inBlock + ringbuffer->inPos;
		src			= (deUint8*)buf + *written;

		deMemcpy(dst, src, (size_t)writeSize);

		ringbuffer->inPos += writeSize;
		*written += writeSize;
		ringbuffer->blockUsage[ringbuffer->inBlock] += writeSize;

		/* Block is full move to next one (or "between" this and next block) */
		if (ringbuffer->inPos == ringbuffer->blockSize)
		{
			ringbuffer->inPos = 0;
			ringbuffer->inBlock++;

			if (ringbuffer->inBlock == ringbuffer->blockCount)
				ringbuffer->inBlock = 0;
			deSemaphore_increment(ringbuffer->fullCount);
		}
	}

	return DE_STREAMRESULT_SUCCESS;
}

static deStreamResult producerStream_flush (deStreamData* stream)
{
	deRingbuffer* ringbuffer = (deRingbuffer*)stream;

	DE_ASSERT(stream);

	/* No blocks reserved by producer */
	if (ringbuffer->inPos == 0)
		return DE_STREAMRESULT_SUCCESS;

	ringbuffer->inPos		= 0;
	ringbuffer->inBlock++;

	if (ringbuffer->inBlock == ringbuffer->blockCount)
		ringbuffer->inBlock = 0;

	deSemaphore_increment(ringbuffer->fullCount);
	return DE_STREAMRESULT_SUCCESS;
}

static deStreamResult producerStream_deinit (deStreamData* stream)
{
	DE_ASSERT(stream);

	producerStream_flush(stream);

	/* \note mika Stream doesn't own ringbuffer, so it's not deallocated */
	return DE_STREAMRESULT_SUCCESS;
}

static deStreamResult consumerStream_read (deStreamData* stream, void* buf, deInt32 bufSize, deInt32* read)
{
	deRingbuffer* ringbuffer = (deRingbuffer*)stream;

	DE_ASSERT(stream);

	*read = 0;
	DE_ASSERT(ringbuffer);

	while (*read < bufSize)
	{
		deInt32		writeSize	= 0;
		deUint8*	src			= DE_NULL;
		deUint8*	dst			= DE_NULL;

		/* If between blocks accuire new block */
		if (ringbuffer->outPos == 0)
		{
			/* If consumer is set to stop after everything is consumed,
			 * do not block if there is no more input left
			 */
			if (ringbuffer->consumerStopping)
			{
				/* Try to accuire new block, if can't there is no more input */
				if (!deSemaphore_tryDecrement(ringbuffer->fullCount))
				{
					return DE_STREAMRESULT_END_OF_STREAM;
				}
			}
			else
			{
				/* If not stopping block until there is more input */
				deSemaphore_decrement(ringbuffer->fullCount);
				/* Ringbuffer was set to stop */
				if (ringbuffer->stopNotified)
				{
					ringbuffer->consumerStopping = DE_TRUE;
				}
			}

		}

		writeSize	= deMin32(ringbuffer->blockUsage[ringbuffer->outBlock] - ringbuffer->outPos, bufSize - *read);
		src			= ringbuffer->buffer + ringbuffer->blockSize * ringbuffer->outBlock + ringbuffer->outPos;
		dst			= (deUint8*)buf + *read;

		deMemcpy(dst, src, (size_t)writeSize);

		ringbuffer->outPos += writeSize;
		*read += writeSize;

		/* Block is consumed move to next one (or "between" this and next block) */
		if (ringbuffer->outPos == ringbuffer->blockUsage[ringbuffer->outBlock])
		{
			ringbuffer->blockUsage[ringbuffer->outBlock] = 0;
			ringbuffer->outPos = 0;
			ringbuffer->outBlock++;

			if (ringbuffer->outBlock == ringbuffer->blockCount)
				ringbuffer->outBlock = 0;

			deSemaphore_increment(ringbuffer->emptyCount);
		}
	}

	return DE_STREAMRESULT_SUCCESS;
}


static deStreamResult consumerStream_deinit (deStreamData* stream)
{
	DE_ASSERT(stream);
	DE_UNREF(stream);

	return DE_STREAMRESULT_SUCCESS;
}

/* There are no sensible errors so status is always good */
deStreamStatus dummy_getStatus (deStreamData* stream)
{
	DE_UNREF(stream);

	return DE_STREAMSTATUS_GOOD;
}

/* There are no sensible errors in ringbuffer */
static const char* dummy_getError (deStreamData* stream)
{
	DE_ASSERT(stream);
	DE_UNREF(stream);
	return DE_NULL;
}

static const deIOStreamVFTable producerStreamVFTable = {
	DE_NULL,
	producerStream_write,
	dummy_getError,
	producerStream_flush,
	producerStream_deinit,
	dummy_getStatus
};

static const deIOStreamVFTable consumerStreamVFTable = {
	consumerStream_read,
	DE_NULL,
	dummy_getError,
	DE_NULL,
	consumerStream_deinit,
	dummy_getStatus
};

void deProducerStream_init (deOutStream* stream, deRingbuffer* buffer)
{
	stream->ioStream.streamData = (deStreamData*)buffer;
	stream->ioStream.vfTable = &producerStreamVFTable;
}

void deConsumerStream_init (deInStream* stream, deRingbuffer* buffer)
{
	stream->ioStream.streamData = (deStreamData*)buffer;
	stream->ioStream.vfTable = &consumerStreamVFTable;
}
