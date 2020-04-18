// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/public/cpp/safe_json_parser_android.h"

#include <utility>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "services/data_decoder/public/cpp/json_sanitizer.h"

namespace data_decoder {

SafeJsonParserAndroid::SafeJsonParserAndroid(
    const std::string& unsafe_json,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback)
    : unsafe_json_(unsafe_json),
      success_callback_(success_callback),
      error_callback_(error_callback) {}

SafeJsonParserAndroid::~SafeJsonParserAndroid() {}

void SafeJsonParserAndroid::Start() {
  JsonSanitizer::Sanitize(
      /*connector=*/nullptr,  // connector is unused on Android.
      unsafe_json_,
      base::Bind(&SafeJsonParserAndroid::OnSanitizationSuccess,
                 base::Unretained(this)),
      base::Bind(&SafeJsonParserAndroid::OnSanitizationError,
                 base::Unretained(this)));
}

void SafeJsonParserAndroid::OnSanitizationSuccess(
    const std::string& sanitized_json) {
  // Self-destruct at the end of this method.
  std::unique_ptr<SafeJsonParserAndroid> deleter(this);

  int error_code;
  std::string error;
  std::unique_ptr<base::Value> value = base::JSONReader::ReadAndReturnError(
      sanitized_json, base::JSON_PARSE_RFC, &error_code, &error);

  if (!value) {
    error_callback_.Run(error);
    return;
  }

  success_callback_.Run(std::move(value));
}

void SafeJsonParserAndroid::OnSanitizationError(const std::string& error) {
  error_callback_.Run(error);
  delete this;
}

}  // namespace data_decoder
