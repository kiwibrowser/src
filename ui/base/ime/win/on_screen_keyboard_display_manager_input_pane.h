// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_WIN_ON_SCREEN_KEYBOARD_DISPLAY_MANAGER_INPUT_PANE_H_
#define UI_BASE_IME_WIN_ON_SCREEN_KEYBOARD_DISPLAY_MANAGER_INPUT_PANE_H_

#include <inputpaneinterop.h>
#include <windows.ui.viewmanagement.h>
#include <wrl/client.h>
#include <wrl/event.h>

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/windows_types.h"
#include "ui/base/ime/input_method_keyboard_controller.h"
#include "ui/base/ime/ui_base_ime_export.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {

class OnScreenKeyboardTest;

// This class provides an implementation of the OnScreenKeyboardDisplayManager
// that uses InputPane which is available on Windows >= 10.0.10240.0.
class UI_BASE_IME_EXPORT OnScreenKeyboardDisplayManagerInputPane final
    : public InputMethodKeyboardController {
 public:
  OnScreenKeyboardDisplayManagerInputPane(HWND hwnd);
  ~OnScreenKeyboardDisplayManagerInputPane() override;

  // InputMethodKeyboardController:
  bool DisplayVirtualKeyboard() override;
  void DismissVirtualKeyboard() override;
  void AddObserver(InputMethodKeyboardControllerObserver* observer) override;
  void RemoveObserver(InputMethodKeyboardControllerObserver* observer) override;
  bool IsKeyboardVisible() override;

  void SetInputPaneForTesting(
      ABI::Windows::UI::ViewManagement::IInputPane* pane);

 private:
  friend class OnScreenKeyboardTest;

  bool EnsureInputPanePointers();
  HRESULT InputPaneShown(
      ABI::Windows::UI::ViewManagement::IInputPane* pane,
      ABI::Windows::UI::ViewManagement::IInputPaneVisibilityEventArgs* args);
  HRESULT InputPaneHidden(
      ABI::Windows::UI::ViewManagement::IInputPane* pane,
      ABI::Windows::UI::ViewManagement::IInputPaneVisibilityEventArgs* args);

  void NotifyObserversOnKeyboardShown(gfx::Rect rect);
  void NotifyObserversOnKeyboardHidden();
  void TryShow();
  void TryHide();

  using InputPaneEventHandler = ABI::Windows::Foundation::ITypedEventHandler<
      ABI::Windows::UI::ViewManagement::InputPane*,
      ABI::Windows::UI::ViewManagement::InputPaneVisibilityEventArgs*>;

  // The main window which displays the on screen keyboard.
  const HWND hwnd_;
  Microsoft::WRL::ComPtr<ABI::Windows::UI::ViewManagement::IInputPane>
      input_pane_;
  Microsoft::WRL::ComPtr<ABI::Windows::UI::ViewManagement::IInputPane2>
      input_pane2_;
  base::ObserverList<InputMethodKeyboardControllerObserver, false> observers_;
  EventRegistrationToken show_event_token_;
  EventRegistrationToken hide_event_token_;
  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  base::WeakPtrFactory<OnScreenKeyboardDisplayManagerInputPane> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(OnScreenKeyboardDisplayManagerInputPane);
};

}  // namespace ui

#endif  // UI_BASE_IME_WIN_ON_SCREEN_KEYBOARD_DISPLAY_MANAGER_INPUT_PANE_H_
