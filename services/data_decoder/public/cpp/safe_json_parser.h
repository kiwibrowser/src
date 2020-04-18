// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_PARSER_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_PARSER_H_

#include <memory>
#include <string>

#include "base/callback.h"

namespace base {
class Value;
}

namespace service_manager {
class Connector;
}

namespace data_decoder {

// SafeJsonParser parses a given JSON safely via a platform-dependent mechanism
// (like parsing it in a utility process or in a memory-safe environment).
// Internally, an instance of this class is created when Parse() is called and
// is kept alive until one of the two callbacks is called, after which it
// deletes itself.
class SafeJsonParser {
 public:
  using SuccessCallback = base::Callback<void(std::unique_ptr<base::Value>)>;
  using ErrorCallback = base::Callback<void(const std::string&)>;

  using Factory = SafeJsonParser* (*)(const std::string& unsafe_json,
                                      const SuccessCallback& success_callback,
                                      const ErrorCallback& error_callback);

  // Starts parsing the passed in |unsafe_json| and calls either
  // |success_callback| or |error_callback| when finished.
  // |connector| is the connector provided by the service manager and is used
  // to retrieve the JSON decoder service. It's commonly retrieved from a
  // service manager connection context object that the embedder provides.
  // Note that on Android, this runs in process, the sanitizing of |unsafe_json|
  // being performed in Java for safety. On other platforms the parsing happens
  // in an isolated sandboxed utility process.
  static void Parse(service_manager::Connector* connector,
                    const std::string& unsafe_json,
                    const SuccessCallback& success_callback,
                    const ErrorCallback& error_callback);

  static void SetFactoryForTesting(Factory factory);

 protected:
  virtual ~SafeJsonParser() {}

 private:
  virtual void Start() = 0;
};

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_PARSER_H_
