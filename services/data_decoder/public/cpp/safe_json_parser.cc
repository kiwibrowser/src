// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/public/cpp/safe_json_parser.h"

#include "build/build_config.h"

#if defined(OS_ANDROID)
#include "services/data_decoder/public/cpp/safe_json_parser_android.h"
#else
#include "services/data_decoder/public/cpp/safe_json_parser_impl.h"
#endif

namespace data_decoder {

namespace {

SafeJsonParser::Factory g_factory = nullptr;

SafeJsonParser* Create(service_manager::Connector* connector,
                       const std::string& unsafe_json,
                       const SafeJsonParser::SuccessCallback& success_callback,
                       const SafeJsonParser::ErrorCallback& error_callback) {
  if (g_factory)
    return g_factory(unsafe_json, success_callback, error_callback);

#if defined(OS_ANDROID)
  return new SafeJsonParserAndroid(unsafe_json, success_callback,
                                   error_callback);
#else
  return new SafeJsonParserImpl(connector, unsafe_json, success_callback,
                                error_callback);
#endif
}

}  // namespace

// static
void SafeJsonParser::SetFactoryForTesting(Factory factory) {
  g_factory = factory;
}

// static
void SafeJsonParser::Parse(service_manager::Connector* connector,
                           const std::string& unsafe_json,
                           const SuccessCallback& success_callback,
                           const ErrorCallback& error_callback) {
  SafeJsonParser* parser =
      Create(connector, unsafe_json, success_callback, error_callback);
  parser->Start();
}

}  // namespace data_decoder
