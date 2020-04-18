// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/extensions/extension_popup_aura.h"

#include "chrome/browser/ui/browser_dialogs.h"
#include "ui/aura/window.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/core/window_util.h"
#include "ui/wm/public/activation_client.h"

// static
ExtensionPopup* ExtensionPopup::Create(extensions::ExtensionViewHost* host,
                                       views::View* anchor_view,
                                       views::BubbleBorder::Arrow arrow,
                                       ShowAction show_action) {
  auto* popup = new ExtensionPopupAura(host, anchor_view, arrow, show_action);
  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(popup);
  gfx::NativeView native_view = widget->GetNativeView();

  wm::SetWindowVisibilityAnimationType(
      native_view, wm::WINDOW_VISIBILITY_ANIMATION_TYPE_VERTICAL);
  wm::SetWindowVisibilityAnimationVerticalPosition(native_view, -3.0f);

  wm::GetActivationClient(native_view->GetRootWindow())->AddObserver(popup);

  return popup;
}

ExtensionPopupAura::ExtensionPopupAura(extensions::ExtensionViewHost* host,
                                       views::View* anchor_view,
                                       views::BubbleBorder::Arrow arrow,
                                       ShowAction show_action)
    : ExtensionPopup(host, anchor_view, arrow, show_action) {
  chrome::RecordDialogCreation(chrome::DialogIdentifier::EXTENSION_POPUP_AURA);
}

ExtensionPopupAura::~ExtensionPopupAura() {
}

void ExtensionPopupAura::OnWidgetDestroying(views::Widget* widget) {
  ExtensionPopup::OnWidgetDestroying(widget);

  if (widget == GetWidget()) {
    auto* activation_client =
        wm::GetActivationClient(widget->GetNativeWindow()->GetRootWindow());
    // If the popup was being inspected with devtools and the browser window
    // was closed, then the root window and activation client are already
    // destroyed.
    if (activation_client)
      activation_client->RemoveObserver(this);
  }
}

void ExtensionPopupAura::OnWindowActivated(
    wm::ActivationChangeObserver::ActivationReason reason,
    aura::Window* gained_active,
    aura::Window* lost_active) {
  // Close on anchor window activation (ie. user clicked the browser window).
  // DesktopNativeWidgetAura does not trigger the expected browser widget
  // [de]activation events when activating widgets in its own root window.
  // This additional check handles those cases. See: http://crbug.com/320889
  if (gained_active == anchor_widget()->GetNativeWindow())
    CloseUnlessUnderInspection();
}
