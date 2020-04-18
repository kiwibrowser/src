// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "components/payments/content/utility/payment_manifest_parser.h"
#include "components/payments/content/web_app_manifest.h"

struct Environment {
  Environment() { logging::SetMinLogLevel(logging::LOG_FATAL); }
};

Environment* env = new Environment();

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::string json_data(reinterpret_cast<const char*>(data), size);
  std::unique_ptr<base::Value> value = base::JSONReader::Read(json_data);

  std::vector<payments::WebAppManifestSection> output;
  payments::PaymentManifestParser::ParseWebAppManifestIntoVector(
      std::move(value), &output);
  return 0;
}
