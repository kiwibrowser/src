// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BROWSER_PLUGIN_BROWSER_PLUGIN_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_BROWSER_PLUGIN_BROWSER_PLUGIN_MESSAGE_FILTER_H_

#include "base/macros.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"

namespace content {

// This class filters out incoming IPC messages for the guest renderer process
// on the IPC thread before other message filters handle them.
class BrowserPluginMessageFilter : public BrowserMessageFilter {
 public:
  explicit BrowserPluginMessageFilter(int render_process_id);

  // BrowserMessageFilter implementation.
  void OverrideThreadForMessage(const IPC::Message& message,
                                BrowserThread::ID* thread) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnDestruct() const override;

  // Test-only functions:
  void SetSubFilterForTesting(scoped_refptr<BrowserMessageFilter> sub_filter);

 private:
  friend class BrowserThread;
  friend class base::DeleteHelper<BrowserPluginMessageFilter>;

  ~BrowserPluginMessageFilter() override;

  void ForwardMessageToGuest(const IPC::Message& message);

  const int render_process_id_;

  scoped_refptr<BrowserMessageFilter> sub_filter_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(BrowserPluginMessageFilter);
};

} // namespace content

#endif  // CONTENT_BROWSER_BROWSER_PLUGIN_BROWSER_PLUGIN_MESSAGE_FILTER_H_
