// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/certificate_provider/pin_dialog_manager.h"

#include "base/strings/string16.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "ui/aura/window.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/window/dialog_delegate.h"

namespace {

gfx::NativeWindow GetBrowserParentWindow() {
  if (chromeos::LoginDisplayHost::default_host()) {
    return chromeos::LoginDisplayHost::default_host()->GetNativeWindow();
  }

  Browser* browser =
      chrome::FindTabbedBrowser(ProfileManager::GetPrimaryUserProfile(), true);
  if (browser) {
    return browser->window()->GetNativeWindow();
  }

  return nullptr;
}

}  // namespace

namespace chromeos {

// Define timeout for issued sign_request_id.
const int SIGN_REQUEST_ID_TIMEOUT_MINS = 10;

PinDialogManager::PinDialogManager() : weak_factory_(this) {}

PinDialogManager::~PinDialogManager() {
  // Close the active dialog if present to avoid leaking callbacks.
  if (active_pin_dialog_) {
    CloseDialog(active_dialog_extension_id_);
  }
}

void PinDialogManager::AddSignRequestId(const std::string& extension_id,
                                        int sign_request_id) {
  ExtensionNameRequestIdPair key(extension_id, sign_request_id);
  // Cache the ID with current timestamp.
  base::Time current_time = base::Time::Now();
  sign_request_times_[key] = current_time;
}

PinDialogManager::RequestPinResponse PinDialogManager::ShowPinDialog(
    const std::string& extension_id,
    const std::string& extension_name,
    int sign_request_id,
    RequestPinView::RequestPinCodeType code_type,
    RequestPinView::RequestPinErrorType error_type,
    int attempts_left,
    const RequestPinView::RequestPinCallback& callback) {
  bool accept_input = (attempts_left != 0);
  // If active dialog exists already, we need to make sure it belongs to the
  // same extension and the user submitted some input.
  if (active_pin_dialog_ != nullptr) {
    DCHECK(!active_dialog_extension_id_.empty());
    if (extension_id != active_dialog_extension_id_) {
      return OTHER_FLOW_IN_PROGRESS;
    }

    // Extension requests a PIN without having received any input from its
    // previous request. Reject the new request.
    if (!active_pin_dialog_->IsLocked()) {
      return DIALOG_DISPLAYED_ALREADY;
    }

    // Set the new callback to be used by the view.
    active_pin_dialog_->SetCallback(callback);
    active_pin_dialog_->SetDialogParameters(code_type, error_type,
                                            attempts_left, accept_input);
    active_pin_dialog_->DialogModelChanged();
    return SUCCESS;
  }

  // Check the validity of sign_request_id
  ExtensionNameRequestIdPair key(extension_id, sign_request_id);
  if (sign_request_times_.find(key) == sign_request_times_.end()) {
    return INVALID_ID;
  }

  base::Time current_time = base::Time::Now();
  if ((current_time - sign_request_times_[key]).InMinutes() >
      SIGN_REQUEST_ID_TIMEOUT_MINS) {
    return INVALID_ID;
  }

  active_dialog_extension_id_ = extension_id;
  active_pin_dialog_ = new RequestPinView(extension_name, code_type,
                                          attempts_left, callback, this);

  gfx::NativeWindow parent = GetBrowserParentWindow();
  // If there is no parent, falls back to the root window for new windows.
  active_window_ = views::DialogDelegate::CreateDialogWidget(
      active_pin_dialog_, /*context=*/ nullptr, parent);
  active_window_->Show();

  return SUCCESS;
}

void PinDialogManager::OnPinDialogInput() {
  last_response_closed_[active_dialog_extension_id_] = false;
}

void PinDialogManager::OnPinDialogClosed() {
  last_response_closed_[active_dialog_extension_id_] = true;
  // |active_pin_dialog_| is managed by |active_window_|. This local copy of
  // the pointer is reset here to allow a new dialog to be created when a new
  // request comes.
  active_pin_dialog_ = nullptr;
}

PinDialogManager::StopPinRequestResponse PinDialogManager::UpdatePinDialog(
    const std::string& extension_id,
    RequestPinView::RequestPinErrorType error_type,
    bool accept_input,
    const RequestPinView::RequestPinCallback& callback) {
  if (active_pin_dialog_ == nullptr ||
      extension_id != active_dialog_extension_id_) {
    return NO_ACTIVE_DIALOG;
  }

  if (!active_pin_dialog_->IsLocked()) {
    return NO_USER_INPUT;
  }

  active_pin_dialog_->SetCallback(callback);
  active_pin_dialog_->SetDialogParameters(
      RequestPinView::RequestPinCodeType::UNCHANGED, error_type, -1,
      accept_input);
  active_pin_dialog_->DialogModelChanged();
  return STOPPED;
}

bool PinDialogManager::LastPinDialogClosed(const std::string& extension_id) {
  return last_response_closed_[extension_id];
}

bool PinDialogManager::CloseDialog(const std::string& extension_id) {
  if (extension_id != active_dialog_extension_id_ ||
      active_pin_dialog_ == nullptr) {
    LOG(ERROR) << "StopPinRequest called by unexpected extension: "
               << extension_id;
    return false;
  }

  // Close the window. |active_pin_dialog_| gets deleted inside Close().
  active_window_->Close();
  active_pin_dialog_ = nullptr;

  return true;
}

void PinDialogManager::ExtensionUnloaded(const std::string& extension_id) {
  if (active_pin_dialog_ && active_dialog_extension_id_ == extension_id) {
    CloseDialog(extension_id);
  }

  last_response_closed_[extension_id] = false;

  for (auto it = sign_request_times_.cbegin();
       it != sign_request_times_.cend();) {
    if (it->first.first == extension_id) {
      sign_request_times_.erase(it++);
    } else {
      ++it;
    }
  }
}

}  // namespace chromeos
