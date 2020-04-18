// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_printing_message_filter.h"

#include "android_webview/browser/aw_print_manager.h"
#include "base/file_descriptor_posix.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

using content::BrowserThread;
using content::WebContents;

namespace android_webview {

namespace {

AwPrintManager* GetPrintManager(int render_process_id, int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::RenderFrameHost* frame =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (!frame)
    return nullptr;
  WebContents* web_contents = WebContents::FromRenderFrameHost(frame);
  return web_contents ? AwPrintManager::FromWebContents(web_contents) : nullptr;
}

} // namespace

AwPrintingMessageFilter::AwPrintingMessageFilter(int render_process_id)
    : BrowserMessageFilter(PrintMsgStart),
      render_process_id_(render_process_id) {
}

AwPrintingMessageFilter::~AwPrintingMessageFilter() {
}

void AwPrintingMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message, BrowserThread::ID* thread) {
  if (message.type() == PrintHostMsg_AllocateTempFileForPrinting::ID ||
      message.type() == PrintHostMsg_TempFileForPrintingWritten::ID) {
    *thread = BrowserThread::UI;
  }
}

bool AwPrintingMessageFilter::OnMessageReceived(const IPC::Message& message) {
  // TODO(timvolodine): move this filter to component (crbug.com/500949).
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AwPrintingMessageFilter, message)
    IPC_MESSAGE_HANDLER(PrintHostMsg_AllocateTempFileForPrinting,
                        OnAllocateTempFileForPrinting)
    IPC_MESSAGE_HANDLER(PrintHostMsg_TempFileForPrintingWritten,
                        OnTempFileForPrintingWritten)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void AwPrintingMessageFilter::OnAllocateTempFileForPrinting(
    int render_frame_id,
    base::FileDescriptor* temp_file_fd,
    int* sequence_number) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  AwPrintManager* print_manager =
      GetPrintManager(render_process_id_, render_frame_id);
  if (!print_manager)
    return;

  *sequence_number = 0;  // we don't really use the sequence number.
  temp_file_fd->fd = print_manager->file_descriptor().fd;
  temp_file_fd->auto_close = false;
}

void AwPrintingMessageFilter::OnTempFileForPrintingWritten(int render_frame_id,
                                                           int sequence_number,
                                                           int page_count) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_GT(page_count, 0);
  AwPrintManager* print_manager =
      GetPrintManager(render_process_id_, render_frame_id);
  if (print_manager)
    print_manager->PdfWritingDone(page_count);
}

}  // namespace android_webview
