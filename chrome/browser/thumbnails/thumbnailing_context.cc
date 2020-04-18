// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/thumbnails/thumbnailing_context.h"

namespace thumbnails {

ThumbnailingContext::ThumbnailingContext(const GURL& url,
                                         bool at_top,
                                         bool load_completed)
    : url(url), clip_result(CLIP_RESULT_UNPROCESSED) {
  score.at_top = at_top;
  score.load_completed = load_completed;
}

ThumbnailingContext::~ThumbnailingContext() = default;

}  // namespace thumbnails
