// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_PLUGIN_PEPPER_CURSOR_SETTER_H_
#define REMOTING_CLIENT_PLUGIN_PEPPER_CURSOR_SETTER_H_

#include "base/macros.h"
#include "ppapi/cpp/instance_handle.h"
#include "remoting/protocol/cursor_shape_stub.h"

namespace remoting {

// Helper that applies supplied cursor shapes to the plugin.
class PepperCursorSetter : public protocol::CursorShapeStub {
 public:
  explicit PepperCursorSetter(const pp::InstanceHandle& instance);
  ~PepperCursorSetter() override;

  // protocol::CursorShapeStub interface.
  void SetCursorShape(const protocol::CursorShapeInfo& cursor_shape) override;

  // Sets a CursorShapeStub to which the cursor will be delegated if it cannot
  // be set via PPAPI.
  void set_delegate_stub(protocol::CursorShapeStub* delegate_stub) {
    delegate_stub_ = delegate_stub;
  }

  // Maximum width and height of a mouse cursor supported by PPAPI.
  static const int kMaxCursorWidth = 32;
  static const int kMaxCursorHeight = 32;

 private:
  // Attempts to set the supplied cursor via PPAPI, returning true on success.
  bool SetInstanceCursor(const protocol::CursorShapeInfo& cursor_shape);

  pp::InstanceHandle instance_;
  protocol::CursorShapeStub* delegate_stub_;

  DISALLOW_COPY_AND_ASSIGN(PepperCursorSetter);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_PLUGIN_PEPPER_CURSOR_SETTER_H_
