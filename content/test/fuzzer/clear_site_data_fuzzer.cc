// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "base/command_line.h"
#include "content/browser/browsing_data/clear_site_data_throttle.h"  // nogncheck

namespace content {

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  base::CommandLine::Init(*argc, *argv);
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::string header(reinterpret_cast<const char*>(data), size);

  bool remove_cookies;
  bool remove_storage;
  bool remove_cache;
  ClearSiteDataThrottle::ConsoleMessagesDelegate delegate_;

  content::ClearSiteDataThrottle::ParseHeaderForTesting(
      header, &remove_cookies, &remove_storage, &remove_cache, &delegate_,
      GURL());

  return 0;
}

}  // namespace content
