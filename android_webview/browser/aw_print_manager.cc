// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_print_manager.h"

#include "base/memory/ptr_util.h"
#include "components/printing/browser/print_manager_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(android_webview::AwPrintManager);

namespace android_webview {

struct AwPrintManager::FrameDispatchHelper {
  AwPrintManager* manager;
  content::RenderFrameHost* render_frame_host;

  bool Send(IPC::Message* msg) { return render_frame_host->Send(msg); }

  void OnGetDefaultPrintSettings(IPC::Message* reply_msg) {
    manager->OnGetDefaultPrintSettings(render_frame_host, reply_msg);
  }

  void OnScriptedPrint(const PrintHostMsg_ScriptedPrint_Params& scripted_params,
                       IPC::Message* reply_msg) {
    manager->OnScriptedPrint(render_frame_host, scripted_params, reply_msg);
  }
};

// static
AwPrintManager* AwPrintManager::CreateForWebContents(
    content::WebContents* contents,
    const printing::PrintSettings& settings,
    const base::FileDescriptor& file_descriptor,
    PrintManager::PdfWritingDoneCallback callback) {
  AwPrintManager* print_manager = new AwPrintManager(
      contents, settings, file_descriptor, std::move(callback));
  contents->SetUserData(UserDataKey(), base::WrapUnique(print_manager));
  return print_manager;
}

AwPrintManager::AwPrintManager(content::WebContents* contents,
                               const printing::PrintSettings& settings,
                               const base::FileDescriptor& file_descriptor,
                               PdfWritingDoneCallback callback)
    : PrintManager(contents), settings_(settings) {
  set_file_descriptor(file_descriptor);
  pdf_writing_done_callback_ = std::move(callback);
  cookie_ = 1;
}

AwPrintManager::~AwPrintManager() {
}

bool AwPrintManager::PrintNow() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* rfh = web_contents()->GetMainFrame();
  return rfh->Send(new PrintMsg_PrintPages(rfh->GetRoutingID()));
}

bool AwPrintManager::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  FrameDispatchHelper helper = {this, render_frame_host};
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AwPrintManager, message)
    IPC_MESSAGE_FORWARD_DELAY_REPLY(
        PrintHostMsg_GetDefaultPrintSettings, &helper,
        FrameDispatchHelper::OnGetDefaultPrintSettings)
    IPC_MESSAGE_FORWARD_DELAY_REPLY(PrintHostMsg_ScriptedPrint, &helper,
                                    FrameDispatchHelper::OnScriptedPrint)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled ? true
                 : PrintManager::OnMessageReceived(message, render_frame_host);
}

void AwPrintManager::OnGetDefaultPrintSettings(
    content::RenderFrameHost* render_frame_host,
    IPC::Message* reply_msg) {
  // Unlike the printing_message_filter, we do process this in UI thread.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrintMsg_Print_Params params;
  printing::RenderParamsFromPrintSettings(settings_, &params);
  params.document_cookie = cookie_;
  PrintHostMsg_GetDefaultPrintSettings::WriteReplyParams(reply_msg, params);
  render_frame_host->Send(reply_msg);
}

void AwPrintManager::OnScriptedPrint(
    content::RenderFrameHost* render_frame_host,
    const PrintHostMsg_ScriptedPrint_Params& scripted_params,
    IPC::Message* reply_msg) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrintMsg_PrintPages_Params params;
  printing::RenderParamsFromPrintSettings(settings_, &params.params);
  params.params.document_cookie = scripted_params.cookie;
  params.pages = printing::PageRange::GetPages(settings_.ranges());
  PrintHostMsg_ScriptedPrint::WriteReplyParams(reply_msg, params);
  render_frame_host->Send(reply_msg);
}

}  // namespace android_webview
