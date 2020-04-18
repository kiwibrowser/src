#ifndef _DEFILE_H
#define _DEFILE_H
/*-------------------------------------------------------------------------
 * drawElements Utility Library
 * ----------------------------
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
 * \brief File abstraction.
 *//*--------------------------------------------------------------------*/

#include "deDefs.h"

DE_BEGIN_EXTERN_C

/* File types. */
typedef struct deFile_s deFile;

typedef enum deFileMode_e
{
	DE_FILEMODE_READ		= (1<<0),	/*!< Read access to file.											*/
	DE_FILEMODE_WRITE		= (1<<2),	/*!< Write access to file.											*/
	DE_FILEMODE_CREATE		= (1<<3),	/*!< Create file if it doesn't exist. Requires DE_FILEMODE_WRITE.	*/
	DE_FILEMODE_OPEN		= (1<<4),	/*!< Open file if it exists.										*/
	DE_FILEMODE_TRUNCATE	= (1<<5)	/*!< Truncate content of file. Requires DE_FILEMODE_OPEN.			*/
} deFileMode;

typedef enum deFileFlag_e
{
	DE_FILE_NONBLOCKING		= (1<<0),	/*!< Set to non-blocking mode. Not supported on Win32!				*/
	DE_FILE_CLOSE_ON_EXEC	= (1<<1)
} deFileFlag;

typedef enum deFileResult_e
{
	DE_FILERESULT_SUCCESS		= 0,
	DE_FILERESULT_END_OF_FILE	= 1,
	DE_FILERESULT_WOULD_BLOCK	= 2,
	DE_FILERESULT_ERROR			= 3,

	DE_FILERESULT_LAST
} deFileResult;

typedef enum deFilePosition_e
{
	DE_FILEPOSITION_BEGIN		= 0,
	DE_FILEPOSITION_END			= 1,
	DE_FILEPOSITION_CURRENT		= 2,

	DE_FILEPOSITION_LAST
} deFilePosition;

/* File API. */

deBool			deFileExists			(const char* filename);
deBool			deDeleteFile			(const char* filename);

deFile*			deFile_create			(const char* filename, deUint32 mode);
deFile*			deFile_createFromHandle	(deUintptr handle);
void			deFile_destroy			(deFile* file);

deBool			deFile_setFlags			(deFile* file, deUint32 flags);

deInt64			deFile_getPosition		(const deFile* file);
deBool			deFile_seek				(deFile* file, deFilePosition base, deInt64 offset);
deInt64			deFile_getSize			(const deFile* file);

deFileResult	deFile_read				(deFile* file, void* buf, deInt64 bufSize, deInt64* numRead);
deFileResult	deFile_write			(deFile* file, const void* buf, deInt64 bufSize, deInt64* numWritten);

DE_END_EXTERN_C

#endif /* _DEFILE_H */
