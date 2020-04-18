// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_PAGE_FLIP_REQUEST_H_
#define UI_OZONE_PLATFORM_DRM_GPU_PAGE_FLIP_REQUEST_H_

#include <memory>

#include "base/atomic_ref_count.h"
#include "base/callback.h"
#include "base/macros.h"
#include "ui/gfx/swap_result.h"
#include "ui/ozone/public/swap_completion_callback.h"

namespace gfx {
struct PresentationFeedback;
}  // namespace gfx

namespace ui {

class PageFlipRequest : public base::RefCounted<PageFlipRequest> {
 public:
  PageFlipRequest(int crtc_count, SwapCompletionOnceCallback callback);

  void Signal(gfx::SwapResult result,
              const gfx::PresentationFeedback& feedback);

 private:
  friend class base::RefCounted<PageFlipRequest>;
  ~PageFlipRequest();

  SwapCompletionOnceCallback callback_;
  int crtc_count_;
  gfx::SwapResult result_ = gfx::SwapResult::SWAP_ACK;

  DISALLOW_COPY_AND_ASSIGN(PageFlipRequest);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_PAGE_FLIP_REQUEST_H_
