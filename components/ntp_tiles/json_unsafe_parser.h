// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_TILES_JSON_UNSAFE_PARSER_H_
#define COMPONENTS_NTP_TILES_JSON_UNSAFE_PARSER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"

namespace base {
class Value;
}

namespace ntp_tiles {

// Mimics SafeJsonParser, but parses unsafely.
//
// Do not use this class, unless you can't help it. On most platforms,
// SafeJsonParser is available and is safer. If it is not available (e.g. on
// iOS), then this class mimics its API without its safety.
class JsonUnsafeParser {
 public:
  using SuccessCallback = base::Callback<void(std::unique_ptr<base::Value>)>;
  using ErrorCallback = base::Callback<void(const std::string&)>;

  // As with SafeJsonParser, runs either success_callback or error_callback on
  // the calling thread, but not before the call returns.
  static void Parse(const std::string& unsafe_json,
                    const SuccessCallback& success_callback,
                    const ErrorCallback& error_callback);

  JsonUnsafeParser() = delete;
};

}  // namespace ntp_tiles

#endif  // COMPONENTS_NTP_TILES_POPULAR_SITES_H_
