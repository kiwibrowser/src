// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_COMMON_LAYOUT_TEST_LAYOUT_TEST_CONTENT_CLIENT_H_
#define CONTENT_SHELL_COMMON_LAYOUT_TEST_LAYOUT_TEST_CONTENT_CLIENT_H_

#include "base/macros.h"
#include "content/shell/common/shell_content_client.h"

namespace content {

class LayoutTestContentClient : public ShellContentClient {
 public:
  LayoutTestContentClient() {}
  bool CanSendWhileSwappedOut(const IPC::Message* message) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LayoutTestContentClient);
};

}  // namespace content

#endif  // CONTENT_SHELL_COMMON_LAYOUT_TEST_LAYOUT_TEST_CONTENT_CLIENT_H_
