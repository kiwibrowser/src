// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_INPUT_METHOD_INPUT_METHOD_ENGINE_BASE_H_
#define CHROME_BROWSER_UI_INPUT_METHOD_INPUT_METHOD_ENGINE_BASE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "ui/base/ime/composition_text.h"
#include "ui/base/ime/ime_engine_handler_interface.h"
#include "url/gurl.h"

#if defined(OS_CHROMEOS)
#include "ui/base/ime/chromeos/input_method_descriptor.h"
#endif  // defined(OS_CHROMEOS)

class Profile;

namespace ui {
struct CompositionText;
class IMEEngineHandlerInterface;
class KeyEvent;
}  // namespace ui

namespace input_method {

class InputMethodEngineBase : virtual public ui::IMEEngineHandlerInterface {
 public:
  struct KeyboardEvent {
    KeyboardEvent();
    KeyboardEvent(const KeyboardEvent& other);
    virtual ~KeyboardEvent();

    std::string type;
    std::string key;
    std::string code;
    int key_code;  // only used by on-screen keyboards.
    std::string extension_id;
    bool alt_key;
    bool ctrl_key;
    bool shift_key;
    bool caps_lock;
  };

  enum SegmentStyle {
    SEGMENT_STYLE_UNDERLINE,
    SEGMENT_STYLE_DOUBLE_UNDERLINE,
    SEGMENT_STYLE_NO_UNDERLINE,
  };

  struct SegmentInfo {
    int start;
    int end;
    SegmentStyle style;
  };

#if defined(OS_CHROMEOS)
  enum MouseButtonEvent {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE,
  };
#endif

  class Observer {
   public:
    virtual ~Observer() {}

    // Called when the IME becomes the active IME.
    virtual void OnActivate(const std::string& engine_id) = 0;

    // Called when a text field gains focus, and will be sending key events.
    virtual void OnFocus(
        const IMEEngineHandlerInterface::InputContext& context) = 0;

    // Called when a text field loses focus, and will no longer generate events.
    virtual void OnBlur(int context_id) = 0;

    // Called when the user pressed a key with a text field focused.
    virtual void OnKeyEvent(
        const std::string& engine_id,
        const InputMethodEngineBase::KeyboardEvent& event,
        ui::IMEEngineHandlerInterface::KeyEventDoneCallback key_data) = 0;

    // Called when Chrome terminates on-going text input session.
    virtual void OnReset(const std::string& engine_id) = 0;

    // Called when the IME is no longer active.
    virtual void OnDeactivated(const std::string& engine_id) = 0;

    // Called when composition bounds are changed.
    virtual void OnCompositionBoundsChanged(
        const std::vector<gfx::Rect>& bounds) = 0;

    // Returns whether the observer is interested in key events.
    virtual bool IsInterestedInKeyEvent() const = 0;

    // Called when a surrounding text is changed.
    virtual void OnSurroundingTextChanged(const std::string& engine_id,
                                          const std::string& text,
                                          int cursor_pos,
                                          int anchor_pos,
                                          int offset_pos) = 0;

#if defined(OS_CHROMEOS)

    // Called when an InputContext's properties change while it is focused.
    virtual void OnInputContextUpdate(
        const IMEEngineHandlerInterface::InputContext& context) = 0;

    // Called when the user clicks on an item in the candidate list.
    virtual void OnCandidateClicked(
        const std::string& component_id,
        int candidate_id,
        InputMethodEngineBase::MouseButtonEvent button) = 0;

    // Called when a menu item for this IME is interacted with.
    virtual void OnMenuItemActivated(const std::string& component_id,
                                     const std::string& menu_id) = 0;
#endif  // defined(OS_CHROMEOS)
  };

  InputMethodEngineBase();

  ~InputMethodEngineBase() override;

  void Initialize(std::unique_ptr<InputMethodEngineBase::Observer> observer,
                  const char* extension_id,
                  Profile* profile);

  // IMEEngineHandlerInterface overrides.
  void FocusIn(const ui::IMEEngineHandlerInterface::InputContext& input_context)
      override;
  void FocusOut() override;
  void Enable(const std::string& component_id) override;
  void Disable() override;
  void Reset() override;
  void ProcessKeyEvent(const ui::KeyEvent& key_event,
                       KeyEventDoneCallback callback) override;
  void SetSurroundingText(const std::string& text,
                          uint32_t cursor_pos,
                          uint32_t anchor_pos,
                          uint32_t offset_pos) override;
  void SetCompositionBounds(const std::vector<gfx::Rect>& bounds) override;
  bool ClearComposition(int context_id, std::string* error) override;
  bool CommitText(int context_id,
                  const char* text,
                  std::string* error) override;
  const std::string& GetActiveComponentId() const override;
  bool DeleteSurroundingText(int context_id,
                             int offset,
                             size_t number_of_chars,
                             std::string* error) override;
  int GetCotextIdForTesting() { return context_id_; }
  bool IsInterestedInKeyEvent() const override;

  // Send the sequence of key events.
  bool SendKeyEvents(int context_id, const std::vector<KeyboardEvent>& events);

  // Set the current composition and associated properties.
  bool SetComposition(int context_id,
                      const char* text,
                      int selection_start,
                      int selection_end,
                      int cursor,
                      const std::vector<SegmentInfo>& segments,
                      std::string* error);
  // Called when a key event is handled.
  void KeyEventHandled(const std::string& extension_id,
                       const std::string& request_id,
                       bool handled);

  // Adds unprocessed key event to |request_map_|.
  std::string AddRequest(
      const std::string& component_id,
      ui::IMEEngineHandlerInterface::KeyEventDoneCallback key_data);

  // Get the composition bounds.
  const std::vector<gfx::Rect>& composition_bounds() const {
    return composition_bounds_;
  }

 protected:
  // Notifies InputContextHandler that the composition is changed.
  virtual void UpdateComposition(const ui::CompositionText& composition_text,
                                 uint32_t cursor_pos,
                                 bool is_visible) = 0;
  // Notifies InputContextHanlder to commit |text|.
  virtual void CommitTextToInputContext(int context_id,
                                        const std::string& text) = 0;
  // Sends the key event to the window tree host.
  virtual bool SendKeyEvent(ui::KeyEvent* ui_event,
                            const std::string& code) = 0;

  ui::TextInputType current_input_type_;

  // ID that is used for the current input context.  False if there is no focus.
  int context_id_;

  // Next id that will be assigned to a context.
  int next_context_id_;

  // The input_component ID in IME extension's manifest.
  std::string active_component_id_;

  // The IME extension ID.
  std::string extension_id_;

  // The observer object recieving events for this IME.
  std::unique_ptr<InputMethodEngineBase::Observer> observer_;

  // The current preedit text, and it's cursor position.
  std::unique_ptr<ui::CompositionText> composition_text_;
  int composition_cursor_;

  // Used with SendKeyEvents and ProcessKeyEvent to check if the key event
  // sent to ProcessKeyEvent is sent by SendKeyEvents.
  const ui::KeyEvent* sent_key_event_;

  Profile* profile_;

  using RequestMap =
      std::map<std::string,
               std::pair<std::string,
                         ui::IMEEngineHandlerInterface::KeyEventDoneCallback>>;

  unsigned int next_request_id_;
  RequestMap request_map_;

  // The composition text to be set from calling input.ime.setComposition API.
  ui::CompositionText composition_;
  bool composition_changed_;

  // The composition bounds returned by inputMethodPrivate.getCompositionBounds
  // API.
  std::vector<gfx::Rect> composition_bounds_;

  // The text to be committed from calling input.ime.commitText API.
  std::string text_;
  bool commit_text_changed_;

  // Indicates whether the IME extension is currently handling a physical key
  // event. This is used in CommitText/UpdateCompositionText/etc.
  bool handling_key_event_;
};

}  // namespace input_method

#endif  // CHROME_BROWSER_UI_INPUT_METHOD_INPUT_METHOD_ENGINE_BASE_H_
