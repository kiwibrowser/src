// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_view_manager.h"

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "build/build_config.h"
#include "chrome/browser/plugins/chrome_plugin_service_filter.h"
#include "chrome/browser/printing/print_preview_dialog_controller.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "chrome/common/chrome_content_client.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/webplugininfo.h"
#include "ipc/ipc_message_macros.h"
#include "printing/buildflags/buildflags.h"

using content::BrowserThread;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintViewManager);

namespace {

// Keeps track of pending scripted print preview closures.
// No locking, only access on the UI thread.
base::LazyInstance<std::map<content::RenderProcessHost*, base::Closure>>::Leaky
    g_scripted_print_preview_closure_map = LAZY_INSTANCE_INITIALIZER;

void EnableInternalPDFPluginForContents(int render_process_id,
                                        int render_frame_id) {
  // Always enable the internal PDF plugin for the print preview page.
  base::FilePath pdf_plugin_path = base::FilePath::FromUTF8Unsafe(
      ChromeContentClient::kPDFPluginPath);

  content::WebPluginInfo pdf_plugin;
  if (!content::PluginService::GetInstance()->GetPluginInfoByPath(
      pdf_plugin_path, &pdf_plugin)) {
    return;
  }

  ChromePluginServiceFilter::GetInstance()->OverridePluginForFrame(
      render_process_id, render_frame_id, GURL(), pdf_plugin);
}

}  // namespace

namespace printing {

struct PrintViewManager::FrameDispatchHelper {
  PrintViewManager* manager;
  content::RenderFrameHost* render_frame_host;

  bool Send(IPC::Message* msg) { return render_frame_host->Send(msg); }

  void OnSetupScriptedPrintPreview(IPC::Message* reply_msg) {
    manager->OnSetupScriptedPrintPreview(render_frame_host, reply_msg);
  }
};

PrintViewManager::PrintViewManager(content::WebContents* web_contents)
    : PrintViewManagerBase(web_contents),
      print_preview_state_(NOT_PREVIEWING),
      print_preview_rfh_(nullptr),
      scripted_print_preview_rph_(nullptr),
      is_switching_to_system_dialog_(false) {
  if (PrintPreviewDialogController::IsPrintPreviewURL(web_contents->GetURL())) {
    EnableInternalPDFPluginForContents(
        web_contents->GetMainFrame()->GetProcess()->GetID(),
        web_contents->GetMainFrame()->GetRoutingID());
  }
}

PrintViewManager::~PrintViewManager() {
  DCHECK_EQ(NOT_PREVIEWING, print_preview_state_);
}

bool PrintViewManager::PrintForSystemDialogNow(
    const base::Closure& dialog_shown_callback) {
  DCHECK(!dialog_shown_callback.is_null());
  DCHECK(on_print_dialog_shown_callback_.is_null());
  on_print_dialog_shown_callback_ = dialog_shown_callback;
  is_switching_to_system_dialog_ = true;

  SetPrintingRFH(print_preview_rfh_);
  int32_t id = print_preview_rfh_->GetRoutingID();
  return PrintNowInternal(print_preview_rfh_,
                          std::make_unique<PrintMsg_PrintForSystemDialog>(id));
}

bool PrintViewManager::BasicPrint(content::RenderFrameHost* rfh) {
  PrintPreviewDialogController* dialog_controller =
      PrintPreviewDialogController::GetInstance();
  if (!dialog_controller)
    return false;

  content::WebContents* print_preview_dialog =
      dialog_controller->GetPrintPreviewForContents(web_contents());
  if (!print_preview_dialog)
    return PrintNow(rfh);

  return !!print_preview_dialog->GetWebUI();
}

bool PrintViewManager::PrintPreviewNow(content::RenderFrameHost* rfh,
                                       bool has_selection) {
  // Users can send print commands all they want and it is beyond
  // PrintViewManager's control. Just ignore the extra commands.
  // See http://crbug.com/136842 for example.
  if (print_preview_state_ != NOT_PREVIEWING)
    return false;

  auto message = std::make_unique<PrintMsg_InitiatePrintPreview>(
      rfh->GetRoutingID(), has_selection);
  if (!PrintNowInternal(rfh, std::move(message)))
    return false;

  DCHECK(!print_preview_rfh_);
  print_preview_rfh_ = rfh;
  print_preview_state_ = USER_INITIATED_PREVIEW;
  return true;
}

void PrintViewManager::PrintPreviewForWebNode(content::RenderFrameHost* rfh) {
  if (print_preview_state_ != NOT_PREVIEWING)
    return;

  DCHECK(rfh);
  DCHECK(!print_preview_rfh_);
  print_preview_rfh_ = rfh;
  print_preview_state_ = USER_INITIATED_PREVIEW;
}

void PrintViewManager::PrintPreviewDone() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (print_preview_state_ == NOT_PREVIEWING)
    return;

// Send ClosePrintPreview message for 'afterprint' event.
#if defined(OS_WIN)
  // On Windows, we always send ClosePrintPreviewDialog. It's ok to dispatch
  // 'afterprint' at this timing because system dialog printing on
  // Windows doesn't need the original frame.
  bool send_message = true;
#else
  // On non-Windows, we don't need to send ClosePrintPreviewDialog when we are
  // switching to system dialog. PrintRenderFrameHelper is responsible to
  // dispatch 'afterprint' event.
  bool send_message = !is_switching_to_system_dialog_;
#endif
  if (send_message) {
    print_preview_rfh_->Send(new PrintMsg_ClosePrintPreviewDialog(
        print_preview_rfh_->GetRoutingID()));
  }
  is_switching_to_system_dialog_ = false;

  if (print_preview_state_ == SCRIPTED_PREVIEW) {
    auto& map = g_scripted_print_preview_closure_map.Get();
    auto it = map.find(scripted_print_preview_rph_);
    CHECK(it != map.end());
    it->second.Run();
    map.erase(it);
    scripted_print_preview_rph_ = nullptr;
  }
  print_preview_state_ = NOT_PREVIEWING;
  print_preview_rfh_ = nullptr;
}

void PrintViewManager::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  if (PrintPreviewDialogController::IsPrintPreviewURL(
          web_contents()->GetURL())) {
    EnableInternalPDFPluginForContents(render_frame_host->GetProcess()->GetID(),
                                       render_frame_host->GetRoutingID());
  }
}

void PrintViewManager::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == print_preview_rfh_)
    PrintPreviewDone();
  PrintViewManagerBase::RenderFrameDeleted(render_frame_host);
}

void PrintViewManager::OnDidShowPrintDialog(content::RenderFrameHost* rfh) {
  if (rfh != print_preview_rfh_)
    return;

  if (on_print_dialog_shown_callback_.is_null())
    return;

  on_print_dialog_shown_callback_.Run();
  on_print_dialog_shown_callback_.Reset();
}

void PrintViewManager::OnSetupScriptedPrintPreview(
    content::RenderFrameHost* rfh,
    IPC::Message* reply_msg) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto& map = g_scripted_print_preview_closure_map.Get();
  content::RenderProcessHost* rph = rfh->GetProcess();

  if (base::ContainsKey(map, rph)) {
    // Renderer already handling window.print(). Abort this attempt to prevent
    // the renderer from having multiple nested loops. If multiple nested loops
    // existed, then they have to exit in the right order and that is messy.
    rfh->Send(reply_msg);
    return;
  }

  if (print_preview_state_ != NOT_PREVIEWING) {
    // If a print dialog is already open for this tab, ignore the scripted print
    // message.
    rfh->Send(reply_msg);
    return;
  }

  PrintPreviewDialogController* dialog_controller =
      PrintPreviewDialogController::GetInstance();
  if (!dialog_controller) {
    rfh->Send(reply_msg);
    return;
  }

  DCHECK(!print_preview_rfh_);
  print_preview_rfh_ = rfh;
  print_preview_state_ = SCRIPTED_PREVIEW;
  map[rph] = base::Bind(&PrintViewManager::OnScriptedPrintPreviewReply,
                        base::Unretained(this), reply_msg);
  scripted_print_preview_rph_ = rph;
}

void PrintViewManager::OnShowScriptedPrintPreview(content::RenderFrameHost* rfh,
                                                  bool source_is_modifiable) {
  DCHECK(print_preview_rfh_);
  if (rfh != print_preview_rfh_)
    return;

  PrintPreviewDialogController* dialog_controller =
      PrintPreviewDialogController::GetInstance();
  if (!dialog_controller) {
    PrintPreviewDone();
    return;
  }

  // Running a dialog causes an exit to webpage-initiated fullscreen.
  // http://crbug.com/728276
  if (web_contents()->IsFullscreenForCurrentTab())
    web_contents()->ExitFullscreen(true);

  dialog_controller->PrintPreview(web_contents());
  PrintHostMsg_RequestPrintPreview_Params params;
  params.is_modifiable = source_is_modifiable;
  PrintPreviewUI::SetInitialParams(
      dialog_controller->GetPrintPreviewForContents(web_contents()), params);
}

void PrintViewManager::OnScriptedPrintPreviewReply(IPC::Message* reply_msg) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  print_preview_rfh_->Send(reply_msg);
}

bool PrintViewManager::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  FrameDispatchHelper helper = {this, render_frame_host};
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(PrintViewManager, message, render_frame_host)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidShowPrintDialog, OnDidShowPrintDialog)
    IPC_MESSAGE_FORWARD_DELAY_REPLY(
        PrintHostMsg_SetupScriptedPrintPreview, &helper,
        FrameDispatchHelper::OnSetupScriptedPrintPreview)
    IPC_MESSAGE_HANDLER(PrintHostMsg_ShowScriptedPrintPreview,
                        OnShowScriptedPrintPreview)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled ||
         PrintViewManagerBase::OnMessageReceived(message, render_frame_host);
}

}  // namespace printing
