// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/pepper_mouse_locker.h"

#include "base/logging.h"
#include "remoting/client/empty_cursor_filter.h"
#include "remoting/proto/control.pb.h"

namespace remoting {

PepperMouseLocker::PepperMouseLocker(
    pp::Instance* instance,
    const base::Callback<void(bool)>& enable_mouse_deltas,
    protocol::CursorShapeStub* cursor_stub)
    : pp::MouseLock(instance),
      enable_mouse_deltas_(enable_mouse_deltas),
      cursor_stub_(cursor_stub),
      cursor_shape_(new protocol::CursorShapeInfo),
      callback_factory_(this),
      has_focus_(false),
      mouse_lock_state_(MouseLockOff) {
  *cursor_shape_ = EmptyCursorShape();
}

PepperMouseLocker::~PepperMouseLocker() {}

void PepperMouseLocker::DidChangeFocus(bool has_focus) {
  has_focus_ = has_focus;
  if (has_focus_)
    RequestMouseLock();
}

void PepperMouseLocker::SetCursorShape(
    const protocol::CursorShapeInfo& cursor_shape) {
  *cursor_shape_ = cursor_shape;
  if (IsCursorShapeEmpty(*cursor_shape_)) {
    RequestMouseLock();
  } else {
    CancelMouseLock();
  }
}

void PepperMouseLocker::MouseLockLost() {
  DCHECK(mouse_lock_state_ == MouseLockOn ||
         mouse_lock_state_ == MouseLockCancelling);

  OnMouseLockOff();
}

void PepperMouseLocker::OnMouseLocked(int error) {
  DCHECK(mouse_lock_state_ == MouseLockRequestPending ||
         mouse_lock_state_ == MouseLockCancelling);

  bool should_cancel = (mouse_lock_state_ == MouseLockCancelling);

  // See if the operation succeeded.
  if (error == PP_OK) {
    mouse_lock_state_ = MouseLockOn;
    enable_mouse_deltas_.Run(true);
  } else {
    OnMouseLockOff();
  }

  // Cancel as needed.
  if (should_cancel)
    CancelMouseLock();
}

void PepperMouseLocker::OnMouseLockOff() {
  mouse_lock_state_ = MouseLockOff;
  cursor_stub_->SetCursorShape(*cursor_shape_);
  enable_mouse_deltas_.Run(false);
}

void PepperMouseLocker::RequestMouseLock() {
  // Request mouse lock only if the plugin is focused, the host-supplied cursor
  // is empty and no callback is pending.
  if (!has_focus_)
    return;
  if (!IsCursorShapeEmpty(*cursor_shape_))
    return;
  if (mouse_lock_state_ != MouseLockOff)
    return;

  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&PepperMouseLocker::OnMouseLocked);
  int result = pp::MouseLock::LockMouse(callback);
  if (result != PP_OK_COMPLETIONPENDING) {
    LOG(ERROR) << "Unexpected MouseLock result:" << result;
    return;
  }
  mouse_lock_state_ = MouseLockRequestPending;

  // Hide cursor to avoid it becoming a black square (see crbug.com/285809).
  cursor_stub_->SetCursorShape(EmptyCursorShape());
}

void PepperMouseLocker::CancelMouseLock() {
  switch (mouse_lock_state_) {
    case MouseLockOff:
      OnMouseLockOff();
      break;

    case MouseLockCancelling:
      break;

    case MouseLockRequestPending:
      // The mouse lock request is pending. Delay UnlockMouse() call until
      // the callback is called.
      mouse_lock_state_ = MouseLockCancelling;
      break;

    case MouseLockOn:
      pp::MouseLock::UnlockMouse();

      // Note that mouse-lock has been cancelled. We will continue to receive
      // locked events until MouseLockLost() is called back.
      mouse_lock_state_ = MouseLockCancelling;
      break;

    default:
      NOTREACHED();
  }
}

}  // namespace remoting
