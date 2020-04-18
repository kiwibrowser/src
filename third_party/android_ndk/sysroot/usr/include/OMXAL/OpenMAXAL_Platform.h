/*
 * Copyright (c) 2007-2010 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and/or associated documentation files (the
 * "Materials "), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 * OpenMAXAL_Platform.h - OpenMAX AL version 1.0.1
 *
 */

/****************************************************************************/
/* NOTE: This file contains definitions for the base types and the          */
/* XAAPIENTRY macro. This file **WILL NEED TO BE EDITED** to provide        */
/* the correct definitions specific to the platform being used.             */
/****************************************************************************/

#ifndef _OPENMAXAL_PLATFORM_H_
#define _OPENMAXAL_PLATFORM_H_

typedef unsigned char               xa_uint8_t;
typedef signed char                 xa_int8_t;
typedef unsigned short              xa_uint16_t;
typedef signed short                xa_int16_t;
typedef unsigned int /*long*/       xa_uint32_t;
typedef signed int /*long*/         xa_int32_t;
typedef long long                   xa_int64_t;
typedef unsigned long long          xa_uint64_t;

#ifndef XAAPIENTRY
#define XAAPIENTRY                 /* override per-platform */
#endif

/** The XA_API is a platform-specific macro used
 *  to declare OPENMAX AL function prototypes.  It is modified to meet the
 *  requirements for a particular platform
 *
 * Example:
 * #ifdef __SYMBIAN32__
 * #   define XA_API __declspec(dllimport)
 * #endif
 */

#ifndef XA_API
#ifdef __GNUC__
#define XA_API
#else
#define XA_API __declspec(dllimport)
#endif
#endif

#endif /* _OPENMAXAL_PLATFORM_H_ */
