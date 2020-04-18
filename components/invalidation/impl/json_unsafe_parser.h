// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_JSON_UNSAFE_PARSER_H_
#define COMPONENTS_INVALIDATION_IMPL_JSON_UNSAFE_PARSER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"

namespace base {
class Value;
}

namespace syncer {

// Mimics SafeJsonParser, but parses unsafely.
//
// Do not use this class, unless you can't help it. On most platforms,
// SafeJsonParser is available and is safer. If it is not available (e.g. on
// iOS), then this class mimics its API without its safety.
//
// TODO(https://crbug.com/828833): This code is the duplicate of same code in
// the ntp component. It should be removed, once appropriate place is found.
class JsonUnsafeParser {
 public:
  using SuccessCallback =
      base::OnceCallback<void(std::unique_ptr<base::Value>)>;
  using ErrorCallback = base::OnceCallback<void(const std::string&)>;

  // As with SafeJsonParser, runs either success_callback or error_callback on
  // the calling thread, but not before the call returns.
  static void Parse(const std::string& unsafe_json,
                    SuccessCallback success_callback,
                    ErrorCallback error_callback);

  JsonUnsafeParser() = delete;
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_IMPL_JSON_UNSAFE_PARSER_H_
