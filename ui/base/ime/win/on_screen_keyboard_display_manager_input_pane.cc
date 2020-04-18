// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/win/on_screen_keyboard_display_manager_input_pane.h"

#include "base/strings/utf_string_conversions.h"
#include "base/win/com_init_util.h"
#include "base/win/core_winrt_util.h"
#include "base/win/windows_version.h"
#include "ui/base/ime/input_method_keyboard_controller_observer.h"

namespace ui {

OnScreenKeyboardDisplayManagerInputPane::
    OnScreenKeyboardDisplayManagerInputPane(HWND hwnd)
    : hwnd_(hwnd),
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {
  DCHECK_GE(base::win::GetVersion(), base::win::VERSION_WIN10_RS1);
}

OnScreenKeyboardDisplayManagerInputPane::
    ~OnScreenKeyboardDisplayManagerInputPane() = default;

bool OnScreenKeyboardDisplayManagerInputPane::DisplayVirtualKeyboard() {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&OnScreenKeyboardDisplayManagerInputPane::TryShow,
                     weak_factory_.GetWeakPtr()));
  return true;
}

void OnScreenKeyboardDisplayManagerInputPane::DismissVirtualKeyboard() {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&OnScreenKeyboardDisplayManagerInputPane::TryHide,
                     weak_factory_.GetWeakPtr()));
}

void OnScreenKeyboardDisplayManagerInputPane::AddObserver(
    InputMethodKeyboardControllerObserver* observer) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  observers_.AddObserver(observer);
}

void OnScreenKeyboardDisplayManagerInputPane::RemoveObserver(
    InputMethodKeyboardControllerObserver* observer) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  observers_.RemoveObserver(observer);
}

bool OnScreenKeyboardDisplayManagerInputPane::IsKeyboardVisible() {
  if (!EnsureInputPanePointers())
    return false;
  ABI::Windows::Foundation::Rect rect;
  input_pane_->get_OccludedRect(&rect);
  // Height == 0 is special indicating it is floating on only check width.
  return rect.Width != 0;
}

void OnScreenKeyboardDisplayManagerInputPane::SetInputPaneForTesting(
    ABI::Windows::UI::ViewManagement::IInputPane* input_pane) {
  DCHECK(!input_pane_);
  input_pane_ = input_pane;
  HRESULT hr = input_pane_.As(&input_pane2_);
  DCHECK(SUCCEEDED(hr));

  input_pane_->add_Showing(
      Microsoft::WRL::Callback<InputPaneEventHandler>(
          this, &OnScreenKeyboardDisplayManagerInputPane::InputPaneShown)
          .Get(),
      &show_event_token_);
  input_pane_->add_Hiding(
      Microsoft::WRL::Callback<InputPaneEventHandler>(
          this, &OnScreenKeyboardDisplayManagerInputPane::InputPaneHidden)
          .Get(),
      &hide_event_token_);
}

bool OnScreenKeyboardDisplayManagerInputPane::EnsureInputPanePointers() {
  if (input_pane2_)
    return true;
  if (!base::win::ResolveCoreWinRTDelayload() ||
      !base::win::ScopedHString::ResolveCoreWinRTStringDelayload()) {
    return false;
  }

  base::win::AssertComApartmentType(base::win::ComApartmentType::MTA);
  base::win::ScopedHString input_pane_guid = base::win::ScopedHString::Create(
      RuntimeClass_Windows_UI_ViewManagement_InputPane);
  Microsoft::WRL::ComPtr<IInputPaneInterop> input_pane_interop;
  HRESULT hr = base::win::RoGetActivationFactory(
      input_pane_guid.get(), IID_PPV_ARGS(&input_pane_interop));
  if (FAILED(hr))
    return false;

  hr = input_pane_interop->GetForWindow(hwnd_, IID_PPV_ARGS(&input_pane_));
  if (FAILED(hr))
    return false;

  hr = input_pane_.As(&input_pane2_);
  if (FAILED(hr))
    return false;

  input_pane_->add_Showing(
      Microsoft::WRL::Callback<InputPaneEventHandler>(
          this, &OnScreenKeyboardDisplayManagerInputPane::InputPaneShown)
          .Get(),
      &show_event_token_);
  input_pane_->add_Hiding(
      Microsoft::WRL::Callback<InputPaneEventHandler>(
          this, &OnScreenKeyboardDisplayManagerInputPane::InputPaneHidden)
          .Get(),
      &hide_event_token_);
  return true;
}

HRESULT OnScreenKeyboardDisplayManagerInputPane::InputPaneShown(
    ABI::Windows::UI::ViewManagement::IInputPane* pane,
    ABI::Windows::UI::ViewManagement::IInputPaneVisibilityEventArgs* args) {
  // get_OccludedRect is in DIPs already.
  ABI::Windows::Foundation::Rect rect;
  input_pane_->get_OccludedRect(&rect);
  gfx::Rect dip_rect(rect.X, rect.Y, rect.Width, rect.Height);

  main_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&OnScreenKeyboardDisplayManagerInputPane::
                                    NotifyObserversOnKeyboardShown,
                                weak_factory_.GetWeakPtr(), dip_rect));
  return S_OK;
}

HRESULT OnScreenKeyboardDisplayManagerInputPane::InputPaneHidden(
    ABI::Windows::UI::ViewManagement::IInputPane* pane,
    ABI::Windows::UI::ViewManagement::IInputPaneVisibilityEventArgs* args) {
  main_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&OnScreenKeyboardDisplayManagerInputPane::
                                    NotifyObserversOnKeyboardHidden,
                                weak_factory_.GetWeakPtr()));
  return S_OK;
}

void OnScreenKeyboardDisplayManagerInputPane::NotifyObserversOnKeyboardShown(
    gfx::Rect dip_rect) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  for (InputMethodKeyboardControllerObserver& observer : observers_)
    observer.OnKeyboardVisible(dip_rect);
}

void OnScreenKeyboardDisplayManagerInputPane::
    NotifyObserversOnKeyboardHidden() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  for (InputMethodKeyboardControllerObserver& observer : observers_)
    observer.OnKeyboardHidden();
}

void OnScreenKeyboardDisplayManagerInputPane::TryShow() {
  if (!EnsureInputPanePointers())
    return;
  boolean res;
  input_pane2_->TryShow(&res);
}

void OnScreenKeyboardDisplayManagerInputPane::TryHide() {
  if (!input_pane2_)
    return;
  boolean res;
  input_pane2_->TryHide(&res);
}

}  // namespace ui
