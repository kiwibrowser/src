// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_PARSER_ANDROID_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_PARSER_ANDROID_H_

#include <memory>

#include "base/macros.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"

namespace data_decoder {

class SafeJsonParserAndroid : public SafeJsonParser {
 public:
  SafeJsonParserAndroid(const std::string& unsafe_json,
                        const SuccessCallback& success_callback,
                        const ErrorCallback& error_callback);

 private:
  friend std::default_delete<SafeJsonParserAndroid>;

  ~SafeJsonParserAndroid() override;

  void OnSanitizationSuccess(const std::string& sanitized_json);
  void OnSanitizationError(const std::string& error);

  // SafeJsonParser implementation.
  void Start() override;

  const std::string unsafe_json_;
  SuccessCallback success_callback_;
  ErrorCallback error_callback_;

  DISALLOW_COPY_AND_ASSIGN(SafeJsonParserAndroid);
};

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_PARSER_ANDROID_H_
