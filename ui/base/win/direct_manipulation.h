// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WIN_DIRECT_MANIPULATION_H_
#define UI_WIN_DIRECT_MANIPULATION_H_

#include <windows.h>

#include <directmanipulation.h>
#include <wrl.h>
#include <memory>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "ui/base/ui_base_export.h"
#include "ui/base/win/window_event_target.h"
#include "ui/gfx/geometry/size.h"

namespace content {
class DirectManipulationBrowserTest;
}  // namespace content

namespace ui {
namespace win {

class DirectManipulationUnitTest;

class DirectManipulationHelper;

// DirectManipulationHandler receives status update and gesture events from
// Direct Manipulation API.
class DirectManipulationHandler
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<
              Microsoft::WRL::RuntimeClassType::ClassicCom>,
          Microsoft::WRL::Implements<
              Microsoft::WRL::RuntimeClassFlags<
                  Microsoft::WRL::RuntimeClassType::ClassicCom>,
              Microsoft::WRL::FtmBase,
              IDirectManipulationViewportEventHandler>> {
 public:
  explicit DirectManipulationHandler(DirectManipulationHelper* helper,
                                     WindowEventTarget* event_target);

  // WindowEventTarget updates for every DM_POINTERHITTEST in case window
  // hierarchy changed.
  void SetWindowEventTarget(WindowEventTarget* event_target);

  void SetDeviceScaleFactor(float device_scale_factor);

 private:
  friend DirectManipulationUnitTest;

  DirectManipulationHandler();
  ~DirectManipulationHandler() override;

  enum class Gesture { kNone, kScroll, kFling, kPinch };

  void TransitionToState(Gesture gesture);

  HRESULT STDMETHODCALLTYPE
  OnViewportStatusChanged(_In_ IDirectManipulationViewport* viewport,
                          _In_ DIRECTMANIPULATION_STATUS current,
                          _In_ DIRECTMANIPULATION_STATUS previous) override;

  HRESULT STDMETHODCALLTYPE
  OnViewportUpdated(_In_ IDirectManipulationViewport* viewport) override;

  HRESULT STDMETHODCALLTYPE
  OnContentUpdated(_In_ IDirectManipulationViewport* viewport,
                   _In_ IDirectManipulationContent* content) override;

  DirectManipulationHelper* helper_ = nullptr;
  WindowEventTarget* event_target_ = nullptr;
  float device_scale_factor_ = 1.0f;
  float last_scale_ = 1.0f;
  int last_x_offset_ = 0;
  int last_y_offset_ = 0;
  bool first_ready_ = false;
  bool should_send_scroll_begin_ = false;

  // Current recognized gesture from Direct Manipulation.
  Gesture gesture_state_ = Gesture::kNone;

  DISALLOW_COPY_AND_ASSIGN(DirectManipulationHandler);
};

// Windows 10 provides a new API called Direct Manipulation which generates
// smooth scroll and scale factor via IDirectManipulationViewportEventHandler
// on precision touchpad.
// 1. The foreground window is checked to see if it is a Direct Manipulation
//    consumer.
// 2. Call SetContact in Direct Manipulation takes over the following scrolling
//    when DM_POINTERHITTEST.
// 3. OnViewportStatusChanged will be called when the gesture phase change.
//    OnContentUpdated will be called when the gesture update.
class UI_BASE_EXPORT DirectManipulationHelper {
 public:
  // Creates and initializes an instance of this class if Direct Manipulation is
  // enabled on the platform. Returns nullptr if it disabled or failed on
  // initialization.
  static std::unique_ptr<DirectManipulationHelper> CreateInstance(
      HWND window,
      WindowEventTarget* event_target);

  // Creates and initializes an instance for testing.
  static std::unique_ptr<DirectManipulationHelper> CreateInstanceForTesting(
      WindowEventTarget* event_target,
      Microsoft::WRL::ComPtr<IDirectManipulationViewport> viewport);

  ~DirectManipulationHelper();

  // Registers and activates the passed in |window| as a Direct Manipulation
  // consumer.
  void Activate();

  // Deactivates Direct Manipulation processing on the passed in |window|.
  void Deactivate();

  // Updates viewport size. Call it when window bounds updated.
  void SetSize(const gfx::Size& size_in_pixels);

  // Reset the fake viewport for gesture end.
  HRESULT ResetViewport(bool need_animtation);

  // Pass the pointer hit test to Direct Manipulation. Return true indicated we
  // need poll for new events every frame from here.
  bool OnPointerHitTest(WPARAM w_param, WindowEventTarget* event_target);

  // On each frame poll new Direct Manipulation events. Return true if we still
  // need poll for new events on next frame, otherwise stop request need begin
  // frame.
  bool PollForNextEvent();

 private:
  friend class content::DirectManipulationBrowserTest;
  friend class DirectManipulationUnitTest;

  DirectManipulationHelper();

  // This function instantiates Direct Manipulation and creates a viewport for
  // the passed in |window|. Return false if initialize failed.
  bool Initialize(WindowEventTarget* event_target);

  void SetDeviceScaleFactorForTesting(float factor);

  Microsoft::WRL::ComPtr<IDirectManipulationManager> manager_;
  Microsoft::WRL::ComPtr<IDirectManipulationUpdateManager> update_manager_;
  Microsoft::WRL::ComPtr<IDirectManipulationViewport> viewport_;
  Microsoft::WRL::ComPtr<DirectManipulationHandler> event_handler_;
  HWND window_;
  DWORD view_port_handler_cookie_;
  bool need_poll_events_ = false;
  gfx::Size viewport_size_;

  DISALLOW_COPY_AND_ASSIGN(DirectManipulationHelper);
};

}  // namespace win
}  // namespace ui

#endif  // UI_WIN_DIRECT_MANIPULATION_H_
