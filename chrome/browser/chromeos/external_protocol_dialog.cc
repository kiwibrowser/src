// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/external_protocol_dialog.h"

#include "chrome/browser/chromeos/arc/intent_helper/arc_external_protocol_dialog.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/text_elider.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

using content::WebContents;

namespace {

const int kMessageWidth = 400;

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// ExternalProtocolHandler

// static
void ExternalProtocolHandler::RunExternalProtocolDialog(
    const GURL& url,
    int render_process_host_id,
    int routing_id,
    ui::PageTransition page_transition,
    bool has_user_gesture) {
  // First, check if ARC version of the dialog is available and run ARC version
  // when possible.
  if (arc::RunArcExternalProtocolDialog(url, render_process_host_id, routing_id,
                                        page_transition, has_user_gesture)) {
    return;
  }
  WebContents* web_contents = tab_util::GetWebContentsByID(
      render_process_host_id, routing_id);
  new ExternalProtocolDialog(web_contents, url);
}

///////////////////////////////////////////////////////////////////////////////
// ExternalProtocolDialog

ExternalProtocolDialog::~ExternalProtocolDialog() {
}

//////////////////////////////////////////////////////////////////////////////
// ExternalProtocolDialog, views::DialogDelegate implementation:

int ExternalProtocolDialog::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK;
}

base::string16 ExternalProtocolDialog::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(IDS_EXTERNAL_PROTOCOL_OK_BUTTON_TEXT);
}

base::string16 ExternalProtocolDialog::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_EXTERNAL_PROTOCOL_TITLE);
}

void ExternalProtocolDialog::DeleteDelegate() {
  delete this;
}

views::View* ExternalProtocolDialog::GetContentsView() {
  return message_box_view_;
}

const views::Widget* ExternalProtocolDialog::GetWidget() const {
  return message_box_view_->GetWidget();
}

views::Widget* ExternalProtocolDialog::GetWidget() {
  return message_box_view_->GetWidget();
}

///////////////////////////////////////////////////////////////////////////////
// ExternalProtocolDialog, private:

ExternalProtocolDialog::ExternalProtocolDialog(WebContents* web_contents,
                                               const GURL& url)
    : creation_time_(base::TimeTicks::Now()),
      scheme_(url.scheme()) {
  views::MessageBoxView::InitParams params((base::string16()));
  params.message_width = kMessageWidth;
  message_box_view_ = new views::MessageBoxView(params);

  gfx::NativeWindow parent_window;
  if (web_contents) {
    parent_window = web_contents->GetTopLevelNativeWindow();
  } else {
    // Dialog is top level if we don't have a web_contents associated with us.
    parent_window = NULL;
  }
  views::DialogDelegate::CreateDialogWidget(this, NULL, parent_window)->Show();
  chrome::RecordDialogCreation(
      chrome::DialogIdentifier::EXTERNAL_PROTOCOL_CHROMEOS);
}
