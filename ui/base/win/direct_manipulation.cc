// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/direct_manipulation.h"

#include <objbase.h>
#include <cmath>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/windows_version.h"
#include "ui/base/ui_base_features.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {
namespace win {

// static
std::unique_ptr<DirectManipulationHelper>
DirectManipulationHelper::CreateInstance(HWND window,
                                         WindowEventTarget* event_target) {
  if (!::IsWindow(window))
    return nullptr;

  if (!base::FeatureList::IsEnabled(features::kPrecisionTouchpad))
    return nullptr;

  // DM_POINTERHITTEST supported since Win10.
  if (base::win::GetVersion() < base::win::VERSION_WIN10)
    return nullptr;

  std::unique_ptr<DirectManipulationHelper> instance =
      base::WrapUnique(new DirectManipulationHelper());
  instance->window_ = window;

  if (instance->Initialize(event_target))
    return instance;

  return nullptr;
}

// static
std::unique_ptr<DirectManipulationHelper>
DirectManipulationHelper::CreateInstanceForTesting(
    WindowEventTarget* event_target,
    Microsoft::WRL::ComPtr<IDirectManipulationViewport> viewport) {
  if (!base::FeatureList::IsEnabled(features::kPrecisionTouchpad))
    return nullptr;

  // DM_POINTERHITTEST supported since Win10.
  if (base::win::GetVersion() < base::win::VERSION_WIN10)
    return nullptr;

  std::unique_ptr<DirectManipulationHelper> instance =
      base::WrapUnique(new DirectManipulationHelper());

  instance->event_handler_ = Microsoft::WRL::Make<DirectManipulationHandler>(
      instance.get(), event_target);

  instance->viewport_ = viewport;

  return instance;
}

DirectManipulationHelper::~DirectManipulationHelper() {
  if (viewport_)
    viewport_->Abandon();
}

DirectManipulationHelper::DirectManipulationHelper() {}

bool DirectManipulationHelper::Initialize(WindowEventTarget* event_target) {
  // IDirectManipulationUpdateManager is the first COM object created by the
  // application to retrieve other objects in the Direct Manipulation API.
  // It also serves to activate and deactivate Direct Manipulation functionality
  // on a per-HWND basis.
  HRESULT hr =
      ::CoCreateInstance(CLSID_DirectManipulationManager, nullptr,
                         CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&manager_));
  if (!SUCCEEDED(hr))
    return false;

  // Since we want to use fake viewport, we need UpdateManager to tell a fake
  // fake render frame.
  hr = manager_->GetUpdateManager(IID_PPV_ARGS(&update_manager_));
  if (!SUCCEEDED(hr))
    return false;

  hr = manager_->CreateViewport(nullptr, window_, IID_PPV_ARGS(&viewport_));
  if (!SUCCEEDED(hr))
    return false;

  DIRECTMANIPULATION_CONFIGURATION configuration =
      DIRECTMANIPULATION_CONFIGURATION_INTERACTION |
      DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_X |
      DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_Y |
      DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_INERTIA |
      DIRECTMANIPULATION_CONFIGURATION_RAILS_X |
      DIRECTMANIPULATION_CONFIGURATION_RAILS_Y |
      DIRECTMANIPULATION_CONFIGURATION_SCALING;

  hr = viewport_->ActivateConfiguration(configuration);
  if (!SUCCEEDED(hr))
    return false;

  // Since we are using fake viewport and only want to use Direct Manipulation
  // for touchpad, we need to use MANUALUPDATE option.
  hr = viewport_->SetViewportOptions(
      DIRECTMANIPULATION_VIEWPORT_OPTIONS_MANUALUPDATE);
  if (!SUCCEEDED(hr))
    return false;

  event_handler_ =
      Microsoft::WRL::Make<DirectManipulationHandler>(this, event_target);
  if (!SUCCEEDED(hr))
    return false;

  // We got Direct Manipulation transform from
  // IDirectManipulationViewportEventHandler.
  hr = viewport_->AddEventHandler(window_, event_handler_.Get(),
                                  &view_port_handler_cookie_);
  if (!SUCCEEDED(hr))
    return false;

  // Set default rect for viewport before activate.
  viewport_size_ = {1000, 1000};
  RECT rect = gfx::Rect(viewport_size_).ToRECT();
  hr = viewport_->SetViewportRect(&rect);
  if (!SUCCEEDED(hr))
    return false;

  manager_->Activate(window_);

  hr = viewport_->Enable();
  update_manager_->Update(nullptr);
  if (!SUCCEEDED(hr))
    return false;

  return true;
}

void DirectManipulationHelper::Activate() {
  viewport_->Stop();
  manager_->Activate(window_);
}

void DirectManipulationHelper::Deactivate() {
  viewport_->Stop();
  manager_->Deactivate(window_);
}

void DirectManipulationHelper::SetSize(const gfx::Size& size) {
  if (viewport_size_ == size)
    return;

  viewport_->Stop();

  viewport_size_ = size;
  RECT rect = gfx::Rect(viewport_size_).ToRECT();
  viewport_->SetViewportRect(&rect);
}

bool DirectManipulationHelper::OnPointerHitTest(
    WPARAM w_param,
    WindowEventTarget* event_target) {
  // Update the device scale factor.
  event_handler_->SetDeviceScaleFactor(
      display::win::ScreenWin::GetScaleFactorForHWND(window_));

  // Only DM_POINTERHITTEST can be the first message of input sequence of
  // touchpad input.
  // TODO(chaopeng) Check if Windows API changes:
  // For WM_POINTER, the pointer type will show the event from mouse.
  // For WM_POINTERACTIVATE, the pointer id will be different with the following
  // message.
  event_handler_->SetWindowEventTarget(event_target);

  using GetPointerTypeFn = BOOL(WINAPI*)(UINT32, POINTER_INPUT_TYPE*);
  UINT32 pointer_id = GET_POINTERID_WPARAM(w_param);
  POINTER_INPUT_TYPE pointer_type;
  static GetPointerTypeFn get_pointer_type = reinterpret_cast<GetPointerTypeFn>(
      GetProcAddress(GetModuleHandleA("user32.dll"), "GetPointerType"));
  if (get_pointer_type && get_pointer_type(pointer_id, &pointer_type) &&
      pointer_type == PT_TOUCHPAD && event_target) {
    viewport_->SetContact(pointer_id);
    // Request begin frame for fake viewport.
    need_poll_events_ = true;
  }
  return need_poll_events_;
}

HRESULT DirectManipulationHelper::ResetViewport(bool need_poll_events) {
  // By zooming the primary content to a rect that match the viewport rect, we
  // reset the content's transform to identity.
  HRESULT hr =
      viewport_->ZoomToRect(static_cast<float>(0), static_cast<float>(0),
                            static_cast<float>(viewport_size_.width()),
                            static_cast<float>(viewport_size_.height()), FALSE);
  if (!SUCCEEDED(hr))
    return hr;

  need_poll_events_ = need_poll_events;
  return S_OK;
}

bool DirectManipulationHelper::PollForNextEvent() {
  // Simulate 1 frame in update_manager_.
  update_manager_->Update(nullptr);
  return need_poll_events_;
}

void DirectManipulationHelper::SetDeviceScaleFactorForTesting(float factor) {
  event_handler_->SetDeviceScaleFactor(factor);
}

// DirectManipulationHandler
DirectManipulationHandler::DirectManipulationHandler() {
  NOTREACHED();
}

DirectManipulationHandler::DirectManipulationHandler(
    DirectManipulationHelper* helper,
    WindowEventTarget* event_target)
    : helper_(helper), event_target_(event_target) {}

DirectManipulationHandler::~DirectManipulationHandler() {}

void DirectManipulationHandler::TransitionToState(Gesture new_gesture_state) {
  if (gesture_state_ == new_gesture_state)
    return;

  Gesture previous_gesture_state = gesture_state_;
  gesture_state_ = new_gesture_state;

  // End the previous sequence.
  switch (previous_gesture_state) {
    case Gesture::kScroll: {
      // kScroll -> kNone, kPinch, ScrollEnd.
      // kScroll -> kFling, we don't want to end the current scroll sequence.
      if (new_gesture_state != Gesture::kFling)
        event_target_->ApplyPanGestureScrollEnd();
      break;
    }
    case Gesture::kFling: {
      // kFling -> *, FlingEnd.
      event_target_->ApplyPanGestureFlingEnd();
      break;
    }
    case Gesture::kPinch: {
      DCHECK_EQ(new_gesture_state, Gesture::kNone);
      // kPinch -> kNone, PinchEnd. kPinch should only transition to kNone.
      event_target_->ApplyPinchZoomEnd();
      break;
    }
    case Gesture::kNone: {
      // kNone -> *, no cleanup is needed.
      break;
    }
    default:
      NOTREACHED();
  }

  // Start the new sequence.
  switch (new_gesture_state) {
    case Gesture::kScroll: {
      // kFling, kNone -> kScroll, ScrollBegin.
      // ScrollBegin is different phase event with others. It must send within
      // the first scroll event.
      should_send_scroll_begin_ = true;
      break;
    }
    case Gesture::kFling: {
      // Only kScroll can transition to kFling.
      DCHECK_EQ(previous_gesture_state, Gesture::kScroll);
      event_target_->ApplyPanGestureFlingBegin();
      break;
    }
    case Gesture::kPinch: {
      // * -> kPinch, PinchBegin.
      // Pinch gesture may begin with some scroll events.
      event_target_->ApplyPinchZoomBegin();
      break;
    }
    case Gesture::kNone: {
      // * -> kNone, only cleanup is needed.
      break;
    }
    default:
      NOTREACHED();
  }
}

HRESULT DirectManipulationHandler::OnViewportStatusChanged(
    IDirectManipulationViewport* viewport,
    DIRECTMANIPULATION_STATUS current,
    DIRECTMANIPULATION_STATUS previous) {
  // MSDN never mention |viewport| are nullable and we never saw it is null when
  // testing.
  DCHECK(viewport);

  // The state of our viewport has changed! We'l be in one of three states:
  // - ENABLED: initial state
  // - READY: the previous gesture has been completed
  // - RUNNING: gesture updating
  // - INERTIA: finger leave touchpad content still updating by inertia
  HRESULT hr = S_OK;

  // Windows should not call this when event_target_ is null since we do not
  // pass the DM_POINTERHITTEST to DirectManipulation.
  if (!event_target_)
    return hr;

  if (current == previous)
    return hr;

  if (current == DIRECTMANIPULATION_INERTIA) {
    // Fling must lead by Scroll. We can actually hit here when user pinch then
    // quickly pan gesture and leave touchpad. In this case, we don't want to
    // start a new sequence until the gesture end. The rest events in sequence
    // will be ignore since sequence still in pinch and only scale factor
    // changes will be applied.
    if (previous != DIRECTMANIPULATION_RUNNING ||
        gesture_state_ != Gesture::kScroll) {
      return hr;
    }

    TransitionToState(Gesture::kFling);
  }

  if (current == DIRECTMANIPULATION_RUNNING) {
    // INERTIA -> RUNNING, should start a new sequence.
    if (previous == DIRECTMANIPULATION_INERTIA)
      TransitionToState(Gesture::kNone);
  }

  // Reset the viewport when we're idle, so the content transforms always start
  // at identity.
  if (current == DIRECTMANIPULATION_READY) {
    // Every animation will receive 2 ready message, we should stop request
    // compositor animation at the second ready.
    first_ready_ = !first_ready_;
    hr = helper_->ResetViewport(first_ready_);
    last_scale_ = 1.0f;
    last_x_offset_ = 0.0f;
    last_y_offset_ = 0.0f;

    TransitionToState(Gesture::kNone);
  }

  return hr;
}

HRESULT DirectManipulationHandler::OnViewportUpdated(
    IDirectManipulationViewport* viewport) {
  // Nothing to do here.
  return S_OK;
}

namespace {

bool FloatEquals(float f1, float f2) {
  // The idea behind this is to use this fraction of the larger of the
  // two numbers as the limit of the difference.  This breaks down near
  // zero, so we reuse this as the minimum absolute size we will use
  // for the base of the scale too.
  static const float epsilon_scale = 0.00001f;
  return fabs(f1 - f2) <
         epsilon_scale *
             std::fmax(std::fmax(std::fabs(f1), std::fabs(f2)), epsilon_scale);
}

bool DifferentLessThanOne(int f1, int f2) {
  return abs(f1 - f2) < 1;
}

}  // namespace

HRESULT DirectManipulationHandler::OnContentUpdated(
    IDirectManipulationViewport* viewport,
    IDirectManipulationContent* content) {
  // MSDN never mention these params are nullable and we never saw they are null
  // when testing.
  DCHECK(viewport);
  DCHECK(content);

  HRESULT hr = S_OK;

  // Windows should not call this when event_target_ is null since we do not
  // pass the DM_POINTERHITTEST to DirectManipulation.
  if (!event_target_)
    return hr;

  float xform[6];
  hr = content->GetContentTransform(xform, ARRAYSIZE(xform));
  if (!SUCCEEDED(hr))
    return hr;

  float scale = xform[0];
  int x_offset = xform[4] / device_scale_factor_;
  int y_offset = xform[5] / device_scale_factor_;

  // Ignore if Windows pass scale=0 to us.
  if (scale == 0.0f) {
    LOG(ERROR) << "Windows DirectManipulation API pass scale = 0.";
    return hr;
  }

  // Ignore the scale factor change less than float point rounding error and
  // scroll offset change less than 1.
  // TODO(456622) Because we don't fully support fractional scroll, pass float
  // scroll offset feels steppy. eg.
  // first x_offset is 0.1 ignored, but last_x_offset_ set to 0.1
  // second x_offset is 1 but x_offset - last_x_offset_ is 0.9 ignored.
  if (FloatEquals(scale, last_scale_) &&
      DifferentLessThanOne(x_offset, last_x_offset_) &&
      DifferentLessThanOne(y_offset, last_y_offset_)) {
    return hr;
  }

  DCHECK_NE(last_scale_, 0.0f);

  // DirectManipulation will send xy transform move to down-right which is noise
  // when pinch zoom. We should consider the gesture either Scroll or Pinch at
  // one sequence. But Pinch gesture may begin with some scroll transform since
  // DirectManipulation recognition maybe wrong at start if the user pinch with
  // slow motion. So we allow kScroll -> kPinch.

  // Consider this is a Scroll when scale factor equals 1.0.
  if (FloatEquals(scale, 1.0f)) {
    if (gesture_state_ == Gesture::kNone)
      TransitionToState(Gesture::kScroll);
  } else {
    // Pinch gesture may begin with some scroll events.
    TransitionToState(Gesture::kPinch);
  }

  if (gesture_state_ == Gesture::kScroll) {
    if (should_send_scroll_begin_) {
      event_target_->ApplyPanGestureScrollBegin(x_offset - last_x_offset_,
                                                y_offset - last_y_offset_);
      should_send_scroll_begin_ = false;
    } else {
      event_target_->ApplyPanGestureScroll(x_offset - last_x_offset_,
                                           y_offset - last_y_offset_);
    }
  } else if (gesture_state_ == Gesture::kFling) {
    event_target_->ApplyPanGestureFling(x_offset - last_x_offset_,
                                        y_offset - last_y_offset_);
  } else {
    event_target_->ApplyPinchZoomScale(scale / last_scale_);
  }

  last_scale_ = scale;
  last_x_offset_ = x_offset;
  last_y_offset_ = y_offset;

  return hr;
}

void DirectManipulationHandler::SetWindowEventTarget(
    WindowEventTarget* event_target) {
  event_target_ = event_target;
}

void DirectManipulationHandler::SetDeviceScaleFactor(
    float device_scale_factor) {
  device_scale_factor_ = device_scale_factor;
}

}  // namespace win
}  // namespace ui
