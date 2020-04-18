// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/gaia_dialog_delegate.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "chrome/browser/chromeos/login/ui/login_display_host_mojo.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/extensions/chrome_extension_web_contents_observer.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "chrome/browser/ui/webui/chrome_web_contents_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/controls/webview/web_dialog_view.h"
#include "ui/views/widget/widget.h"

namespace chromeos {

namespace {

constexpr char kGaiaURL[] = "chrome://oobe/gaia-signin";
constexpr int kGaiaDialogHeight = 640;
constexpr int kGaiaDialogWidth = 768;

}  // namespace

GaiaDialogDelegate::GaiaDialogDelegate(
    base::WeakPtr<LoginDisplayHostMojo> controller)
    : controller_(controller),
      size_(gfx::Size(kGaiaDialogWidth, kGaiaDialogHeight)) {}

GaiaDialogDelegate::~GaiaDialogDelegate() {
  if (controller_)
    controller_->OnDialogDestroyed(this);
}

void GaiaDialogDelegate::Init() {
  DCHECK(!dialog_view_ && !dialog_widget_);
  // Life cycle of |dialog_view_| is managed by the widget:
  // Widget owns a root view which has |dialog_view_| as its child view.
  // Before the widget is destroyed, it will clean up the view hierarchy
  // starting from root view.
  dialog_view_ = new views::WebDialogView(ProfileHelper::GetSigninProfile(),
                                          this, new ChromeWebContentsHandler);
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.delegate = dialog_view_;
  ash_util::SetupWidgetInitParamsForContainer(
      &params, ash::kShellWindowId_LockSystemModalContainer);

  dialog_widget_ = new views::Widget;
  dialog_widget_->Init(params);

  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      dialog_view_->web_contents());
}

void GaiaDialogDelegate::Show(bool closable_by_esc) {
  closable_by_esc_ = closable_by_esc;
  dialog_widget_->Show();
}

void GaiaDialogDelegate::Hide() {
  if (dialog_widget_)
    dialog_widget_->Hide();
}

void GaiaDialogDelegate::Close() {
  if (dialog_widget_)
    dialog_widget_->Close();
}

void GaiaDialogDelegate::SetSize(int width, int height) {
  if (size_ == gfx::Size(width, height))
    return;

  size_.SetSize(width, height);
  if (!dialog_widget_)
    return;

  const gfx::Rect rect =
      display::Screen::GetScreen()
          ->GetDisplayNearestWindow(dialog_widget_->GetNativeWindow())
          .work_area();

  // Place the dialog in the center of the screen.
  const gfx::Rect bounds(rect.x() + (rect.width() - size_.width()) / 2,
                         rect.y() + (rect.height() - size_.height()) / 2,
                         size_.width(), size_.height());
  dialog_widget_->SetBounds(bounds);
}

OobeUI* GaiaDialogDelegate::GetOobeUI() const {
  if (dialog_view_) {
    content::WebUI* webui = dialog_view_->web_contents()->GetWebUI();
    if (webui)
      return static_cast<OobeUI*>(webui->GetController());
  }
  return nullptr;
}

ui::ModalType GaiaDialogDelegate::GetDialogModalType() const {
  return ui::MODAL_TYPE_SYSTEM;
}

base::string16 GaiaDialogDelegate::GetDialogTitle() const {
  return base::string16();
}

GURL GaiaDialogDelegate::GetDialogContentURL() const {
  return GURL(kGaiaURL);
}

void GaiaDialogDelegate::GetWebUIMessageHandlers(
    std::vector<content::WebUIMessageHandler*>* handlers) const {}

void GaiaDialogDelegate::GetDialogSize(gfx::Size* size) const {
  *size = size_;
}

bool GaiaDialogDelegate::CanResizeDialog() const {
  return false;
}

std::string GaiaDialogDelegate::GetDialogArgs() const {
  return std::string();
}

void GaiaDialogDelegate::OnDialogClosed(const std::string& json_retval) {
  delete this;
}

void GaiaDialogDelegate::OnCloseContents(content::WebContents* source,
                                         bool* out_close_dialog) {
  *out_close_dialog = true;
}

bool GaiaDialogDelegate::ShouldShowDialogTitle() const {
  return false;
}

bool GaiaDialogDelegate::HandleContextMenu(
    const content::ContextMenuParams& params) {
  return true;
}

std::vector<ui::Accelerator> GaiaDialogDelegate::GetAccelerators() {
  // TODO(crbug.com/809648): Adding necessory accelerators.
  return std::vector<ui::Accelerator>();
}

bool GaiaDialogDelegate::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  if (ui::VKEY_ESCAPE == accelerator.key_code()) {
    // The widget should not be closed until the login is done.
    // Consume the escape key here so WebDialogView won't have a chance to
    // close the widget.
    if (closable_by_esc_)
      dialog_widget_->Hide();
    return true;
  }

  return false;
}

}  // namespace chromeos
