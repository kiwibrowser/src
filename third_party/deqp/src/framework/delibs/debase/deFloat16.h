#ifndef _DEFLOAT16_H
#define _DEFLOAT16_H
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
 * \brief 16-bit floating-point math.
 *//*--------------------------------------------------------------------*/

#include "deDefs.h"
#include "deMath.h"

DE_BEGIN_EXTERN_C

typedef		deUint16			deFloat16;

#if defined(DE_DEPRECATED_TYPES)
typedef		deFloat16			DEfloat16;
#endif

/*--------------------------------------------------------------------*//*!
 * \brief Convert 32-bit floating point number to 16 bit.
 * \param val32	Input value.
 * \return Converted 16-bit floating-point value.
 *//*--------------------------------------------------------------------*/
deFloat16	deFloat32To16				(float val32);
deFloat16	deFloat32To16Round			(float val32, deRoundingMode mode);
void		deFloat16_selfTest			(void);

/*--------------------------------------------------------------------*//*!
 * \brief Convert 16-bit floating point number to 32 bit.
 * \param val16	Input value.
 * \return Converted 32-bit floating-point value.
 *//*--------------------------------------------------------------------*/
float		deFloat16To32		(deFloat16 val16);

DE_END_EXTERN_C

#endif /* _DEFLOAT16_H */
