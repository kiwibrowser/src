// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/link.h"

#include "build/build_config.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/font_list.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/native_cursor.h"
#include "ui/views/style/platform_style.h"

namespace views {

const char Link::kViewClassName[] = "Link";
constexpr int Link::kFocusBorderPadding;

Link::Link(const base::string16& title, int text_context, int text_style)
    : Label(title, text_context, text_style),
      requested_enabled_color_(gfx::kPlaceholderColor),
      requested_enabled_color_set_(false) {
  Init();
}

Link::~Link() {
}

// static
Link::FocusStyle Link::GetDefaultFocusStyle() {
  return ui::MaterialDesignController::IsSecondaryUiMaterial()
             ? FocusStyle::UNDERLINE
             : FocusStyle::RING;
}

Link::FocusStyle Link::GetFocusStyle() const {
  // Use the default, unless the link would "always" be underlined.
  if (underline_ && GetDefaultFocusStyle() == FocusStyle::UNDERLINE)
    return FocusStyle::RING;

  return GetDefaultFocusStyle();
}

void Link::PaintFocusRing(gfx::Canvas* canvas) const {
  if (GetFocusStyle() == FocusStyle::RING)
    canvas->DrawFocusRect(GetFocusRingBounds());
}

gfx::Insets Link::GetInsets() const {
  gfx::Insets insets = Label::GetInsets();
  if (GetFocusStyle() == FocusStyle::RING &&
      focus_behavior() != FocusBehavior::NEVER) {
    DCHECK(!text().empty());
    insets += gfx::Insets(kFocusBorderPadding);
  }
  return insets;
}

const char* Link::GetClassName() const {
  return kViewClassName;
}

gfx::NativeCursor Link::GetCursor(const ui::MouseEvent& event) {
  if (!enabled())
    return gfx::kNullCursor;
  return GetNativeHandCursor();
}

bool Link::CanProcessEventsWithinSubtree() const {
  // Links need to be able to accept events (e.g., clicking) even though
  // in general Labels do not.
  return View::CanProcessEventsWithinSubtree();
}

bool Link::OnMousePressed(const ui::MouseEvent& event) {
  if (!enabled() ||
      (!event.IsLeftMouseButton() && !event.IsMiddleMouseButton()))
    return false;
  SetPressed(true);
  return true;
}

bool Link::OnMouseDragged(const ui::MouseEvent& event) {
  SetPressed(enabled() &&
             (event.IsLeftMouseButton() || event.IsMiddleMouseButton()) &&
             HitTestPoint(event.location()));
  return true;
}

void Link::OnMouseReleased(const ui::MouseEvent& event) {
  // Change the highlight first just in case this instance is deleted
  // while calling the controller
  OnMouseCaptureLost();
  if (enabled() &&
      (event.IsLeftMouseButton() || event.IsMiddleMouseButton()) &&
      HitTestPoint(event.location())) {
    // Focus the link on click.
    RequestFocus();

    if (listener_)
      listener_->LinkClicked(this, event.flags());
  }
}

void Link::OnMouseCaptureLost() {
  SetPressed(false);
}

bool Link::OnKeyPressed(const ui::KeyEvent& event) {
  bool activate = (((event.key_code() == ui::VKEY_SPACE) &&
                    (event.flags() & ui::EF_ALT_DOWN) == 0) ||
                   (event.key_code() == ui::VKEY_RETURN &&
                    PlatformStyle::kReturnClicksFocusedControl));
  if (!activate)
    return false;

  SetPressed(false);

  // Focus the link on key pressed.
  RequestFocus();

  if (listener_)
    listener_->LinkClicked(this, event.flags());

  return true;
}

void Link::OnGestureEvent(ui::GestureEvent* event) {
  if (!enabled())
    return;

  if (event->type() == ui::ET_GESTURE_TAP_DOWN) {
    SetPressed(true);
  } else if (event->type() == ui::ET_GESTURE_TAP) {
    RequestFocus();
    if (listener_)
      listener_->LinkClicked(this, event->flags());
  } else {
    SetPressed(false);
    return;
  }
  event->SetHandled();
}

bool Link::SkipDefaultKeyEventProcessing(const ui::KeyEvent& event) {
  // Don't process Space and Return (depending on the platform) as an
  // accelerator.
  return event.key_code() == ui::VKEY_SPACE ||
         (event.key_code() == ui::VKEY_RETURN &&
          PlatformStyle::kReturnClicksFocusedControl);
}

void Link::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  Label::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kLink;
}

void Link::OnEnabledChanged() {
  RecalculateFont();
  View::OnEnabledChanged();  // Jump over Label.
}

void Link::OnFocus() {
  Label::OnFocus();
  RecalculateFont();
  // We render differently focused.
  SchedulePaint();
}

void Link::OnBlur() {
  Label::OnBlur();
  RecalculateFont();
  // We render differently focused.
  SchedulePaint();
}

void Link::SetFontList(const gfx::FontList& font_list) {
  Label::SetFontList(font_list);
  RecalculateFont();
}

void Link::SetText(const base::string16& text) {
  Label::SetText(text);
  ConfigureFocus();
}

void Link::OnNativeThemeChanged(const ui::NativeTheme* theme) {
  Label::OnNativeThemeChanged(theme);
  Label::SetEnabledColor(GetColor());
}

void Link::SetEnabledColor(SkColor color) {
  requested_enabled_color_set_ = true;
  requested_enabled_color_ = color;
  Label::SetEnabledColor(GetColor());
}

bool Link::IsSelectionSupported() const {
  return false;
}

void Link::SetUnderline(bool underline) {
  if (underline_ == underline)
    return;
  underline_ = underline;
  RecalculateFont();
}

void Link::Init() {
  listener_ = NULL;
  pressed_ = false;
  underline_ = GetDefaultFocusStyle() != FocusStyle::UNDERLINE;
  RecalculateFont();

  // Label::Init() calls SetText(), but if that's being called from Label(), our
  // SetText() override will not be reached (because the constructed class is
  // only a Label at the moment, not yet a Link).  So explicitly configure focus
  // here.
  ConfigureFocus();
}

void Link::SetPressed(bool pressed) {
  if (pressed_ != pressed) {
    pressed_ = pressed;
    Label::SetEnabledColor(GetColor());
    RecalculateFont();
    SchedulePaint();
  }
}

void Link::RecalculateFont() {
  // Underline the link if it is enabled and |underline_| is true. Also
  // underline to indicate focus when that's the style.
  const int style = font_list().GetFontStyle();
  const bool underline =
      underline_ || (HasFocus() && GetFocusStyle() == FocusStyle::UNDERLINE);
  const int intended_style = (enabled() && underline) ?
      (style | gfx::Font::UNDERLINE) : (style & ~gfx::Font::UNDERLINE);

  if (style != intended_style)
    Label::SetFontList(font_list().DeriveWithStyle(intended_style));
}

void Link::ConfigureFocus() {
  // Disable focusability for empty links.
  if (text().empty()) {
    SetFocusBehavior(FocusBehavior::NEVER);
  } else {
#if defined(OS_MACOSX)
    SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
#else
    SetFocusBehavior(FocusBehavior::ALWAYS);
#endif
  }
}

SkColor Link::GetColor() {
  // TODO(tapted): Use style::GetColor().
  const ui::NativeTheme* theme = GetNativeTheme();
  DCHECK(theme);
  if (!enabled())
    return theme->GetSystemColor(ui::NativeTheme::kColorId_LinkDisabled);

  if (requested_enabled_color_set_)
    return requested_enabled_color_;

  return GetNativeTheme()->GetSystemColor(
      pressed_ ? ui::NativeTheme::kColorId_LinkPressed
               : ui::NativeTheme::kColorId_LinkEnabled);
}

}  // namespace views
