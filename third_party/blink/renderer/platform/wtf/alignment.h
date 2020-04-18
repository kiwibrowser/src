/*
 *  Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_ALIGNMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_ALIGNMENT_H_

#include <stdint.h>
#include "build/build_config.h"

namespace WTF {

#if defined(COMPILER_GCC)
#define WTF_ALIGN_OF(type) __alignof__(type)
#define WTF_ALIGNED(variable_type, variable, n) \
  variable_type variable __attribute__((__aligned__(n)))
#elif defined(COMPILER_MSVC)
#define WTF_ALIGN_OF(type) __alignof(type)
#define WTF_ALIGNED(variable_type, variable, n) \
  __declspec(align(n)) variable_type variable
#else
#error WTF_ALIGN macros need alignment control.
#endif

template <uintptr_t mask>
inline bool IsAlignedTo(const void* pointer) {
  return !(reinterpret_cast<uintptr_t>(pointer) & mask);
}

}  // namespace WTF

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_ALIGNMENT_H_
