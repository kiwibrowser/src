// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_MHTML_GENERATION_PARAMS_H_
#define CONTENT_PUBLIC_COMMON_MHTML_GENERATION_PARAMS_H_

#include "base/files/file_path.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/web/web_frame_serializer_cache_control_policy.h"

namespace content {

struct CONTENT_EXPORT MHTMLGenerationParams {
  MHTMLGenerationParams(const base::FilePath& file_path);
  ~MHTMLGenerationParams() = default;

  // The file that will contain the generated MHTML.
  base::FilePath file_path;

  // Uses Content-Transfer-Encoding: binary when encoding.  See
  // https://tools.ietf.org/html/rfc2045 for details about
  // Content-Transfer-Encoding.
  bool use_binary_encoding = false;

  // By default, MHTML includes all subresources.  This flag can be used to
  // cause the generator to fail or silently ignore resources if the
  // Cache-Control header is used.
  blink::WebFrameSerializerCacheControlPolicy cache_control_policy =
      blink::WebFrameSerializerCacheControlPolicy::kNone;

  // Removes popups that could obstruct the user's view of normal content.
  bool remove_popup_overlay = false;

  // Run page problem detectors while generating MTHML if true.
  bool use_page_problem_detectors = false;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_MHTML_GENERATION_PARAMS_H_
