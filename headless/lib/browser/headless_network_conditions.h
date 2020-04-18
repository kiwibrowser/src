// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_CONDITIONS_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_CONDITIONS_H_

#include <string>
#include <vector>

#include "base/macros.h"

#include "headless/public/headless_export.h"

namespace headless {

// HeadlessNetworkConditions holds information about desired network conditions.
struct HEADLESS_EXPORT HeadlessNetworkConditions {
  HeadlessNetworkConditions();
  ~HeadlessNetworkConditions();

  explicit HeadlessNetworkConditions(bool offline);
  HeadlessNetworkConditions(bool offline,
                            double latency,
                            double download_throughput,
                            double upload_throughput);

  bool offline;
  double latency;
  double download_throughput;
  double upload_throughput;
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_CONDITIONS_H_
