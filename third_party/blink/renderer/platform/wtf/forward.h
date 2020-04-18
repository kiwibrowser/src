/*
 *  Copyright (C) 2006, 2009, 2011 Apple Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_FORWARD_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_FORWARD_H_

#include <stddef.h>
#include "third_party/blink/renderer/platform/wtf/compiler.h"

template <typename T>
class scoped_refptr;

namespace WTF {

template <typename T>
class StringBuffer;
class PartitionAllocator;
template <typename T,
          size_t inlineCapacity = 0,
          typename Allocator = PartitionAllocator>
class Vector;

class ArrayBuffer;
class ArrayBufferView;
class ArrayPiece;
class AtomicString;
class BigInt64Array;
class BigUint64Array;
class CString;
class Float32Array;
class Float64Array;
class Int8Array;
class Int16Array;
class Int32Array;
class OrdinalNumber;
class String;
class StringBuilder;
class StringImpl;
class StringView;
class TextStream;
class Uint8Array;
class Uint8ClampedArray;
class Uint16Array;
class Uint32Array;

}  // namespace WTF

using WTF::Vector;

using WTF::ArrayBuffer;
using WTF::ArrayBufferView;
using WTF::ArrayPiece;
using WTF::AtomicString;
using WTF::BigInt64Array;
using WTF::BigUint64Array;
using WTF::CString;
using WTF::Float32Array;
using WTF::Float64Array;
using WTF::Int8Array;
using WTF::Int16Array;
using WTF::Int32Array;
using WTF::String;
using WTF::StringBuffer;
using WTF::StringBuilder;
using WTF::StringImpl;
using WTF::StringView;
using WTF::Uint8Array;
using WTF::Uint8ClampedArray;
using WTF::Uint16Array;
using WTF::Uint32Array;

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_FORWARD_H_
