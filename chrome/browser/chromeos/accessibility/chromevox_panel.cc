// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/accessibility/chromevox_panel.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/interfaces/accessibility_controller.mojom.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/data_use_measurement/data_use_web_contents_observer.h"
#include "chrome/browser/extensions/chrome_extension_web_contents_observer.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/service_manager_connection.h"
#include "extensions/browser/view_type_utils.h"
#include "extensions/common/constants.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/window_animations.h"

namespace {

const char kChromeVoxPanelRelativeUrl[] = "/cvox2/background/panel.html";
const char kFullscreenURLFragment[] = "fullscreen";
const char kDisableSpokenFeedbackURLFragment[] = "close";
const char kFocusURLFragment[] = "focus";

}  // namespace

class ChromeVoxPanel::ChromeVoxPanelWebContentsObserver
    : public content::WebContentsObserver {
 public:
  ChromeVoxPanelWebContentsObserver(content::WebContents* web_contents,
                                    ChromeVoxPanel* panel)
      : content::WebContentsObserver(web_contents), panel_(panel) {}
  ~ChromeVoxPanelWebContentsObserver() override {}

  // content::WebContentsObserver overrides.
  void DidFirstVisuallyNonEmptyPaint() override {
    panel_->DidFirstVisuallyNonEmptyPaint();
  }

  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    // The ChromeVox panel uses the URL fragment to communicate state
    // to this panel host.
    std::string fragment = web_contents()->GetLastCommittedURL().ref();
    if (fragment == kDisableSpokenFeedbackURLFragment)
      chromeos::AccessibilityManager::Get()->EnableSpokenFeedback(false);
    else if (fragment == kFullscreenURLFragment)
      panel_->EnterFullscreen();
    else if (fragment == kFocusURLFragment)
      panel_->Focus();
    else
      panel_->ExitFullscreen();
  }

 private:
  ChromeVoxPanel* panel_;

  DISALLOW_COPY_AND_ASSIGN(ChromeVoxPanelWebContentsObserver);
};

ChromeVoxPanel::ChromeVoxPanel(content::BrowserContext* browser_context)
    : widget_(nullptr), web_view_(nullptr) {
  std::string url("chrome-extension://");
  url += extension_misc::kChromeVoxExtensionId;
  url += kChromeVoxPanelRelativeUrl;

  views::WebView* web_view = new views::WebView(browser_context);
  content::WebContents* contents = web_view->GetWebContents();
  web_contents_observer_.reset(
      new ChromeVoxPanelWebContentsObserver(contents, this));
  data_use_measurement::DataUseWebContentsObserver::CreateForWebContents(
      contents);
  contents->SetDelegate(this);
  extensions::SetViewType(contents, extensions::VIEW_TYPE_COMPONENT);
  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      contents);
  web_view->LoadInitialURL(GURL(url));
  web_view_ = web_view;

  widget_ = new views::Widget();
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  // Placing the panel in the accessibility panel container allows ash to manage
  // both the window bounds and display work area.
  ash_util::SetupWidgetInitParamsForContainer(
      &params, ash::kShellWindowId_AccessibilityPanelContainer);
  params.bounds = display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  params.delegate = this;
  params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  params.name = "ChromeVoxPanel";
  widget_->Init(params);
  wm::SetShadowElevation(widget_->GetNativeWindow(),
                         wm::kShadowElevationInactiveWindow);

  // WebContentsObserver::DidFirstVisuallyNonEmptyPaint is not called under
  // mash. Work around this by showing the window immediately.
  // TODO(jamescook|fsamuel): Fix this. It causes a white flash when opening the
  // window. The underlying problem is FrameToken plumbing, see
  // ui::ws::ServerWindow::OnFrameTokenChanged. https://crbug.com/771331
  if (chromeos::GetAshConfig() == ash::Config::MASH)
    widget_->Show();
}

ChromeVoxPanel::~ChromeVoxPanel() = default;

aura::Window* ChromeVoxPanel::GetRootWindow() {
  return GetWidget()->GetNativeWindow()->GetRootWindow();
}

void ChromeVoxPanel::CloseNow() {
  widget_->CloseNow();
}

void ChromeVoxPanel::Close() {
  // NOTE: Close the widget asynchronously because it's not legal to delete
  // a WebView/WebContents during a DidFinishNavigation callback.
  widget_->Close();
}

const views::Widget* ChromeVoxPanel::GetWidget() const {
  return widget_;
}

views::Widget* ChromeVoxPanel::GetWidget() {
  return widget_;
}

void ChromeVoxPanel::DeleteDelegate() {
  delete this;
}

views::View* ChromeVoxPanel::GetContentsView() {
  return web_view_;
}

bool ChromeVoxPanel::HandleContextMenu(
    const content::ContextMenuParams& params) {
  // Eat all requests as context menus are disallowed.
  return true;
}

void ChromeVoxPanel::DidFirstVisuallyNonEmptyPaint() {
  widget_->Show();
}

void ChromeVoxPanel::EnterFullscreen() {
  Focus();
  SetAccessibilityPanelFullscreen(true);
}

void ChromeVoxPanel::ExitFullscreen() {
  widget_->Deactivate();
  widget_->widget_delegate()->set_can_activate(false);
  SetAccessibilityPanelFullscreen(false);
}

void ChromeVoxPanel::Focus() {
  widget_->widget_delegate()->set_can_activate(true);
  widget_->Activate();
  web_view_->RequestFocus();
}

void ChromeVoxPanel::SetAccessibilityPanelFullscreen(bool fullscreen) {
  // Connect to the accessibility mojo interface in ash.
  ash::mojom::AccessibilityControllerPtr accessibility_controller;
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &accessibility_controller);
  accessibility_controller->SetAccessibilityPanelFullscreen(fullscreen);
}
