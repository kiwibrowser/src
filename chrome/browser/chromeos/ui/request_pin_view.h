// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_UI_REQUEST_PIN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_UI_REQUEST_PIN_VIEW_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class Label;
}

namespace chromeos {

// A dialog box for requesting PIN code. Instances of this class are managed by
// PinDialogManager.
class RequestPinView : public views::DialogDelegateView,
                       public views::TextfieldController {
 public:
  enum RequestPinCodeType { PIN, PUK, UNCHANGED };

  enum RequestPinErrorType {
    NONE,
    INVALID_PIN,
    INVALID_PUK,
    MAX_ATTEMPTS_EXCEEDED,
    UNKNOWN_ERROR
  };

  class Delegate {
   public:
    // Notification when user closes the PIN dialog.
    virtual void OnPinDialogClosed() = 0;

    // Notification when the user provided input to dialog.
    virtual void OnPinDialogInput() = 0;
  };

  // Used to send the PIN/PUK entered by the user in the textfield to the
  // extension that asked for the code.
  using RequestPinCallback = base::Callback<void(const base::string16&)>;

  // Creates the view to be embeded in the dialog that requests the PIN/PUK.
  // |extension_name| - the name of the extension making the request. Displayed
  //     in the title and in the header of the view.
  // |code_type| - the type of code requested, PIN or PUK. UNCHANGED is not
  //     accepted here.
  // |attempts_left| - the number of attempts user has to try the code. When
  //     zero the textfield is disabled and user cannot provide any input. When
  //     -1 the user is allowed to provide the input and no information about
  //     the attepts left is displayed in the view.
  // |callback| - used to send the value of the PIN/PUK the user entered.
  // |delegate| - used to notify that dialog was closed. Cannot be null.
  RequestPinView(const std::string& extension_name,
                 RequestPinCodeType code_type,
                 int attempts_left,
                 const RequestPinCallback& callback,
                 Delegate* delegate);
  ~RequestPinView() override;

  // views::TextfieldController
  void ContentsChanged(views::Textfield* sender,
                       const base::string16& new_contents) override;

  // views::DialogDelegateView
  bool Cancel() override;
  bool Accept() override;
  base::string16 GetWindowTitle() const override;
  views::View* GetInitiallyFocusedView() override;
  bool IsDialogButtonEnabled(ui::DialogButton button) const override;

  // views::View
  gfx::Size CalculatePreferredSize() const override;

  // Returns whether the view is locked while waiting the extension to process
  // the user input data.
  bool IsLocked();

  // Set the new callback to be used when user will provide the input. The old
  // callback must be used and reset to null at this point.
  void SetCallback(const RequestPinCallback& callback);

  // |code_type| - specifies whether the user is asked to enter PIN or PUK. If
  //     UNCHANGED value is provided, the dialog displays the same value that
  //     was last set.
  // |error_type| - the error template to be displayed in red in the dialog. If
  //     NONE, no error is displayed.
  // |attempts_left| - included in the view as the number of attepts user can
  //     have to enter correct code.
  // |accept_input| - specifies whether the textfield is enabled. If disabled
  //     the user is unable to provide input.
  void SetDialogParameters(RequestPinCodeType code_type,
                           RequestPinErrorType error_type,
                           int attempts_left,
                           bool accept_input);

  // Set the name of extension that is using this view. The name is included in
  // the header text displayed by the view.
  void SetExtensionName(const std::string& extension_name);

  views::Textfield* textfield_for_testing() { return textfield_; }
  views::Label* error_label_for_testing() { return error_label_; }

 private:
  // This initializes the view, with all the UI components.
  void Init();
  void SetAcceptInput(bool accept_input);
  void SetErrorMessage(RequestPinErrorType error_type, int attempts_left);
  // Updates the header text |header_label_| based on values from
  // |window_title_| and |code_type_|.
  void UpdateHeaderText();

  // Used to send the input when the view is not locked. If user closes the
  // view, the provided input is empty. The |callback_| must be reset to null
  // after being used, allowing to check that it was used when a new callback is
  // set.
  RequestPinCallback callback_;

  // Owned by the caller.
  Delegate* delegate_ = nullptr;

  base::string16 window_title_;
  views::Label* header_label_ = nullptr;
  base::string16 code_type_;
  views::Textfield* textfield_ = nullptr;
  views::Label* error_label_ = nullptr;

  base::WeakPtrFactory<RequestPinView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RequestPinView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_UI_REQUEST_PIN_VIEW_H_
