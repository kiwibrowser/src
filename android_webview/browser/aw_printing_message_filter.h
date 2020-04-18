// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_PRINTING_MESSAGE_FILTER_H_
#define ANDROID_WEBVIEW_BROWSER_AW_PRINTING_MESSAGE_FILTER_H_

#include "base/macros.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"

namespace base {
struct FileDescriptor;
}

namespace android_webview {

// This class filters out incoming printing related IPC messages for the
// renderer process on the IPC thread.
class AwPrintingMessageFilter : public content::BrowserMessageFilter {
 public:
  explicit AwPrintingMessageFilter(int render_process_id);

  // content::BrowserMessageFilter methods.
  void OverrideThreadForMessage(const IPC::Message& message,
                                content::BrowserThread::ID* thread) override;
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  ~AwPrintingMessageFilter() override;

  // Used to ask the browser allocate a temporary file for the renderer
  // to fill in resulting PDF in renderer.
  void OnAllocateTempFileForPrinting(int render_frame_id,
                                     base::FileDescriptor* temp_file_fd,
                                     int* sequence_number);
  void OnTempFileForPrintingWritten(int render_frame_id,
                                    int sequence_number,
                                    int page_count);

  const int render_process_id_;

  DISALLOW_COPY_AND_ASSIGN(AwPrintingMessageFilter);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_PRINTING_MESSAGE_FILTER_H_
