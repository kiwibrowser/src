// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXTERNAL_PROTOCOL_DIALOG_H_
#define CHROME_BROWSER_UI_VIEWS_EXTERNAL_PROTOCOL_DIALOG_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "ui/views/window/dialog_delegate.h"
#include "url/gurl.h"

class ProtocolDialogDelegate;

namespace test {
class ExternalProtocolDialogTestApi;
}

namespace views {
class Checkbox;
}

class ExternalProtocolDialog : public views::DialogDelegateView {
 public:
  // Show by calling ExternalProtocolHandler::RunExternalProtocolDialog().
  ExternalProtocolDialog(std::unique_ptr<const ProtocolDialogDelegate> delegate,
                         int render_process_host_id,
                         int routing_id);

  ~ExternalProtocolDialog() override;

  // views::DialogDelegateView:
  gfx::Size CalculatePreferredSize() const override;
  bool ShouldShowCloseButton() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  base::string16 GetWindowTitle() const override;
  bool Cancel() override;
  bool Accept() override;
  ui::ModalType GetModalType() const override;

 private:
  friend class test::ExternalProtocolDialogTestApi;

  const std::unique_ptr<const ProtocolDialogDelegate> delegate_;

  views::Checkbox* remember_decision_checkbox_;

  // IDs of the associated WebContents.
  int render_process_host_id_;
  int routing_id_;

  // The time at which this dialog was created.
  base::TimeTicks creation_time_;

  DISALLOW_COPY_AND_ASSIGN(ExternalProtocolDialog);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXTERNAL_PROTOCOL_DIALOG_H_
