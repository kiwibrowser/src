// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/chromeos/mock_ime_engine_handler.h"
#include "ui/base/ime/text_input_flags.h"

namespace chromeos {

MockIMEEngineHandler::MockIMEEngineHandler()
    : focus_in_call_count_(0),
      focus_out_call_count_(0),
      set_surrounding_text_call_count_(0),
      process_key_event_call_count_(0),
      reset_call_count_(0),
      last_text_input_context_(ui::TEXT_INPUT_TYPE_NONE,
                               ui::TEXT_INPUT_MODE_DEFAULT,
                               ui::TEXT_INPUT_FLAG_NONE,
                               ui::TextInputClient::FOCUS_REASON_NONE,
                               false /* should_do_learning */),
      last_set_surrounding_cursor_pos_(0),
      last_set_surrounding_anchor_pos_(0) {}

MockIMEEngineHandler::~MockIMEEngineHandler() {
}

void MockIMEEngineHandler::FocusIn(const InputContext& input_context) {
  last_text_input_context_ = input_context;
  if (last_text_input_context_.type != ui::TEXT_INPUT_TYPE_NONE)
    ++focus_in_call_count_;
}

void MockIMEEngineHandler::FocusOut() {
  if (last_text_input_context_.type != ui::TEXT_INPUT_TYPE_NONE)
    ++focus_out_call_count_;
  last_text_input_context_.type = ui::TEXT_INPUT_TYPE_NONE;
}

void MockIMEEngineHandler::Enable(const std::string& component_id) {
}

void MockIMEEngineHandler::Disable() {
}

void MockIMEEngineHandler::PropertyActivate(const std::string& property_name) {
  last_activated_property_ = property_name;
}

void MockIMEEngineHandler::Reset() {
  ++reset_call_count_;
}

bool MockIMEEngineHandler::IsInterestedInKeyEvent() const {
  return true;
}

void MockIMEEngineHandler::ProcessKeyEvent(const ui::KeyEvent& key_event,
                                           KeyEventDoneCallback callback) {
  ++process_key_event_call_count_;
  last_processed_key_event_.reset(new ui::KeyEvent(key_event));
  last_passed_callback_ = std::move(callback);
}

void MockIMEEngineHandler::CandidateClicked(uint32_t index) {}

void MockIMEEngineHandler::SetSurroundingText(const std::string& text,
                                              uint32_t cursor_pos,
                                              uint32_t anchor_pos,
                                              uint32_t offset_pos) {
  ++set_surrounding_text_call_count_;
  last_set_surrounding_text_ = text;
  last_set_surrounding_cursor_pos_ = cursor_pos;
  last_set_surrounding_anchor_pos_ = anchor_pos;
}

void MockIMEEngineHandler::SetCompositionBounds(
    const std::vector<gfx::Rect>& bounds) {
}

bool MockIMEEngineHandler::ClearComposition(int context_id,
                                            std::string* error) {
  return false;
}

bool MockIMEEngineHandler::CommitText(int context_id,
                                      const char* text,
                                      std::string* error) {
  return false;
}

bool MockIMEEngineHandler::IsActive() const {
  return false;
}

const std::string& MockIMEEngineHandler::GetActiveComponentId() const {
  return active_component_id_;
}

bool MockIMEEngineHandler::DeleteSurroundingText(int context_id,
                                                 int offset,
                                                 size_t number_of_chars,
                                                 std::string* error) {
  return false;
}

bool MockIMEEngineHandler::SetCandidateWindowVisible(bool visible,
                                                     std::string* error) {
  return false;
}

bool MockIMEEngineHandler::SetCursorPosition(int context_id,
                                             int candidate_id,
                                             std::string* error) {
  return false;
}

} // namespace chromeos
