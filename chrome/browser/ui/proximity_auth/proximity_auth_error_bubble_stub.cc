// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/proximity_auth/proximity_auth_error_bubble.h"

#include "base/logging.h"
#include "ui/base/ui_features.h"

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
void ShowProximityAuthErrorBubble(const base::string16& message,
                                  const gfx::Range& link_range,
                                  const GURL& link_url,
                                  const gfx::Rect& anchor_rect,
                                  content::WebContents* web_contents) {
  NOTIMPLEMENTED();
}

void HideProximityAuthErrorBubble() {
  NOTIMPLEMENTED();
}
#endif
