// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_SCOPED_ANDROID_CONFIGURATION_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_SCOPED_ANDROID_CONFIGURATION_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"

namespace net {
class SocketPosix;
}

namespace content {

class ScopedAndroidConfiguration {
 public:
  ScopedAndroidConfiguration();
  ~ScopedAndroidConfiguration();

  // Redirects stdin, stdout, and stderr to the ports specified on the command
  // line.
  void RedirectStreams();

 private:
  // Adds |socket| to |sockets_|.
  void AddSocket(std::unique_ptr<net::SocketPosix> socket);

  // The sockets to which the IO streams are being redirected.
  std::vector<std::unique_ptr<net::SocketPosix>> sockets_;

  DISALLOW_COPY_AND_ASSIGN(ScopedAndroidConfiguration);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_SCOPED_ANDROID_CONFIGURATION_H_
