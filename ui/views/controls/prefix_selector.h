// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_PREFIX_SELECTOR_H_
#define UI_VIEWS_CONTROLS_PREFIX_SELECTOR_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/views/views_export.h"

namespace views {

class PrefixDelegate;
class View;

// PrefixSelector is used to change the selection in a view as the user
// types characters.
class VIEWS_EXPORT PrefixSelector : public ui::TextInputClient {
 public:
  PrefixSelector(PrefixDelegate* delegate, View* host_view);
  ~PrefixSelector() override;

  // Invoked from the view when it loses focus.
  void OnViewBlur();

  // ui::TextInputClient:
  void SetCompositionText(const ui::CompositionText& composition) override;
  void ConfirmCompositionText() override;
  void ClearCompositionText() override;
  void InsertText(const base::string16& text) override;
  void InsertChar(const ui::KeyEvent& event) override;
  ui::TextInputType GetTextInputType() const override;
  ui::TextInputMode GetTextInputMode() const override;
  base::i18n::TextDirection GetTextDirection() const override;
  int GetTextInputFlags() const override;
  bool CanComposeInline() const override;
  gfx::Rect GetCaretBounds() const override;
  bool GetCompositionCharacterBounds(uint32_t index,
                                     gfx::Rect* rect) const override;
  bool HasCompositionText() const override;
  FocusReason GetFocusReason() const override;
  bool GetTextRange(gfx::Range* range) const override;
  bool GetCompositionTextRange(gfx::Range* range) const override;
  bool GetSelectionRange(gfx::Range* range) const override;
  bool SetSelectionRange(const gfx::Range& range) override;
  bool DeleteRange(const gfx::Range& range) override;
  bool GetTextFromRange(const gfx::Range& range,
                        base::string16* text) const override;
  void OnInputMethodChanged() override;
  bool ChangeTextDirectionAndLayoutAlignment(
      base::i18n::TextDirection direction) override;
  void ExtendSelectionAndDelete(size_t before, size_t after) override;
  void EnsureCaretNotInRect(const gfx::Rect& rect) override;

  bool IsTextEditCommandEnabled(ui::TextEditCommand command) const override;
  void SetTextEditCommandForNextKeyEvent(ui::TextEditCommand command) override;
  const std::string& GetClientSourceInfo() const override;
  bool ShouldDoLearning() override;

 private:
  // Invoked when text is typed. Tries to change the selection appropriately.
  void OnTextInput(const base::string16& text);

  // Returns true if the text of the node at |row| starts with |lower_text|.
  bool TextAtRowMatchesText(int row, const base::string16& lower_text);

  // Clears |current_text_| and resets |time_of_last_key_|.
  void ClearText();

  PrefixDelegate* prefix_delegate_;

  View* host_view_;

  // Time OnTextInput() was last invoked.
  base::TimeTicks time_of_last_key_;

  base::string16 current_text_;

  DISALLOW_COPY_AND_ASSIGN(PrefixSelector);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_PREFIX_SELECTOR_H_
