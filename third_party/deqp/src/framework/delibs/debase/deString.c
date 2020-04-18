/*-------------------------------------------------------------------------
 * drawElements Base Portability Library
 * -------------------------------------
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
 * \brief Basic string operations.
 *//*--------------------------------------------------------------------*/

#include "deString.h"

#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdarg.h>

DE_BEGIN_EXTERN_C

/*--------------------------------------------------------------------*//*!
 * \brief Compute hash from string.
 * \param str	String to compute hash value for.
 * \return Computed hash value.
 *//*--------------------------------------------------------------------*/
deUint32 deStringHash (const char* str)
{
	/* \note [pyry] This hash is used in DT_GNU_HASH and is proven
	to be robust for symbol hashing. */
	/* \see http://sources.redhat.com/ml/binutils/2006-06/msg00418.html */
	deUint32 hash = 5381;
	unsigned int c;

	DE_ASSERT(str);
	while ((c = (unsigned int)*str++) != 0)
		hash = (hash << 5) + hash + c;

	return hash;
}

deUint32 deStringHashLeading (const char* str, int numLeadingChars)
{
	deUint32 hash = 5381;
	unsigned int c;

	DE_ASSERT(str);
	while (numLeadingChars-- && (c = (unsigned int)*str++) != 0)
		hash = (hash << 5) + hash + c;

	return hash;
}

deUint32 deMemoryHash (const void* ptr, size_t numBytes)
{
	/* \todo [2010-05-10 pyry] Better generic hash function? */
	const deUint8*	input	= (const deUint8*)ptr;
	deUint32		hash	= 5381;

	DE_ASSERT(ptr);
	while (numBytes--)
		hash = (hash << 5) + hash + *input++;

	return hash;
}

deBool deMemoryEqual (const void* ptr, const void* cmp, size_t numBytes)
{
	return memcmp(ptr, cmp, numBytes) == 0;
}

/*--------------------------------------------------------------------*//*!
 * \brief Compare two strings for equality.
 * \param a		First string.
 * \param b		Second string.
 * \return True if strings equal, false otherwise.
 *//*--------------------------------------------------------------------*/
deBool deStringEqual (const char* a, const char* b)
{
	DE_ASSERT(a && b);
	return (strcmp(a, b) == 0);
}

deBool deStringBeginsWith (const char* str, const char* lead)
{
	const char* a = str;
	const char* b = lead;

	while (*b)
	{
		if (*a++ != *b++)
			return DE_FALSE;
	}

	return DE_TRUE;
}

int deVsprintf (char* string, size_t size, const char* format, va_list list)
{
	int			res;

	DE_ASSERT(string && format);

#if (DE_COMPILER == DE_COMPILER_MSC)
#	if (DE_OS == DE_OS_WINCE)
	res = _vsnprintf(string, size, format, list);
#	else
	res = vsnprintf_s(string, size, _TRUNCATE, format, list);
#	endif
#else
	res = vsnprintf(string, size, format, list);
#endif

	return res;
}

/*--------------------------------------------------------------------*//*!
 * \brief	Safe string print
 * \note	This has the new safe signature, i.e., string length is a
 *			required parameter.
 *//*--------------------------------------------------------------------*/
int deSprintf (char* string, size_t size, const char* format, ...)
{
	va_list		list;
	int			res;

	DE_ASSERT(string && format);

	va_start(list, format);

	res = deVsprintf(string, size, format, list);

	va_end(list);

	return res;
}

/*--------------------------------------------------------------------*//*!
 * \note	This has the new safe signature, i.e., string length is a
 *			required parameter.
 *//*--------------------------------------------------------------------*/
char* deStrcpy (char* dst, size_t size, const char* src)
{
#if (DE_COMPILER == DE_COMPILER_MSC) && (DE_OS != DE_OS_WINCE)
	(void)strcpy_s(dst, size, src);
	return dst;
#else
	return strncpy(dst, src, size);
#endif
}

/*--------------------------------------------------------------------*//*!
 * \note	This has the new safe signature, i.e., string length is a
 *			required parameter.
 *//*--------------------------------------------------------------------*/
char* deStrcat (char* s1, size_t size, const char* s2)
{
#if (DE_COMPILER == DE_COMPILER_MSC) && (DE_OS != DE_OS_WINCE)
	(void)strcat_s(s1, size, s2);
	return s1;
#else
	return strncat(s1, s2, size);
#endif
}

size_t deStrnlen (const char* string, size_t maxSize)
{
#if ((DE_COMPILER == DE_COMPILER_MSC) && (DE_OS != DE_OS_WINCE))
	return strnlen_s(string, maxSize);
#else
	size_t len = 0;
	while (len < maxSize && string[len] != 0)
		++len;
	return len;
#endif
}

DE_END_EXTERN_C
