// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_DEVTOOLS_BINDINGS_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_DEVTOOLS_BINDINGS_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/shell/browser/shell_devtools_frontend.h"

namespace content {

class WebContents;

class LayoutTestDevToolsBindings : public ShellDevToolsBindings {
 public:
  static GURL MapTestURLIfNeeded(const GURL& test_url, bool* is_devtools_test);

  LayoutTestDevToolsBindings(WebContents* devtools_contents,
                             WebContents* inspected_contents,
                             const GURL& frontend_url);

  void Attach() override;

  ~LayoutTestDevToolsBindings() override;

 private:
  class SecondaryObserver;

  // WebContentsObserver implementation.
  void RenderProcessGone(base::TerminationStatus status) override;
  void RenderFrameCreated(RenderFrameHost* render_frame_host) override;
  void DocumentAvailableInMainFrame() override;

  void NavigateDevToolsFrontend();

  bool is_startup_test_ = false;
  GURL frontend_url_;
  std::unique_ptr<SecondaryObserver> secondary_observer_;

  DISALLOW_COPY_AND_ASSIGN(LayoutTestDevToolsBindings);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_DEVTOOLS_BINDINGS_H_
