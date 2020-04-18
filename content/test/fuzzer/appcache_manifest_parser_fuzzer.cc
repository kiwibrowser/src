// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/i18n/icu_util.h"
#include "content/browser/appcache/appcache_manifest_parser.h"  // nogncheck
#include "url/gurl.h"

namespace content {

struct IcuEnvironment {
  IcuEnvironment() { CHECK(base::i18n::InitializeICU()); }
  // used by ICU integration.
  base::AtExitManager at_exit_manager;
};

IcuEnvironment* env = new IcuEnvironment();

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  AppCacheManifest manifest;
  const GURL kUrl("http://www.example.com");
  ParseManifest(kUrl, reinterpret_cast<const char*>(data), size,
                PARSE_MANIFEST_ALLOWING_DANGEROUS_FEATURES, manifest);
  return 0;
}

}  // namespace content
