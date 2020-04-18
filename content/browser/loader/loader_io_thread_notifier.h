// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_LOADER_IO_THREAD_NOTIFIER
#define CONTENT_BROWSER_LOADER_LOADER_IO_THREAD_NOTIFIER

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class RenderFrameHost;
class WebContents;

// This class is responsible for notifying the IO thread (specifically, the
// ResourceDispatcherHostImpl) of frame events. It has an interace for callers
// to use and also sends notifications on WebContentsObserver events. All
// methods (static or class) will be called from the UI thread and post to the
// IO thread.
class LoaderIOThreadNotifier : public WebContentsObserver {
 public:
  explicit LoaderIOThreadNotifier(WebContents* web_contents);
  ~LoaderIOThreadNotifier() override;

  // content::WebContentsObserver:
  void RenderFrameDeleted(RenderFrameHost* render_frame_host) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LoaderIOThreadNotifier);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_LOADER_IO_THREAD_NOTIFIER
