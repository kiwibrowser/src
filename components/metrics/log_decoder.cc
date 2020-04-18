// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/log_decoder.h"

#include "third_party/zlib/google/compression_utils.h"

namespace metrics {

bool DecodeLogData(const std::string& compressed_log_data,
                   std::string* log_data) {
  return compression::GzipUncompress(compressed_log_data, log_data);
}

}  // namespace metrics
