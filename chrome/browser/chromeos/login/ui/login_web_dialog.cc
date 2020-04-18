// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/login_web_dialog.h"

#include "base/containers/circular_deque.h"
#include "base/lazy_instance.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/ui/ash/system_tray_client.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_finder.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget.h"

using content::WebContents;
using content::WebUIMessageHandler;

namespace chromeos {

namespace {

// Default width/height ratio of screen size.
const double kDefaultWidthRatio = 0.6;
const double kDefaultHeightRatio = 0.6;

// Default width/height ratio of minimal dialog size.
const double kMinimumWidthRatio = 0.25;
const double kMinimumHeightRatio = 0.25;

base::LazyInstance<base::circular_deque<WebContents*>>::DestructorAtExit
    g_web_contents_stack = LAZY_INSTANCE_INITIALIZER;

// Returns the accelerator which is mapped as hangup button on Chrome OS CFM
// remote controller to close the dialog.
ui::Accelerator GetCloseAccelerator() {
  return ui::Accelerator(ui::VKEY_BROWSER_BACK, ui::EF_SHIFT_DOWN);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// LoginWebDialog, public:

void LoginWebDialog::Delegate::OnDialogClosed() {}

LoginWebDialog::LoginWebDialog(content::BrowserContext* browser_context,
                               Delegate* delegate,
                               gfx::NativeWindow parent_window,
                               const base::string16& title,
                               const GURL& url)
    : browser_context_(browser_context),
      parent_window_(parent_window),
      delegate_(delegate),
      title_(title),
      url_(url) {
  gfx::Rect screen_bounds(CalculateScreenBounds(gfx::Size()));
  width_ = static_cast<int>(kDefaultWidthRatio * screen_bounds.width());
  height_ = static_cast<int>(kDefaultHeightRatio * screen_bounds.height());
}

LoginWebDialog::~LoginWebDialog() {}

void LoginWebDialog::Show() {
  dialog_window_ = nullptr;
  if (parent_window_) {
    dialog_window_ =
        chrome::ShowWebDialog(parent_window_, browser_context_, this);
  } else {
    dialog_window_ = chrome::ShowWebDialogInContainer(
        SystemTrayClient::GetDialogParentContainerId(), browser_context_, this);
  }
}

void LoginWebDialog::SetDialogSize(int width, int height) {
  DCHECK_GE(width, 0);
  DCHECK_GE(height, 0);
  width_ = width;
  height_ = height;
}

void LoginWebDialog::SetDialogTitle(const base::string16& title) {
  title_ = title;
}

///////////////////////////////////////////////////////////////////////////////
// LoginWebDialog, protected:

ui::ModalType LoginWebDialog::GetDialogModalType() const {
  return ui::MODAL_TYPE_SYSTEM;
}

base::string16 LoginWebDialog::GetDialogTitle() const {
  return title_;
}

GURL LoginWebDialog::GetDialogContentURL() const {
  return url_;
}

void LoginWebDialog::GetWebUIMessageHandlers(
    std::vector<WebUIMessageHandler*>* handlers) const {}

void LoginWebDialog::GetDialogSize(gfx::Size* size) const {
  size->SetSize(width_, height_);
}

void LoginWebDialog::GetMinimumDialogSize(gfx::Size* size) const {
  gfx::Rect screen_bounds(CalculateScreenBounds(gfx::Size()));
  size->SetSize(kMinimumWidthRatio * screen_bounds.width(),
                kMinimumHeightRatio * screen_bounds.height());
}

std::string LoginWebDialog::GetDialogArgs() const {
  return std::string();
}

// static.
WebContents* LoginWebDialog::GetCurrentWebContents() {
  auto& stack = g_web_contents_stack.Get();
  return stack.empty() ? nullptr : stack.front();
}

void LoginWebDialog::OnDialogShown(content::WebUI* webui,
                                   content::RenderViewHost* render_view_host) {
  g_web_contents_stack.Pointer()->push_front(webui->GetWebContents());
}

void LoginWebDialog::OnDialogClosed(const std::string& json_retval) {
  dialog_window_ = nullptr;
  if (delegate_)
    delegate_->OnDialogClosed();
  delete this;
}

void LoginWebDialog::OnCloseContents(WebContents* source,
                                     bool* out_close_dialog) {
  *out_close_dialog = true;

  if (GetCurrentWebContents() == source)
    g_web_contents_stack.Pointer()->pop_front();
  // Else: TODO(pkotwicz): Investigate if the else case should ever be hit.
  // http://crbug.com/419837
}

bool LoginWebDialog::ShouldShowDialogTitle() const {
  return true;
}

bool LoginWebDialog::HandleContextMenu(
    const content::ContextMenuParams& params) {
  // Disable context menu.
  return true;
}

bool LoginWebDialog::HandleOpenURLFromTab(WebContents* source,
                                          const content::OpenURLParams& params,
                                          WebContents** out_new_contents) {
  // On a login screen, if a missing extension is trying to show in a web
  // dialog, a NetErrorHelper is displayed instead (hence we have a |source|),
  // but there is no browser window associated with it. A helper screen will
  // fire an auto-reload, which in turn leads to opening a new browser window,
  // so we must suppress it.
  // http://crbug.com/443096
  return (source && !chrome::FindBrowserWithWebContents(source));
}

bool LoginWebDialog::HandleShouldCreateWebContents() {
  return false;
}

std::vector<ui::Accelerator> LoginWebDialog::GetAccelerators() {
  return {GetCloseAccelerator()};
}

bool LoginWebDialog::AcceleratorPressed(const ui::Accelerator& accelerator) {
  if (!dialog_window_)
    return false;

  if (GetCloseAccelerator() == accelerator) {
    views::Widget::GetWidgetForNativeWindow(dialog_window_)->Close();
    return true;
  }

  return false;
}

}  // namespace chromeos
