// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/public/cpp/json_sanitizer.h"

#if defined(OS_ANDROID)
#error Build json_sanitizer_android.cc instead of this file on Android.
#endif

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/json/json_writer.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "build/build_config.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"

namespace data_decoder {

namespace {

class OopJsonSanitizer : public JsonSanitizer {
 public:
  OopJsonSanitizer(service_manager::Connector* connector,
                   const std::string& unsafe_json,
                   const StringCallback& success_callback,
                   const StringCallback& error_callback);

 private:
  friend std::default_delete<OopJsonSanitizer>;
  ~OopJsonSanitizer() {}

  void OnParseSuccess(std::unique_ptr<base::Value> value);
  void OnParseError(const std::string& error);

  StringCallback success_callback_;
  StringCallback error_callback_;

  DISALLOW_COPY_AND_ASSIGN(OopJsonSanitizer);
};

OopJsonSanitizer::OopJsonSanitizer(service_manager::Connector* connector,
                                   const std::string& unsafe_json,
                                   const StringCallback& success_callback,
                                   const StringCallback& error_callback)
    : success_callback_(success_callback), error_callback_(error_callback) {
  SafeJsonParser::Parse(
      connector, unsafe_json,
      base::Bind(&OopJsonSanitizer::OnParseSuccess, base::Unretained(this)),
      base::Bind(&OopJsonSanitizer::OnParseError, base::Unretained(this)));
}

void OopJsonSanitizer::OnParseSuccess(std::unique_ptr<base::Value> value) {
  // Self-destruct at the end of this method.
  std::unique_ptr<OopJsonSanitizer> deleter(this);

  // A valid JSON document may only have a dictionary or list as its top-level
  // type, but the JSON parser also accepts other types, so we filter them out.
  base::Value::Type type = value->type();
  if (type != base::Value::Type::DICTIONARY &&
      type != base::Value::Type::LIST) {
    error_callback_.Run("Invalid top-level type");
    return;
  }

  std::string json;
  if (!base::JSONWriter::Write(*value, &json)) {
    error_callback_.Run("Encoding error");
    return;
  }

  success_callback_.Run(json);
}

void OopJsonSanitizer::OnParseError(const std::string& error) {
  error_callback_.Run("Parse error: " + error);
  delete this;
}

}  // namespace

// static
void JsonSanitizer::Sanitize(service_manager::Connector* connector,
                             const std::string& unsafe_json,
                             const StringCallback& success_callback,
                             const StringCallback& error_callback) {
  // OopJsonSanitizer destroys itself when it is finished.
  new OopJsonSanitizer(connector, unsafe_json, success_callback,
                       error_callback);
}

}  // namespace data_decoder
