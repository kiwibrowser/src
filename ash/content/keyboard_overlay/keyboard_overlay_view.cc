// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/content/keyboard_overlay/keyboard_overlay_view.h"

#include "ash/content/keyboard_overlay/keyboard_overlay_delegate.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/event.h"
#include "ui/views/widget/widget.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

using ui::WebDialogDelegate;

namespace {

// Keys to invoke Cancel (Escape, Ctrl+Alt+/, or Shift+Ctrl+Alt+/, Help, F14).
const ash::KeyboardOverlayView::KeyEventData kCancelKeys[] = {
    {ui::VKEY_ESCAPE, ui::EF_NONE},
    {ui::VKEY_OEM_2, ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN},
    {ui::VKEY_OEM_2, ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN},
    {ui::VKEY_HELP, ui::EF_NONE},
    {ui::VKEY_F14, ui::EF_NONE},
};
}

namespace ash {

KeyboardOverlayView::KeyboardOverlayView(content::BrowserContext* context,
                                         WebDialogDelegate* delegate,
                                         WebContentsHandler* handler)
    : views::WebDialogView(context, delegate, handler) {}

KeyboardOverlayView::~KeyboardOverlayView() = default;

void KeyboardOverlayView::Cancel() {
  Shell::Get()->overlay_filter()->Deactivate(this);
  views::Widget* widget = GetWidget();
  if (widget)
    widget->Close();
}

bool KeyboardOverlayView::IsCancelingKeyEvent(ui::KeyEvent* event) {
  if (event->type() != ui::ET_KEY_PRESSED)
    return false;

  // Ignore the non-modifiers flags in order to treat it the same way
  // Accelerators are generated and compared. crbug.com/535008.
  const int flags = ui::Accelerator::MaskOutKeyEventFlags(event->flags());
  for (size_t i = 0; i < arraysize(kCancelKeys); ++i) {
    if ((kCancelKeys[i].key_code == event->key_code()) &&
        (kCancelKeys[i].flags == flags))
      return true;
  }
  return false;
}

aura::Window* KeyboardOverlayView::GetWindow() {
  return GetWidget()->GetNativeWindow();
}

// static
void KeyboardOverlayView::ShowDialog(content::BrowserContext* context,
                                     WebContentsHandler* handler,
                                     const GURL& url) {
  if (Shell::Get()->overlay_filter()->IsActive())
    return;

  KeyboardOverlayDelegate* delegate = new KeyboardOverlayDelegate(
      l10n_util::GetStringUTF16(IDS_ASH_KEYBOARD_OVERLAY_TITLE), url);
  KeyboardOverlayView* view =
      new KeyboardOverlayView(context, delegate, handler);
  delegate->Show(view);

  Shell::Get()->overlay_filter()->Activate(view);
}

void KeyboardOverlayView::WindowClosing() {
  Cancel();
}

// static
void KeyboardOverlayView::GetCancelingKeysForTesting(
    std::vector<KeyboardOverlayView::KeyEventData>* canceling_keys) {
  CHECK(canceling_keys);
  canceling_keys->clear();
  for (size_t i = 0; i < arraysize(kCancelKeys); ++i)
    canceling_keys->push_back(kCancelKeys[i]);
}

}  // namespace ash
