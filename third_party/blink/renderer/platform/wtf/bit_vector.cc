/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/wtf/bit_vector.h"

#include <string.h>
#include <algorithm>
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/leak_annotations.h"

namespace WTF {

void BitVector::SetSlow(const BitVector& other) {
  uintptr_t new_bits_or_pointer;
  if (other.IsInline()) {
    new_bits_or_pointer = other.bits_or_pointer_;
  } else {
    OutOfLineBits* new_out_of_line_bits = OutOfLineBits::Create(other.size());
    memcpy(new_out_of_line_bits->Bits(), other.Bits(), ByteCount(other.size()));
    new_bits_or_pointer = bit_cast<uintptr_t>(new_out_of_line_bits) >> 1;
  }
  if (!IsInline())
    OutOfLineBits::Destroy(GetOutOfLineBits());
  bits_or_pointer_ = new_bits_or_pointer;
}

void BitVector::Resize(size_t num_bits) {
  if (num_bits <= MaxInlineBits()) {
    if (IsInline())
      return;

    OutOfLineBits* my_out_of_line_bits = GetOutOfLineBits();
    bits_or_pointer_ = MakeInlineBits(*my_out_of_line_bits->Bits());
    OutOfLineBits::Destroy(my_out_of_line_bits);
    return;
  }

  ResizeOutOfLine(num_bits);
}

void BitVector::ClearAll() {
  if (IsInline())
    bits_or_pointer_ = MakeInlineBits(0);
  else
    memset(GetOutOfLineBits()->Bits(), 0, ByteCount(size()));
}

BitVector::OutOfLineBits* BitVector::OutOfLineBits::Create(size_t num_bits) {
  // Because of the way BitVector stores the pointer, memory tools
  // will erroneously report a leak here.
  WTF_INTERNAL_LEAK_SANITIZER_DISABLED_SCOPE;
  num_bits = (num_bits + BitsInPointer() - 1) &
             ~(BitsInPointer() - static_cast<size_t>(1));
  size_t size =
      sizeof(OutOfLineBits) + sizeof(uintptr_t) * (num_bits / BitsInPointer());
  void* allocation = Partitions::BufferMalloc(
      size, WTF_HEAP_PROFILER_TYPE_NAME(OutOfLineBits));
  OutOfLineBits* result = new (NotNull, allocation) OutOfLineBits(num_bits);
  return result;
}

void BitVector::OutOfLineBits::Destroy(OutOfLineBits* out_of_line_bits) {
  Partitions::BufferFree(out_of_line_bits);
}

void BitVector::ResizeOutOfLine(size_t num_bits) {
  DCHECK_GT(num_bits, MaxInlineBits());
  OutOfLineBits* new_out_of_line_bits = OutOfLineBits::Create(num_bits);
  size_t new_num_words = new_out_of_line_bits->NumWords();
  if (IsInline()) {
    // Make sure that all of the bits are zero in case we do a no-op resize.
    *new_out_of_line_bits->Bits() =
        bits_or_pointer_ & ~(static_cast<uintptr_t>(1) << MaxInlineBits());
    memset(new_out_of_line_bits->Bits() + 1, 0,
           (new_num_words - 1) * sizeof(void*));
  } else {
    if (num_bits > size()) {
      size_t old_num_words = GetOutOfLineBits()->NumWords();
      memcpy(new_out_of_line_bits->Bits(), GetOutOfLineBits()->Bits(),
             old_num_words * sizeof(void*));
      memset(new_out_of_line_bits->Bits() + old_num_words, 0,
             (new_num_words - old_num_words) * sizeof(void*));
    } else {
      memcpy(new_out_of_line_bits->Bits(), GetOutOfLineBits()->Bits(),
             new_out_of_line_bits->NumWords() * sizeof(void*));
    }
    OutOfLineBits::Destroy(GetOutOfLineBits());
  }
  bits_or_pointer_ = bit_cast<uintptr_t>(new_out_of_line_bits) >> 1;
}

}  // namespace WTF
