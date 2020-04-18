// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/mhtml_generation_params.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "content/public/common/content_switches.h"

namespace content {

MHTMLGenerationParams::MHTMLGenerationParams(const base::FilePath& file_path)
    : file_path(file_path) {
  // Check which variant of MHTML generation is required.
  std::string mhtmlGeneratorOptionFlag =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
      switches::kMHTMLGeneratorOption);
  if (mhtmlGeneratorOptionFlag == switches::kMHTMLSkipNostoreMain) {
    cache_control_policy =
        blink::WebFrameSerializerCacheControlPolicy::kFailForNoStoreMainFrame;
  } else if (mhtmlGeneratorOptionFlag == switches::kMHTMLSkipNostoreAll) {
    cache_control_policy = blink::WebFrameSerializerCacheControlPolicy::
        kSkipAnyFrameOrResourceMarkedNoStore;
  }
}

}  // namespace content
