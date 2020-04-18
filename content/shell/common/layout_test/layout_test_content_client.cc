// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/common/layout_test/layout_test_content_client.h"

#include "content/shell/common/shell_messages.h"

namespace content {

bool LayoutTestContentClient::CanSendWhileSwappedOut(
    const IPC::Message* message) {
  switch (message->type()) {
    // Used in layout tests; handled in BlinkTestController.
    case ShellViewHostMsg_PrintMessage::ID:
      return true;

    default:
      return false;
  }
}

}  // namespace content
