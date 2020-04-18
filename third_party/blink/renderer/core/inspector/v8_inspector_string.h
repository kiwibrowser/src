// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_V8_INSPECTOR_STRING_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_V8_INSPECTOR_STRING_H_

#include <memory>
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/decimal.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"
#include "third_party/blink/renderer/platform/wtf/text/string_to_number.h"
#include "third_party/blink/renderer/platform/wtf/text/string_view.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "v8/include/v8-inspector.h"

namespace blink {

// Note that passed string must outlive the resulting StringView. This implies
// it must not be a temporary object.
CORE_EXPORT v8_inspector::StringView ToV8InspectorStringView(const StringView&);
CORE_EXPORT std::unique_ptr<v8_inspector::StringBuffer>
ToV8InspectorStringBuffer(const StringView&);
CORE_EXPORT String ToCoreString(const v8_inspector::StringView&);
CORE_EXPORT String ToCoreString(std::unique_ptr<v8_inspector::StringBuffer>);

namespace protocol {

class Value;

using String = WTF::String;
using StringBuilder = WTF::StringBuilder;

class CORE_EXPORT StringUtil {
  STATIC_ONLY(StringUtil);

 public:
  static String substring(const String& s, unsigned pos, unsigned len) {
    return s.Substring(pos, len);
  }
  static String fromInteger(int number) { return String::Number(number); }
  static String fromDouble(double number) {
    return Decimal::FromDouble(number).ToString();
  }
  static double toDouble(const char* s, size_t len, bool* ok) {
    return WTF::CharactersToDouble(reinterpret_cast<const LChar*>(s), len, ok);
  }
  static size_t find(const String& s, const char* needle) {
    return s.Find(needle);
  }
  static size_t find(const String& s, const String& needle) {
    return s.Find(needle);
  }
  static const size_t kNotFound = WTF::kNotFound;
  static void builderAppend(StringBuilder& builder, const String& s) {
    builder.Append(s);
  }
  static void builderAppend(StringBuilder& builder, UChar c) {
    builder.Append(c);
  }
  static void builderAppend(StringBuilder& builder, const char* s, size_t len) {
    builder.Append(s, len);
  }
  static void builderAppendQuotedString(StringBuilder&, const String&);
  static void builderReserve(StringBuilder& builder, unsigned capacity) {
    builder.ReserveCapacity(capacity);
  }
  static String builderToString(StringBuilder& builder) {
    return builder.ToString();
  }
  static std::unique_ptr<protocol::Value> parseJSON(const String&);
};

}  // namespace protocol

}  // namespace blink

// TODO(dgozman): migrate core/inspector/protocol to wtf::HashMap.
namespace std {
template <>
struct hash<WTF::String> {
  std::size_t operator()(const WTF::String& string) const {
    return StringHash::GetHash(string);
  }
};
}  // namespace std

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_V8_INSPECTOR_STRING_H_
