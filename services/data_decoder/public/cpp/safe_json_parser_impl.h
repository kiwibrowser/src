// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_PARSER_IMPL_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_PARSER_IMPL_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"
#include "services/data_decoder/public/mojom/json_parser.mojom.h"

namespace service_manager {
class Connector;
}

namespace data_decoder {

class SafeJsonParserImpl : public SafeJsonParser {
 public:
  SafeJsonParserImpl(service_manager::Connector* connector,
                     const std::string& unsafe_json,
                     const SuccessCallback& success_callback,
                     const ErrorCallback& error_callback);

 private:
  ~SafeJsonParserImpl() override;

  // SafeJsonParser implementation.
  void Start() override;

  void StartOnIOThread();
  void OnConnectionError();

  // mojom::SafeJsonParser::Parse callback.
  void OnParseDone(base::Optional<base::Value> result,
                   const base::Optional<std::string>& error);

  // Reports the result on the calling task runner via the |success_callback_|
  // or the |error_callback_|.
  void ReportResults(std::unique_ptr<base::Value> parsed_json,
                     const std::string& error);

  const std::string unsafe_json_;
  SuccessCallback success_callback_;
  ErrorCallback error_callback_;

  mojom::JsonParserPtr json_parser_ptr_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SafeJsonParserImpl);
};

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_PARSER_IMPL_H_
