// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_INPUT_METHOD_INPUT_METHOD_ENGINE_H_
#define CHROME_BROWSER_CHROMEOS_INPUT_METHOD_INPUT_METHOD_ENGINE_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <string>
#include <vector>
#include "base/time/time.h"
#include "chrome/browser/ui/input_method/input_method_engine_base.h"
#include "ui/base/ime/chromeos/input_method_descriptor.h"
#include "ui/base/ime/chromeos/input_method_manager.h"
#include "ui/base/ime/ime_engine_handler_interface.h"
#include "url/gurl.h"

namespace ui {
class CandidateWindow;
struct CompositionText;
class IMEEngineHandlerInterface;
class KeyEvent;

namespace ime {
struct InputMethodMenuItem;
}  // namespace ime
}  // namespace ui

namespace input_method {
class InputMethodEngineBase;
}

namespace chromeos {

class InputMethodEngine : public ::input_method::InputMethodEngineBase {
 public:
  enum {
    MENU_ITEM_MODIFIED_LABEL = 0x0001,
    MENU_ITEM_MODIFIED_STYLE = 0x0002,
    MENU_ITEM_MODIFIED_VISIBLE = 0x0004,
    MENU_ITEM_MODIFIED_ENABLED = 0x0008,
    MENU_ITEM_MODIFIED_CHECKED = 0x0010,
    MENU_ITEM_MODIFIED_ICON = 0x0020,
  };

  enum CandidateWindowPosition {
    WINDOW_POS_CURSOR,
    WINDOW_POS_COMPOSITTION,
  };

  struct UsageEntry {
    std::string title;
    std::string body;
  };

  struct Candidate {
    Candidate();
    Candidate(const Candidate& other);
    virtual ~Candidate();

    std::string value;
    int id;
    std::string label;
    std::string annotation;
    UsageEntry usage;
    std::vector<Candidate> candidates;
  };

  struct CandidateWindowProperty {
    CandidateWindowProperty();
    virtual ~CandidateWindowProperty();
    int page_size;
    bool is_cursor_visible;
    bool is_vertical;
    bool show_window_at_composition;

    // Auxiliary text is typically displayed in the footer of the candidate
    // window.
    std::string auxiliary_text;
    bool is_auxiliary_text_visible;
  };

  InputMethodEngine();

  ~InputMethodEngine() override;

  // IMEEngineHandlerInterface overrides.
  bool SetCandidateWindowVisible(bool visible, std::string* error) override;
  bool SetCursorPosition(int context_id,
                         int candidate_id,
                         std::string* error) override;
  bool IsActive() const override;
  void Enable(const std::string& component_id) override;
  void PropertyActivate(const std::string& property_name) override;
  void CandidateClicked(uint32_t index) override;
  void HideInputView() override;

  // This function returns the current property of the candidate window.
  // The caller can use the returned value as the default property and
  // modify some of specified items.
  const CandidateWindowProperty& GetCandidateWindowProperty() const;

  // Change the property of the candidate window and repaint the candidate
  // window widget.
  void SetCandidateWindowProperty(const CandidateWindowProperty& property);

  // Set the list of entries displayed in the candidate window.
  bool SetCandidates(int context_id,
                     const std::vector<Candidate>& candidates,
                     std::string* error);

  // Set the list of items that appears in the language menu when this IME is
  // active.
  bool SetMenuItems(
      const std::vector<input_method::InputMethodManager::MenuItem>& items);

  // Update the state of the menu items.
  bool UpdateMenuItems(
      const std::vector<input_method::InputMethodManager::MenuItem>& items);

 private:
  // Converts MenuItem to InputMethodMenuItem.
  void MenuItemToProperty(
      const input_method::InputMethodManager::MenuItem& item,
      ui::ime::InputMethodMenuItem* property);

  // Enables overriding input view page to Virtual Keyboard window.
  void EnableInputView();

  // input_method::InputMethodEngineBase:
  void UpdateComposition(const ui::CompositionText& composition_text,
                         uint32_t cursor_pos,
                         bool is_visible) override;
  void CommitTextToInputContext(int context_id,
                                const std::string& text) override;
  bool SendKeyEvent(ui::KeyEvent* event, const std::string& code) override;

  // The current candidate window.
  std::unique_ptr<ui::CandidateWindow> candidate_window_;

  // The current candidate window property.
  CandidateWindowProperty candidate_window_property_;

  // Indicates whether the candidate window is visible.
  bool window_visible_;

  // Mapping of candidate index to candidate id.
  std::vector<int> candidate_ids_;

  // Mapping of candidate id to index.
  std::map<int, int> candidate_indexes_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_INPUT_METHOD_INPUT_METHOD_ENGINE_H_
