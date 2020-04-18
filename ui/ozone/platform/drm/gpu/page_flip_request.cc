// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/page_flip_request.h"

namespace ui {

PageFlipRequest::PageFlipRequest(int crtc_count,
                                 SwapCompletionOnceCallback callback)
    : callback_(std::move(callback)), crtc_count_(crtc_count) {}

PageFlipRequest::~PageFlipRequest() {
}

void PageFlipRequest::Signal(gfx::SwapResult result,
                             const gfx::PresentationFeedback& feedback) {
  if (result == gfx::SwapResult::SWAP_FAILED)
    result_ = gfx::SwapResult::SWAP_FAILED;
  else if (result != gfx::SwapResult::SWAP_ACK)
    result_ = result;

  if (!--crtc_count_)
    std::move(callback_).Run(result_, feedback);
}

}  // namespace ui
