// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/bindings/string_resource.h"

#include "third_party/blink/renderer/platform/bindings/v8_binding.h"

namespace blink {

template <class StringClass>
struct StringTraits {
  static const StringClass& FromStringResource(StringResourceBase*);
  template <typename V8StringTrait>
  static StringClass FromV8String(v8::Local<v8::String>, int);
};

template <>
struct StringTraits<String> {
  static const String& FromStringResource(StringResourceBase* resource) {
    return resource->WebcoreString();
  }
  template <typename V8StringTrait>
  static String FromV8String(v8::Local<v8::String>, int);
};

template <>
struct StringTraits<AtomicString> {
  static const AtomicString& FromStringResource(StringResourceBase* resource) {
    return resource->GetAtomicString();
  }
  template <typename V8StringTrait>
  static AtomicString FromV8String(v8::Local<v8::String>, int);
};

struct V8StringTwoBytesTrait {
  typedef UChar CharType;
  ALWAYS_INLINE static void Write(v8::Local<v8::String> v8_string,
                                  CharType* buffer,
                                  int length) {
    v8_string->Write(reinterpret_cast<uint16_t*>(buffer), 0, length);
  }
};

struct V8StringOneByteTrait {
  typedef LChar CharType;
  ALWAYS_INLINE static void Write(v8::Local<v8::String> v8_string,
                                  CharType* buffer,
                                  int length) {
    v8_string->WriteOneByte(buffer, 0, length);
  }
};

template <typename V8StringTrait>
String StringTraits<String>::FromV8String(v8::Local<v8::String> v8_string,
                                          int length) {
  DCHECK(v8_string->Length() == length);
  typename V8StringTrait::CharType* buffer;
  String result = String::CreateUninitialized(length, buffer);
  V8StringTrait::Write(v8_string, buffer, length);
  return result;
}

template <typename V8StringTrait>
AtomicString StringTraits<AtomicString>::FromV8String(
    v8::Local<v8::String> v8_string,
    int length) {
  DCHECK(v8_string->Length() == length);
  static const int kInlineBufferSize =
      32 / sizeof(typename V8StringTrait::CharType);
  if (length <= kInlineBufferSize) {
    typename V8StringTrait::CharType inline_buffer[kInlineBufferSize];
    V8StringTrait::Write(v8_string, inline_buffer, length);
    return AtomicString(inline_buffer, length);
  }
  typename V8StringTrait::CharType* buffer;
  String string = String::CreateUninitialized(length, buffer);
  V8StringTrait::Write(v8_string, buffer, length);
  return AtomicString(string);
}

template <typename StringType>
StringType V8StringToWebCoreString(v8::Local<v8::String> v8_string,
                                   ExternalMode external) {
  {
    // This portion of this function is very hot in certain Dromeao benchmarks.
    v8::String::Encoding encoding;
    v8::String::ExternalStringResourceBase* resource =
        v8_string->GetExternalStringResourceBase(&encoding);
    if (LIKELY(!!resource)) {
      StringResourceBase* base;
      if (encoding == v8::String::ONE_BYTE_ENCODING)
        base = static_cast<StringResource8*>(resource);
      else
        base = static_cast<StringResource16*>(resource);
      return StringTraits<StringType>::FromStringResource(base);
    }
  }

  int length = v8_string->Length();
  if (UNLIKELY(!length))
    return StringType("");

  bool one_byte = v8_string->ContainsOnlyOneByte();
  StringType result(one_byte ? StringTraits<StringType>::template FromV8String<
                                   V8StringOneByteTrait>(v8_string, length)
                             : StringTraits<StringType>::template FromV8String<
                                   V8StringTwoBytesTrait>(v8_string, length));

  if (external != kExternalize || !v8_string->CanMakeExternal())
    return result;

  if (result.Is8Bit()) {
    StringResource8* string_resource = new StringResource8(result);
    if (UNLIKELY(!v8_string->MakeExternal(string_resource)))
      delete string_resource;
  } else {
    StringResource16* string_resource = new StringResource16(result);
    if (UNLIKELY(!v8_string->MakeExternal(string_resource)))
      delete string_resource;
  }
  return result;
}

// Explicitly instantiate the above template with the expected
// parameterizations, to ensure the compiler generates the code; otherwise link
// errors can result in GCC 4.4.
template String V8StringToWebCoreString<String>(v8::Local<v8::String>,
                                                ExternalMode);
template AtomicString V8StringToWebCoreString<AtomicString>(
    v8::Local<v8::String>,
    ExternalMode);

// Fast but non thread-safe version.
String Int32ToWebCoreStringFast(int value) {
  // Caching of small strings below is not thread safe: newly constructed
  // AtomicString are not safely published.
  DCHECK(IsMainThread());

  // Most numbers used are <= 100. Even if they aren't used there's very little
  // cost in using the space.
  const int kLowNumbers = 100;
  DEFINE_STATIC_LOCAL(Vector<AtomicString>, low_numbers, (kLowNumbers + 1));
  String web_core_string;
  if (0 <= value && value <= kLowNumbers) {
    web_core_string = low_numbers[value];
    if (!web_core_string) {
      AtomicString value_string = AtomicString::Number(value);
      low_numbers[value] = value_string;
      web_core_string = value_string;
    }
  } else {
    web_core_string = String::Number(value);
  }
  return web_core_string;
}

String Int32ToWebCoreString(int value) {
  // If we are on the main thread (this should always true for non-workers),
  // call the faster one.
  if (IsMainThread())
    return Int32ToWebCoreStringFast(value);
  return String::Number(value);
}

}  // namespace blink
