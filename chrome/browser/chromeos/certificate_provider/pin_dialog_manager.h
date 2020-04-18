// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CERTIFICATE_PROVIDER_PIN_DIALOG_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_CERTIFICATE_PROVIDER_PIN_DIALOG_MANAGER_H_

#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/ui/request_pin_view.h"
#include "ui/views/widget/widget.h"

namespace chromeos {

// Manages the state of the dialog that requests the PIN from user. Used by the
// extensions that need to request the PIN. Implemented as requirement for
// crbug.com/612886
class PinDialogManager : RequestPinView::Delegate {
 public:
  enum RequestPinResponse {
    SUCCESS,
    INVALID_ID,
    OTHER_FLOW_IN_PROGRESS,
    DIALOG_DISPLAYED_ALREADY
  };

  enum StopPinRequestResponse { STOPPED, NO_ACTIVE_DIALOG, NO_USER_INPUT };

  PinDialogManager();
  ~PinDialogManager();

  // Stores internally the |signRequestId| along with current timestamp.
  void AddSignRequestId(const std::string& extension_id, int sign_request_id);

  // Creates a new RequestPinView object and displays it in a dialog or reuses
  // the old dialog if active one exists just updating the parameters.
  // |extension_id| - the ID of the extension requesting the dialog.
  // |extension_name| - the name of the extension requesting the dialog.
  // |sign_request_id| - the ID given by Chrome when the extension was asked to
  //     sign the data. It should be a valid, not expired ID at the time the
  //     extension is requesting PIN the first time.
  // |code_type| - the type of input requested: either "PIN" or "PUK".
  // |error_type| - the error template to be displayed inside the dialog. If
  //     NONE, no error is displayed.
  // |attempts_left| - the number of attempts the user has to try the code. It
  //     is informational only, and enforced on Chrome side only in case it's
  //     zero. In that case the textfield is disabled and the user can't provide
  //     any input to extension. If -1 the textfield from the dialog is enabled
  //     but no information about the attepts left is not given to the user.
  // |callback| - used to notify about the user input in the text_field from the
  //     dialog.
  // Returns SUCCESS if the dialog is displayed and extension owns it. Otherwise
  // the specific error is returned.
  RequestPinResponse ShowPinDialog(
      const std::string& extension_id,
      const std::string& extension_name,
      int sign_request_id,
      RequestPinView::RequestPinCodeType code_type,
      RequestPinView::RequestPinErrorType error_type,
      int attempts_left,
      const RequestPinView::RequestPinCallback& callback);

  // chromeos::RequestPinView::Delegate overrides.
  void OnPinDialogInput() override;
  void OnPinDialogClosed() override;

  // Updates the existing dialog with new error message. Uses |callback| with
  // empty string when user closes the dialog. Returns whether the provided
  // |extension_id| matches the extension owning the active dialog.
  PinDialogManager::StopPinRequestResponse UpdatePinDialog(
      const std::string& extension_id,
      RequestPinView::RequestPinErrorType error_type,
      bool accept_input,
      const RequestPinView::RequestPinCallback& callback);

  // Returns whether the last PIN dialog from this extension was closed by the
  // user.
  bool LastPinDialogClosed(const std::string& extension_id);

  // Called when extension calls the stopPinRequest method. The active dialog is
  // closed if the |extension_id| matches the |active_dialog_extension_id_|.
  // Returns whether the dialog was closed.
  bool CloseDialog(const std::string& extension_id);

  // Resets the manager data related to the extension.
  void ExtensionUnloaded(const std::string& extension_id);

  RequestPinView* active_view_for_testing() { return active_pin_dialog_; }
  views::Widget* active_window_for_testing() { return active_window_; }

 private:
  using ExtensionNameRequestIdPair = std::pair<std::string, int>;

  // Tells whether user closed the last request PIN dialog issued by an
  // extension. The extension_id is the key and value is true if user closed the
  // dialog. Used to determine if the limit of dialogs rejected by the user has
  // been exceeded.
  std::unordered_map<std::string, bool> last_response_closed_;

  // The map with extension_id and sign request id issued by Chrome as key while
  // the time when the id was generated is the value.
  std::map<ExtensionNameRequestIdPair, base::Time> sign_request_times_;

  // There can be only one active dialog to request the PIN at some point in
  // time. Owned by |active_window_|.
  RequestPinView* active_pin_dialog_ = nullptr;
  std::string active_dialog_extension_id_;
  views::Widget* active_window_ = nullptr;

  base::WeakPtrFactory<PinDialogManager> weak_factory_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CERTIFICATE_PROVIDER_PIN_DIALOG_MANAGER_H_
