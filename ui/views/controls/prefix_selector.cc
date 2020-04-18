// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/prefix_selector.h"

#include "base/i18n/case_conversion.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/gfx/range/range.h"
#include "ui/views/controls/prefix_delegate.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace views {

namespace {

const int64_t kTimeBeforeClearingMS = 1000;

}  // namespace

PrefixSelector::PrefixSelector(PrefixDelegate* delegate, View* host_view)
    : prefix_delegate_(delegate), host_view_(host_view) {
}

PrefixSelector::~PrefixSelector() {
}

void PrefixSelector::OnViewBlur() {
  ClearText();
}

void PrefixSelector::SetCompositionText(
    const ui::CompositionText& composition) {
}

void PrefixSelector::ConfirmCompositionText() {
}

void PrefixSelector::ClearCompositionText() {
}

void PrefixSelector::InsertText(const base::string16& text) {
  OnTextInput(text);
}

void PrefixSelector::InsertChar(const ui::KeyEvent& event) {
  OnTextInput(base::string16(1, event.GetCharacter()));
}

ui::TextInputType PrefixSelector::GetTextInputType() const {
  return ui::TEXT_INPUT_TYPE_TEXT;
}

ui::TextInputMode PrefixSelector::GetTextInputMode() const {
  return ui::TEXT_INPUT_MODE_DEFAULT;
}

base::i18n::TextDirection PrefixSelector::GetTextDirection() const {
  return base::i18n::UNKNOWN_DIRECTION;
}

int PrefixSelector::GetTextInputFlags() const {
  return 0;
}

bool PrefixSelector::CanComposeInline() const {
  return false;
}

gfx::Rect PrefixSelector::GetCaretBounds() const {
  gfx::Rect rect(host_view_->GetVisibleBounds().origin(), gfx::Size());
  // TextInputClient::GetCaretBounds is expected to return a value in screen
  // coordinates.
  views::View::ConvertRectToScreen(host_view_, &rect);
  return rect;
}

bool PrefixSelector::GetCompositionCharacterBounds(uint32_t index,
                                                   gfx::Rect* rect) const {
  // TextInputClient::GetCompositionCharacterBounds is expected to fill |rect|
  // in screen coordinates and GetCaretBounds returns screen coordinates.
  *rect = GetCaretBounds();
  return false;
}

bool PrefixSelector::HasCompositionText() const {
  return false;
}

ui::TextInputClient::FocusReason PrefixSelector::GetFocusReason() const {
  // TODO(https://crbug.com/824604): Implement this correctly.
  NOTIMPLEMENTED_LOG_ONCE();
  return ui::TextInputClient::FOCUS_REASON_OTHER;
}

bool PrefixSelector::GetTextRange(gfx::Range* range) const {
  *range = gfx::Range();
  return false;
}

bool PrefixSelector::GetCompositionTextRange(gfx::Range* range) const {
  *range = gfx::Range();
  return false;
}

bool PrefixSelector::GetSelectionRange(gfx::Range* range) const {
  *range = gfx::Range();
  return false;
}

bool PrefixSelector::SetSelectionRange(const gfx::Range& range) {
  return false;
}

bool PrefixSelector::DeleteRange(const gfx::Range& range) {
  return false;
}

bool PrefixSelector::GetTextFromRange(const gfx::Range& range,
                                        base::string16* text) const {
  return false;
}

void PrefixSelector::OnInputMethodChanged() {
  ClearText();
}

bool PrefixSelector::ChangeTextDirectionAndLayoutAlignment(
    base::i18n::TextDirection direction) {
  return true;
}

void PrefixSelector::ExtendSelectionAndDelete(size_t before, size_t after) {
}

void PrefixSelector::EnsureCaretNotInRect(const gfx::Rect& rect) {}

bool PrefixSelector::IsTextEditCommandEnabled(
    ui::TextEditCommand command) const {
  return false;
}

void PrefixSelector::SetTextEditCommandForNextKeyEvent(
    ui::TextEditCommand command) {}

const std::string& PrefixSelector::GetClientSourceInfo() const {
  // TODO(yhanada): Implement this method.
  NOTIMPLEMENTED_LOG_ONCE();
  return base::EmptyString();
}

bool PrefixSelector::ShouldDoLearning() {
  // TODO(https://crbug.com/311180): Implement this method.
  NOTIMPLEMENTED_LOG_ONCE();
  return false;
}

void PrefixSelector::OnTextInput(const base::string16& text) {
  // Small hack to filter out 'tab' and 'enter' input, as the expectation is
  // that they are control characters and will not affect the currently-active
  // prefix.
  if (text.length() == 1 &&
      (text[0] == L'\t' || text[0] == L'\r' || text[0] == L'\n'))
    return;

  const int row_count = prefix_delegate_->GetRowCount();
  if (row_count == 0)
    return;

  // Search for |text| if it has been a while since the user typed, otherwise
  // append |text| to |current_text_| and search for that. If it has been a
  // while search after the current row, otherwise search starting from the
  // current row.
  int row = std::max(0, prefix_delegate_->GetSelectedRow());
  const base::TimeTicks now(base::TimeTicks::Now());
  if ((now - time_of_last_key_).InMilliseconds() < kTimeBeforeClearingMS) {
    current_text_ += text;
  } else {
    current_text_ = text;
    if (prefix_delegate_->GetSelectedRow() >= 0)
      row = (row + 1) % row_count;
  }
  time_of_last_key_ = now;

  const int start_row = row;
  const base::string16 lower_text(base::i18n::ToLower(current_text_));
  do {
    if (TextAtRowMatchesText(row, lower_text)) {
      prefix_delegate_->SetSelectedRow(row);
      return;
    }
    row = (row + 1) % row_count;
  } while (row != start_row);
}

bool PrefixSelector::TextAtRowMatchesText(int row,
                                          const base::string16& lower_text) {
  const base::string16 model_text(
      base::i18n::ToLower(prefix_delegate_->GetTextForRow(row)));
  return (model_text.size() >= lower_text.size()) &&
      (model_text.compare(0, lower_text.size(), lower_text) == 0);
}

void PrefixSelector::ClearText() {
  current_text_.clear();
  time_of_last_key_ = base::TimeTicks();
}

}  // namespace views
