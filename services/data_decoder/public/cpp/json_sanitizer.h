// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_JSON_SANITIZER_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_JSON_SANITIZER_H_

#include <string>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"

#if defined(OS_ANDROID)
#include <jni.h>
#endif

namespace service_manager {
class Connector;
}

namespace data_decoder {

// Sanitizes and normalizes JSON by parsing it in a safe environment and
// re-serializing it. Parsing the sanitized JSON should result in a value
// identical to parsing the original JSON.
// This allows parsing the sanitized JSON with the regular JSONParser while
// reducing the risk versus parsing completely untrusted JSON. It also minifies
// the resulting JSON, which might save some space.
class JsonSanitizer {
 public:
  using StringCallback = base::Callback<void(const std::string&)>;

  // Starts sanitizing the passed in unsafe JSON string. Either the passed
  // |success_callback| or the |error_callback| will be called with the result
  // of the sanitization or an error message, respectively, but not before the
  // method returns.
  // |connector| is the connector provided by the service manager and is used
  // to retrieve the JSON decoder service. It's commonly retrieved from a
  // service manager connection context object that the embedder provides.
  static void Sanitize(service_manager::Connector* connector,
                       const std::string& unsafe_json,
                       const StringCallback& success_callback,
                       const StringCallback& error_callback);

 protected:
  JsonSanitizer() {}
  ~JsonSanitizer() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(JsonSanitizer);
};

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_SAFE_JSON_JSON_SANITIZER_H_
