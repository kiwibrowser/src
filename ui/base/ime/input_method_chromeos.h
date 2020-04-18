// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_INPUT_METHOD_CHROMEOS_H_
#define UI_BASE_IME_INPUT_METHOD_CHROMEOS_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/ime/chromeos/character_composer.h"
#include "ui/base/ime/composition_text.h"
#include "ui/base/ime/ime_input_context_handler_interface.h"
#include "ui/base/ime/input_method_base.h"
#include "ui/base/ime/text_input_client.h"

namespace ui {

// A ui::InputMethod implementation based on IBus.
class UI_BASE_IME_EXPORT InputMethodChromeOS : public InputMethodBase {
 public:
  explicit InputMethodChromeOS(internal::InputMethodDelegate* delegate);
  ~InputMethodChromeOS() override;

  using AckCallback = base::OnceCallback<void(bool)>;
  ui::EventDispatchDetails DispatchKeyEvent(ui::KeyEvent* event,
                                            AckCallback ack_callback);

  // Overridden from InputMethod:
  ui::EventDispatchDetails DispatchKeyEvent(ui::KeyEvent* event) override;
  void OnTextInputTypeChanged(const TextInputClient* client) override;
  void OnCaretBoundsChanged(const TextInputClient* client) override;
  void CancelComposition(const TextInputClient* client) override;
  bool IsCandidatePopupOpen() const override;
  InputMethodKeyboardController* GetInputMethodKeyboardController() override;

 protected:
  // Converts |text| into CompositionText.
  void ExtractCompositionText(const CompositionText& text,
                              uint32_t cursor_position,
                              CompositionText* out_composition) const;

  // Process a key returned from the input method.
  virtual ui::EventDispatchDetails ProcessKeyEventPostIME(
      ui::KeyEvent* event,
      AckCallback ack_callback,
      bool skip_process_filtered,
      bool handled) WARN_UNUSED_RESULT;

  // Resets context and abandon all pending results and key events.
  void ResetContext();

 private:
  class PendingKeyEvent;

  // Overridden from InputMethodBase:
  void OnWillChangeFocusedClient(TextInputClient* focused_before,
                                 TextInputClient* focused) override;
  void OnDidChangeFocusedClient(TextInputClient* focused_before,
                                TextInputClient* focused) override;

  // Asks the client to confirm current composition text.
  void ConfirmCompositionText();

  // Checks the availability of focused text input client and update focus
  // state.
  void UpdateContextFocusState();

  // Processes a key event that was already filtered by the input method.
  // A VKEY_PROCESSKEY may be dispatched to the EventTargets.
  // It returns the result of whether the event has been stopped propagation
  // when dispatching post IME.
  ui::EventDispatchDetails ProcessFilteredKeyPressEvent(
      ui::KeyEvent* event,
      AckCallback ack_callback) WARN_UNUSED_RESULT;

  // Post processes a key event that was already filtered by the input method.
  void PostProcessFilteredKeyPressEvent(ui::KeyEvent* event,
                                        TextInputClient* prev_client,
                                        AckCallback ack_callback,
                                        bool stopped_propagation);

  // Processes a key event that was not filtered by the input method.
  ui::EventDispatchDetails ProcessUnfilteredKeyPressEvent(
      ui::KeyEvent* event,
      AckCallback ack_callback) WARN_UNUSED_RESULT;

  // Post processes a key event that was unfiltered by the input method.
  void PostProcessUnfilteredKeyPressEvent(ui::KeyEvent* event,
                                          TextInputClient* prev_client,
                                          AckCallback ack_callback,
                                          bool stopped_propagation);

  // Sends input method result caused by the given key event to the focused text
  // input client.
  void ProcessInputMethodResult(ui::KeyEvent* event, bool filtered);

  // Checks if the pending input method result needs inserting into the focused
  // text input client as a single character.
  bool NeedInsertChar() const;

  // Checks if there is pending input method result.
  bool HasInputMethodResult() const;

  // Passes keyevent and executes character composition if necessary. Returns
  // true if character composer comsumes key event.
  bool ExecuteCharacterComposer(const ui::KeyEvent& event);

  // ui::IMEInputContextHandlerInterface overrides:
  void CommitText(const std::string& text) override;
  void UpdateCompositionText(const CompositionText& text,
                             uint32_t cursor_pos,
                             bool visible) override;
  void DeleteSurroundingText(int32_t offset, uint32_t length) override;

  // Hides the composition text.
  void HidePreeditText();

  // Callback function for IMEEngineHandlerInterface::ProcessKeyEvent.
  void KeyEventDoneCallback(ui::KeyEvent* event,
                            AckCallback ack_callback,
                            bool is_handled);
  ui::EventDispatchDetails ProcessKeyEventDone(ui::KeyEvent* event,
                                               AckCallback ack_callback,
                                               bool is_handled)
      WARN_UNUSED_RESULT;

  // Returns whether an non-password input field is focused.
  bool IsNonPasswordInputFieldFocused();

  // Returns true if an text input field is focused.
  bool IsInputFieldFocused();

  // Gets the reason how the focused text input client was focused.
  TextInputClient::FocusReason GetClientFocusReason() const;

  // Pending composition text generated by the current pending key event.
  // It'll be sent to the focused text input client as soon as we receive the
  // processing result of the pending key event.
  CompositionText composition_;

  // Pending result text generated by the current pending key event.
  // It'll be sent to the focused text input client as soon as we receive the
  // processing result of the pending key event.
  base::string16 result_text_;

  base::string16 previous_surrounding_text_;
  gfx::Range previous_selection_range_;

  // Indicates if there is an ongoing composition text.
  bool composing_text_;

  // Indicates if the composition text is changed or deleted.
  bool composition_changed_;

  // An object to compose a character from a sequence of key presses
  // including dead key etc.
  CharacterComposer character_composer_;

  // Indicates whether currently is handling a physical key event.
  // This is used in CommitText/UpdateCompositionText/etc.
  bool handling_key_event_;

  // Used for making callbacks.
  base::WeakPtrFactory<InputMethodChromeOS> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(InputMethodChromeOS);
};

}  // namespace ui

#endif  // UI_BASE_IME_INPUT_METHOD_CHROMEOS_H_
