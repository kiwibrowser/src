// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_PLUGIN_PEPPER_MOUSE_LOCKER_H_
#define REMOTING_CLIENT_PLUGIN_PEPPER_MOUSE_LOCKER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "ppapi/cpp/mouse_lock.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "remoting/protocol/cursor_shape_stub.h"

namespace pp {
class Instance;
}  // namespace pp

namespace remoting {

class PepperMouseLocker : public pp::MouseLock,
                          public protocol::CursorShapeStub {
 public:
  // |instance| and |cursor_stub| must outlive |this|.
  PepperMouseLocker(
      pp::Instance* instance,
      const base::Callback<void(bool)>& enable_mouse_deltas,
      protocol::CursorShapeStub* cursor_stub);
  ~PepperMouseLocker() override;

  // Must be called when the plugin receives or loses focus.
  void DidChangeFocus(bool has_focus);

  // protocol::CursorShapeStub interface.
  void SetCursorShape(const protocol::CursorShapeInfo& cursor_shape) override;

private:
  enum MouseLockState {
    MouseLockOff,
    MouseLockRequestPending,
    MouseLockOn,
    MouseLockCancelling
  };

  // pp::MouseLock interface.
  void MouseLockLost() override;

  // Handles completion of the mouse lock request issued by RequestMouseLock().
  void OnMouseLocked(int error);

  // Sets the mouse-lock state to "off" and restores the normal mouse cursor.
  void OnMouseLockOff();

  // Requests the browser to lock the mouse and hides the cursor.
  void RequestMouseLock();

  // Requests the browser to cancel mouse lock and restores the cursor once
  // the lock is gone.
  void CancelMouseLock();

  // Callback used to enable/disable delta motion fields in mouse events.
  base::Callback<void(bool)> enable_mouse_deltas_;

  // Stub used to set the cursor that will actually be displayed.
  protocol::CursorShapeStub* cursor_stub_;

  // Copy of the most-recently-set cursor, to set when mouse-lock is cancelled.
  std::unique_ptr<protocol::CursorShapeInfo> cursor_shape_;

  // Used to create PPAPI callbacks that will be abandoned when |this| is
  // deleted.
  pp::CompletionCallbackFactory<PepperMouseLocker> callback_factory_;

  // True if the plugin has focus.
  bool has_focus_;

  // Holds the current mouse lock state.
  MouseLockState mouse_lock_state_;

  DISALLOW_COPY_AND_ASSIGN(PepperMouseLocker);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_PLUGIN_PEPPER_MOUSE_LOCKER_H_
