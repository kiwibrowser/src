// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_LOG_DECODER_H_
#define COMPONENTS_METRICS_LOG_DECODER_H_

#include <string>

namespace metrics {

// Other modules can call this function instead of directly calling gzip. This
// prevents other modules from having to depend on zlib, or being aware of
// metrics' use of gzip compression, which is a metrics implementation detail.
// Returns true on success, false on failure.
bool DecodeLogData(const std::string& compressed_log_data,
                   std::string* log_data);

}  // namespace metrics

#endif  // COMPONENTS_METRICS_LOG_DECODER_H_
