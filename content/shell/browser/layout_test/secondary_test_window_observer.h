// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_SECONDARY_TEST_WINDOW_OBSERVER_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_SECONDARY_TEST_WINDOW_OBSERVER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {

class SecondaryTestWindowObserver
    : public WebContentsObserver,
      public WebContentsUserData<SecondaryTestWindowObserver> {
 public:
  ~SecondaryTestWindowObserver() override;

  // WebContentsObserver implementation.
  void RenderFrameCreated(RenderFrameHost* render_frame_host) override;

 private:
  friend class WebContentsUserData<SecondaryTestWindowObserver>;
  explicit SecondaryTestWindowObserver(WebContents* web_contents);

  DISALLOW_COPY_AND_ASSIGN(SecondaryTestWindowObserver);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_SECONDARY_TEST_WINDOW_OBSERVER_H_
