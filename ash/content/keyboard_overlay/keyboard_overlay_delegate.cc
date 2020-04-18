// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/content/keyboard_overlay/keyboard_overlay_delegate.h"

#include <algorithm>

#include "ash/shell.h"
#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/controls/webview/web_dialog_view.h"
#include "ui/views/widget/widget.h"

using content::WebContents;
using content::WebUIMessageHandler;

namespace {

const int kBaseWidth = 1252;
const int kBaseHeight = 516;
const int kHorizontalMargin = 28;

// A message handler for detecting the timing when the web contents is painted.
class PaintMessageHandler : public WebUIMessageHandler,
                            public base::SupportsWeakPtr<PaintMessageHandler> {
 public:
  explicit PaintMessageHandler(views::Widget* widget) : widget_(widget) {}
  ~PaintMessageHandler() override = default;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void DidPaint(const base::ListValue* args);

  views::Widget* widget_;

  DISALLOW_COPY_AND_ASSIGN(PaintMessageHandler);
};

void PaintMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "didPaint",
      base::BindRepeating(&PaintMessageHandler::DidPaint, AsWeakPtr()));
}

void PaintMessageHandler::DidPaint(const base::ListValue* args) {
  // Show the widget after the web content has been painted.
  widget_->Show();
  web_ui()->CallJavascriptFunctionUnsafe("onWidgetShown");
}

}  // namespace

namespace ash {

KeyboardOverlayDelegate::KeyboardOverlayDelegate(const base::string16& title,
                                                 const GURL& url)
    : title_(title), url_(url), widget_(NULL) {}

KeyboardOverlayDelegate::~KeyboardOverlayDelegate() = default;

views::Widget* KeyboardOverlayDelegate::Show(views::WebDialogView* view) {
  widget_ = new views::Widget;
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.context = Shell::GetPrimaryRootWindow();
  params.delegate = view;
  widget_->Init(params);

  // Show the widget at the bottom of the work area.
  gfx::Size size;
  GetDialogSize(&size);
  const gfx::Rect rect =
      display::Screen::GetScreen()
          ->GetDisplayNearestWindow(widget_->GetNativeWindow())
          .work_area();
  gfx::Rect bounds(rect.x() + (rect.width() - size.width()) / 2,
                   rect.y() + (rect.height() - size.height()) / 2, size.width(),
                   size.height());
  widget_->SetBounds(bounds);

  // The widget will be shown when the web contents gets ready to display.
  return widget_;
}

ui::ModalType KeyboardOverlayDelegate::GetDialogModalType() const {
  return ui::MODAL_TYPE_SYSTEM;
}

base::string16 KeyboardOverlayDelegate::GetDialogTitle() const {
  return title_;
}

GURL KeyboardOverlayDelegate::GetDialogContentURL() const {
  return url_;
}

void KeyboardOverlayDelegate::GetWebUIMessageHandlers(
    std::vector<WebUIMessageHandler*>* handlers) const {
  handlers->push_back(new PaintMessageHandler(widget_));
}

void KeyboardOverlayDelegate::GetDialogSize(gfx::Size* size) const {
  using std::min;
  DCHECK(widget_);
  gfx::Rect rect = display::Screen::GetScreen()
                       ->GetDisplayNearestWindow(widget_->GetNativeWindow())
                       .work_area();
  const int width = min(kBaseWidth, rect.width() - kHorizontalMargin);
  const int height = width * kBaseHeight / kBaseWidth;
  size->SetSize(width, height);
}

std::string KeyboardOverlayDelegate::GetDialogArgs() const {
  return "[]";
}

void KeyboardOverlayDelegate::OnDialogClosed(const std::string& json_retval) {
  delete this;
  return;
}

void KeyboardOverlayDelegate::OnCloseContents(WebContents* source,
                                              bool* out_close_dialog) {}

bool KeyboardOverlayDelegate::ShouldShowDialogTitle() const {
  return false;
}

bool KeyboardOverlayDelegate::HandleContextMenu(
    const content::ContextMenuParams& params) {
  return true;
}

}  // namespace ash
