// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_JAVASCRIPT_APP_MODAL_DIALOG_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_JAVASCRIPT_APP_MODAL_DIALOG_COCOA_H_

#include <memory>

#include "base/logging.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "components/app_modal/native_app_modal_dialog.h"

class PopunderPreventer;

namespace app_modal {
class JavaScriptAppModalDialog;
}

#if __OBJC__
@class NSAlert;
@class JavaScriptAppModalDialogHelper;
#else
class NSAlert;
class JavaScriptAppModalDialogHelper;
#endif

class JavaScriptAppModalDialogCocoa : public app_modal::NativeAppModalDialog {
 public:
  explicit JavaScriptAppModalDialogCocoa(
      app_modal::JavaScriptAppModalDialog* dialog);
  ~JavaScriptAppModalDialogCocoa() override;

  // Overridden from NativeAppModalDialog:
  int GetAppModalDialogButtons() const override;
  void ShowAppModalDialog() override;
  void ActivateAppModalDialog() override;
  void CloseAppModalDialog() override;
  void AcceptAppModalDialog() override;
  void CancelAppModalDialog() override;
  bool IsShowing() const override;

  app_modal::JavaScriptAppModalDialog* dialog() const {
    return dialog_.get();
  }

 private:
  // Returns the NSAlert associated with the modal dialog.
  NSAlert* GetAlert() const;

  std::unique_ptr<app_modal::JavaScriptAppModalDialog> dialog_;
  std::unique_ptr<PopunderPreventer> popunder_preventer_;

  // Created in the constructor and destroyed in the destructor.
  base::scoped_nsobject<JavaScriptAppModalDialogHelper> helper_;

  bool is_showing_;

  DISALLOW_COPY_AND_ASSIGN(JavaScriptAppModalDialogCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_JAVASCRIPT_APP_MODAL_DIALOG_COCOA_H_
