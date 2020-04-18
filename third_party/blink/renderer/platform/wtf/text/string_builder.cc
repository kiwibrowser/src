/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

#include <algorithm>
#include "third_party/blink/renderer/platform/wtf/dtoa.h"
#include "third_party/blink/renderer/platform/wtf/text/integer_to_string_conversion.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace WTF {

String StringBuilder::ToString() {
  if (!length_)
    return g_empty_string;
  if (string_.IsNull()) {
    if (is8_bit_)
      string_ = String(Characters8(), length_);
    else
      string_ = String(Characters16(), length_);
    ClearBuffer();
  }
  return string_;
}

AtomicString StringBuilder::ToAtomicString() {
  if (!length_)
    return g_empty_atom;
  if (string_.IsNull()) {
    if (is8_bit_)
      string_ = AtomicString(Characters8(), length_);
    else
      string_ = AtomicString(Characters16(), length_);
    ClearBuffer();
  }
  return AtomicString(string_);
}

String StringBuilder::Substring(unsigned start, unsigned length) const {
  if (start >= length_)
    return g_empty_string;
  if (!string_.IsNull())
    return string_.Substring(start, length);
  length = std::min(length, length_ - start);
  if (is8_bit_)
    return String(Characters8() + start, length);
  return String(Characters16() + start, length);
}

void StringBuilder::Swap(StringBuilder& builder) {
  std::swap(string_, builder.string_);
  std::swap(buffer_, builder.buffer_);
  std::swap(length_, builder.length_);
  std::swap(is8_bit_, builder.is8_bit_);
}

void StringBuilder::ClearBuffer() {
  if (is8_bit_)
    delete buffer8_;
  else
    delete buffer16_;
  buffer_ = nullptr;
}

void StringBuilder::Clear() {
  ClearBuffer();
  string_ = String();
  length_ = 0;
  is8_bit_ = true;
}

unsigned StringBuilder::Capacity() const {
  if (!HasBuffer())
    return 0;
  if (is8_bit_)
    return buffer8_->capacity();
  return buffer16_->capacity();
}

void StringBuilder::ReserveCapacity(unsigned new_capacity) {
  if (is8_bit_)
    EnsureBuffer8(new_capacity);
  else
    EnsureBuffer16(new_capacity);
}

void StringBuilder::Resize(unsigned new_size) {
  DCHECK_LE(new_size, length_);
  string_ = string_.Left(new_size);
  length_ = new_size;
  if (HasBuffer()) {
    if (is8_bit_)
      buffer8_->resize(new_size);
    else
      buffer16_->resize(new_size);
  }
}

void StringBuilder::CreateBuffer8(unsigned added_size) {
  DCHECK(!HasBuffer());
  DCHECK(is8_bit_);
  buffer8_ = new Buffer8;
  // createBuffer is called right before appending addedSize more bytes. We
  // want to ensure we have enough space to fit m_string plus the added
  // size.
  //
  // We also ensure that we have at least the initialBufferSize of extra space
  // for appending new bytes to avoid future mallocs for appending short
  // strings or single characters. This is a no-op if m_length == 0 since
  // initialBufferSize() is the same as the inline capacity of the vector.
  // This allows doing append(string); append('\0') without extra mallocs.
  buffer8_->ReserveInitialCapacity(length_ +
                                   std::max(added_size, InitialBufferSize()));
  length_ = 0;
  Append(string_);
  string_ = String();
}

void StringBuilder::CreateBuffer16(unsigned added_size) {
  DCHECK(is8_bit_ || !HasBuffer());
  Buffer8 buffer8;
  unsigned length = length_;
  if (buffer8_) {
    buffer8_->swap(buffer8);
    delete buffer8_;
  }
  buffer16_ = new Buffer16;
  // See createBuffer8's call to reserveInitialCapacity for why we do this.
  buffer16_->ReserveInitialCapacity(length_ +
                                    std::max(added_size, InitialBufferSize()));
  is8_bit_ = false;
  length_ = 0;
  if (!buffer8.IsEmpty()) {
    Append(buffer8.data(), length);
    return;
  }
  Append(string_);
  string_ = String();
}

void StringBuilder::Append(const UChar* characters, unsigned length) {
  if (!length)
    return;
  DCHECK(characters);

  // If there's only one char we use append(UChar) instead since it will
  // check for latin1 and avoid converting to 16bit if possible.
  if (length == 1) {
    Append(*characters);
    return;
  }

  EnsureBuffer16(length);
  buffer16_->Append(characters, length);
  length_ += length;
}

void StringBuilder::Append(const LChar* characters, unsigned length) {
  if (!length)
    return;
  DCHECK(characters);

  if (is8_bit_) {
    EnsureBuffer8(length);
    buffer8_->Append(characters, length);
    length_ += length;
    return;
  }

  EnsureBuffer16(length);
  buffer16_->Append(characters, length);
  length_ += length;
}

template <typename IntegerType>
static void AppendIntegerInternal(StringBuilder& builder, IntegerType input) {
  IntegerToStringConverter<IntegerType> converter(input);
  builder.Append(converter.Characters8(), converter.length());
}

void StringBuilder::AppendNumber(int number) {
  AppendIntegerInternal(*this, number);
}

void StringBuilder::AppendNumber(unsigned number) {
  AppendIntegerInternal(*this, number);
}

void StringBuilder::AppendNumber(long number) {
  AppendIntegerInternal(*this, number);
}

void StringBuilder::AppendNumber(unsigned long number) {
  AppendIntegerInternal(*this, number);
}

void StringBuilder::AppendNumber(long long number) {
  AppendIntegerInternal(*this, number);
}

void StringBuilder::AppendNumber(unsigned long long number) {
  AppendIntegerInternal(*this, number);
}

void StringBuilder::AppendNumber(double number, unsigned precision) {
  NumberToStringBuffer buffer;
  Append(NumberToFixedPrecisionString(number, precision, buffer));
}

void StringBuilder::erase(unsigned index) {
  if (index >= length_)
    return;

  if (is8_bit_) {
    EnsureBuffer8(0);
    buffer8_->EraseAt(index);
  } else {
    EnsureBuffer16(0);
    buffer16_->EraseAt(index);
  }
  --length_;
}

}  // namespace WTF
