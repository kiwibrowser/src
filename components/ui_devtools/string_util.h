// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_STRING_UTIL_H_
#define COMPONENTS_UI_DEVTOOLS_STRING_UTIL_H_

#include <memory>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"

namespace ui_devtools {

using String = std::string;

namespace protocol {

class Value;

class CustomStringBuilder {
  String s_;

 public:
  CustomStringBuilder() {}
  CustomStringBuilder(String& s) : s_(s) {}
  void reserveCapacity(std::size_t size) { s_.reserve(size); }
  void append(const String& s) { s_ += s; }
  void append(char c) { s_ += c; }
  void append(const char* data, size_t length) { s_.append(data, length); }
  String toString() { return s_; }
};

using StringBuilder = CustomStringBuilder;

class StringUtil {
 public:
  static String substring(const String& s, unsigned pos, unsigned len) {
    return s.substr(pos, len);
  }
  static String fromInteger(int number) { return base::IntToString(number); }
  static String fromDouble(double number) {
    return base::NumberToString(number);
  }
  static double toDouble(const char* s, size_t len, bool* ok) {
    double v = 0.0;
    *ok = base::StringToDouble(std::string(s, len), &v);
    return *ok ? v : 0.0;
  }
  static void builderAppend(StringBuilder& builder, const String& s) {
    builder.append(s);
  }
  static void builderAppend(StringBuilder& builder, char c) {
    builder.append(c);
  }
  static void builderAppend(StringBuilder& builder, const char* s, size_t len) {
    builder.append(s, len);
  }
  static void builderAppendQuotedString(StringBuilder& builder,
                                        const String& str);
  static void builderReserve(StringBuilder& builder, unsigned capacity) {
    builder.reserveCapacity(capacity);
  }
  static String builderToString(StringBuilder& builder) {
    return builder.toString();
  }
  static size_t find(const String& s, const char* needle) {
    return s.find(needle);
  }
  static size_t find(const String& s, const String& needle) {
    return s.find(needle);
  }
  static const size_t kNotFound = static_cast<size_t>(-1);
  static std::unique_ptr<Value> parseJSON(const String& string);
};

}  // namespace protocol
}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_STRING_UTIL_H_
