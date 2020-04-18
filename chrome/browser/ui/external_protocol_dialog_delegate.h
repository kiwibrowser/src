// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXTERNAL_PROTOCOL_DIALOG_DELEGATE_H_
#define CHROME_BROWSER_UI_EXTERNAL_PROTOCOL_DIALOG_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/ui/protocol_dialog_delegate.h"
#include "url/gurl.h"

// Provides text for the external protocol handler dialog and handles whether
// or not to launch the application for the given protocol.
class ExternalProtocolDialogDelegate : public ProtocolDialogDelegate {
 public:
  explicit ExternalProtocolDialogDelegate(const GURL& url,
                                          int render_process_host_id,
                                          int render_view_routing_id);
  ~ExternalProtocolDialogDelegate() override;

  const base::string16& program_name() const { return program_name_; }

  void DoAccept(const GURL& url, bool remember) const override;

  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  base::string16 GetMessageText() const override;
  base::string16 GetCheckboxText() const override;
  base::string16 GetTitleText() const override;

 private:
  int render_process_host_id_;
  int render_view_routing_id_;
  const base::string16 program_name_;

  DISALLOW_COPY_AND_ASSIGN(ExternalProtocolDialogDelegate);
};

#endif  // CHROME_BROWSER_UI_EXTERNAL_PROTOCOL_DIALOG_DELEGATE_H_
