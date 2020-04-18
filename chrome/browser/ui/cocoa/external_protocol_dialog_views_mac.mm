// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/external_protocol/external_protocol_handler.h"

#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/external_protocol_dialog.h"
#include "chrome/browser/ui/external_protocol_dialog_delegate.h"
#include "chrome/browser/ui/views/external_protocol_dialog.h"

namespace {

// Note: This is duplicated from
// chrome/browser/ui/views/external_protocol_dialog.cc.
void RunExternalProtocolDialogViews(const GURL& url,
                                    int render_process_host_id,
                                    int routing_id,
                                    ui::PageTransition page_transition,
                                    bool has_user_gesture) {
  std::unique_ptr<ExternalProtocolDialogDelegate> delegate(
      new ExternalProtocolDialogDelegate(url, render_process_host_id,
                                         routing_id));
  if (delegate->program_name().empty()) {
    // ShellExecute won't do anything. Don't bother warning the user.
    return;
  }

  // Windowing system takes ownership.
  new ExternalProtocolDialog(std::move(delegate), render_process_host_id,
                             routing_id);
}

}  // namespace

// static
void ExternalProtocolHandler::RunExternalProtocolDialog(
    const GURL& url,
    int render_process_host_id,
    int routing_id,
    ui::PageTransition page_transition,
    bool has_user_gesture) {
  if (chrome::ShowAllDialogsWithViewsToolkit()) {
    RunExternalProtocolDialogViews(url, render_process_host_id, routing_id,
                                   page_transition, has_user_gesture);
    return;
  }
  [[ExternalProtocolDialogController alloc] initWithGURL:&url
                                     renderProcessHostId:render_process_host_id
                                               routingId:routing_id];
}
