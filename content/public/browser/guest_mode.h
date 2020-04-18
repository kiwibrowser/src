// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_GUEST_MODE_H_
#define CONTENT_PUBLIC_BROWSER_GUEST_MODE_H_

#include "base/macros.h"
#include "content/common/content_export.h"

namespace content {

class WebContents;

class CONTENT_EXPORT GuestMode {
 public:
  // Returns true if |web_contents| is an inner WebContents based on cross
  // process frames.
  static bool IsCrossProcessFrameGuest(WebContents* web_contents);

 private:
  GuestMode();

  DISALLOW_COPY_AND_ASSIGN(GuestMode);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_GUEST_MODE_H_
