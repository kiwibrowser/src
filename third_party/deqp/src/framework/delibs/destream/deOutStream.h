#ifndef _DEOUTSTREAM_H
#define _DEOUTSTREAM_H
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
 * \brief Output stream abstraction.
 *//*--------------------------------------------------------------------*/

#include "deDefs.h"

#include "deIOStream.h"

DE_BEGIN_EXTERN_C

/* Output stream struct, implemented as a wrapper to io stream */
typedef struct deOutStream_s
{
	deIOStream ioStream;
} deOutStream;

DE_INLINE deStreamResult	deOutStream_write		(deOutStream* stream, const void* buf, deInt32 bufSize, deInt32* numWritten);
DE_INLINE deStreamResult	deOutStream_flush		(deOutStream* stream);
DE_INLINE deStreamResult	deOutStream_deinit		(deOutStream* stream);
DE_INLINE const char*		deOutStream_getError	(deOutStream* stream);
DE_INLINE deStreamStatus	deOutStream_getStatus	(deOutStream* stream);


DE_INLINE deStreamResult deOutStream_write (deOutStream* stream, const void* buf, deInt32 bufSize, deInt32* numWritten)
{
	return deIOStream_write(&(stream->ioStream), buf, bufSize, numWritten);
}

DE_INLINE deStreamResult deOutStream_flush (deOutStream* stream)
{
	return deIOStream_flush(&(stream->ioStream));
}

DE_INLINE deStreamResult deOutStream_deinit (deOutStream* stream)
{
	return deIOStream_deinit(&(stream->ioStream));
}

DE_INLINE const char* deOutStream_getError (deOutStream* stream)
{
	return deIOStream_getError(&(stream->ioStream));
}

DE_INLINE deStreamStatus deOutStream_getStatus (deOutStream* stream)
{
	return deIOStream_getStatus(&(stream->ioStream));
}

DE_END_EXTERN_C

#endif /* _DEOUTSTREAM_H */
