// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_TESTING_JSON_PARSER_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_TESTING_JSON_PARSER_H_

#include "base/macros.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"

namespace data_decoder {

// An implementation of SafeJsonParser that parses JSON in process. This can be
// used in unit tests to avoid having to set up the multiprocess infrastructure
// necessary for the out-of-process SafeJsonParser.
class TestingJsonParser : public SafeJsonParser {
 public:
  // A helper class that will temporarily override the SafeJsonParser factory to
  // create instances of this class.
  class ScopedFactoryOverride {
   public:
    ScopedFactoryOverride();
    ~ScopedFactoryOverride();

   private:
    DISALLOW_COPY_AND_ASSIGN(ScopedFactoryOverride);
  };

  // If instantiating this class manually, it must be allocated with |new| (i.e.
  // not on the stack). It will delete itself after calling one of the
  // callbacks.
  TestingJsonParser(const std::string& unsafe_json,
                    const SuccessCallback& success_callback,
                    const ErrorCallback& error_callback);
  ~TestingJsonParser() override;

 private:
  void Start() override;

  const std::string unsafe_json_;
  SuccessCallback success_callback_;
  ErrorCallback error_callback_;

  DISALLOW_COPY_AND_ASSIGN(TestingJsonParser);
};

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_TESTING_JSON_PARSER_H_
