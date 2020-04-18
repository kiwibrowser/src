// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/mobile_setup_dialog.h"

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/login/ui/webui_login_view.h"
#include "chrome/browser/chromeos/mobile/mobile_activator.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/simple_message_box.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

using chromeos::MobileActivator;
using content::BrowserThread;
using content::WebContents;
using content::WebUIMessageHandler;
using ui::WebDialogDelegate;

class MobileSetupDialogDelegate : public WebDialogDelegate {
 public:
  static MobileSetupDialogDelegate* GetInstance();
  void ShowDialog(const std::string& service_path);

 protected:
  friend struct base::DefaultSingletonTraits<MobileSetupDialogDelegate>;

  MobileSetupDialogDelegate();
  ~MobileSetupDialogDelegate() override;

  void OnCloseDialog();

  // WebDialogDelegate overrides.
  ui::ModalType GetDialogModalType() const override;
  base::string16 GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<WebUIMessageHandler*>* handlers) const override;
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(WebContents* source, bool* out_close_dialog) override;
  bool ShouldShowDialogTitle() const override;
  bool HandleContextMenu(const content::ContextMenuParams& params) override;

 private:
  gfx::NativeWindow dialog_window_;
  // Cellular network service path.
  std::string service_path_;
  DISALLOW_COPY_AND_ASSIGN(MobileSetupDialogDelegate);
};

// static
void MobileSetupDialog::ShowByNetworkId(const std::string& network_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const chromeos::NetworkState* network =
      chromeos::NetworkHandler::Get()
          ->network_state_handler()
          ->GetNetworkStateFromGuid(network_id);
  if (!network) {
    LOG(ERROR) << "MobileSetupDialog: Network ID not found: " << network_id;
    return;
  }
  MobileSetupDialogDelegate::GetInstance()->ShowDialog(network->path());
}

// static
MobileSetupDialogDelegate* MobileSetupDialogDelegate::GetInstance() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return base::Singleton<MobileSetupDialogDelegate>::get();
}

MobileSetupDialogDelegate::MobileSetupDialogDelegate()
    : dialog_window_(nullptr) {}

MobileSetupDialogDelegate::~MobileSetupDialogDelegate() {
}

void MobileSetupDialogDelegate::ShowDialog(const std::string& service_path) {
  service_path_ = service_path;

  gfx::NativeWindow parent = nullptr;
  // If we're on the login screen.
  if (chromeos::LoginDisplayHost::default_host()) {
    chromeos::WebUILoginView* login_view =
        chromeos::LoginDisplayHost::default_host()->GetWebUILoginView();
    if (login_view)
      parent = login_view->GetNativeWindow();
  }
  // Only the primary user can change this.
  dialog_window_ = chrome::ShowWebDialog(
      parent,
      ProfileManager::GetPrimaryUserProfile(),
      this);
}

ui::ModalType MobileSetupDialogDelegate::GetDialogModalType() const {
  return ui::MODAL_TYPE_SYSTEM;
}

base::string16 MobileSetupDialogDelegate::GetDialogTitle() const {
  return l10n_util::GetStringUTF16(IDS_MOBILE_SETUP_TITLE);
}

GURL MobileSetupDialogDelegate::GetDialogContentURL() const {
  std::string url(chrome::kChromeUIMobileSetupURL);
  url.append(service_path_);
  return GURL(url);
}

void MobileSetupDialogDelegate::GetWebUIMessageHandlers(
    std::vector<WebUIMessageHandler*>* handlers) const {
}

void MobileSetupDialogDelegate::GetDialogSize(gfx::Size* size) const {
  size->SetSize(850, 650);
}

std::string MobileSetupDialogDelegate::GetDialogArgs() const {
  return std::string();
}

void MobileSetupDialogDelegate::OnDialogClosed(const std::string& json_retval) {
  dialog_window_ = nullptr;
}

void MobileSetupDialogDelegate::OnCloseContents(WebContents* source,
                                                bool* out_close_dialog) {
  // If we're exiting, popping up the confirmation dialog can cause a
  // crash. Note: IsTryingToQuit can be cancelled on other platforms by the
  // onbeforeunload handler, except on ChromeOS. So IsTryingToQuit is the
  // appropriate check to use here.
  if (!dialog_window_ ||
      !MobileActivator::GetInstance()->RunningActivation() ||
      browser_shutdown::IsTryingToQuit()) {
    *out_close_dialog = true;
    return;
  }

  *out_close_dialog = chrome::ShowQuestionMessageBox(
      dialog_window_, l10n_util::GetStringUTF16(IDS_MOBILE_SETUP_TITLE),
      l10n_util::GetStringUTF16(IDS_MOBILE_CANCEL_ACTIVATION));
}

bool MobileSetupDialogDelegate::ShouldShowDialogTitle() const {
  return true;
}

bool MobileSetupDialogDelegate::HandleContextMenu(
    const content::ContextMenuParams& params) {
  return true;
}
