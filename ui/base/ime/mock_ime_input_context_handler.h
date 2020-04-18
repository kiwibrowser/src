// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_MOCK_IME_INPUT_CONTEXT_HANDLER_H_
#define UI_BASE_IME_MOCK_IME_INPUT_CONTEXT_HANDLER_H_

#include <stdint.h>

#include "ui/base/ime/composition_text.h"
#include "ui/base/ime/ime_input_context_handler_interface.h"
#include "ui/base/ime/ui_base_ime_export.h"

namespace ui {
class InputMethod;

class UI_BASE_IME_EXPORT MockIMEInputContextHandler
    : public IMEInputContextHandlerInterface {
 public:
  struct UpdateCompositionTextArg {
    CompositionText composition_text;
    uint32_t cursor_pos;
    bool is_visible;
  };

  struct DeleteSurroundingTextArg {
    int32_t offset;
    uint32_t length;
  };

  MockIMEInputContextHandler();
  virtual ~MockIMEInputContextHandler();

  void CommitText(const std::string& text) override;
  void UpdateCompositionText(const CompositionText& text,
                             uint32_t cursor_pos,
                             bool visible) override;
  void DeleteSurroundingText(int32_t offset, uint32_t length) override;
  void SendKeyEvent(KeyEvent* event) override;
  InputMethod* GetInputMethod() override;

  int commit_text_call_count() const { return commit_text_call_count_; }

  int update_preedit_text_call_count() const {
    return update_preedit_text_call_count_;
  }

  int delete_surrounding_text_call_count() const {
    return delete_surrounding_text_call_count_;
  }

  const std::string& last_commit_text() const { return last_commit_text_; };

  const UpdateCompositionTextArg& last_update_composition_arg() const {
    return last_update_composition_arg_;
  }

  const DeleteSurroundingTextArg& last_delete_surrounding_text_arg() const {
    return last_delete_surrounding_text_arg_;
  }

  // Resets all call count.
  void Reset();

 private:
  int commit_text_call_count_;
  int update_preedit_text_call_count_;
  int delete_surrounding_text_call_count_;
  std::string last_commit_text_;
  UpdateCompositionTextArg last_update_composition_arg_;
  DeleteSurroundingTextArg last_delete_surrounding_text_arg_;
};
}  // ui

#endif  // UI_BASE_IME_MOCK_IME_INPUT_CONTEXT_HANDLER_H_
