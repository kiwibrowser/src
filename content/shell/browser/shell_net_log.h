// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_SHELL_NET_LOG_H_
#define CONTENT_SHELL_BROWSER_SHELL_NET_LOG_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "net/log/net_log.h"

namespace net {
class FileNetLogObserver;
}

namespace content {

class ShellNetLog : public net::NetLog {
 public:
  explicit ShellNetLog(const std::string& app_name);
  ~ShellNetLog() override;

 private:
  std::unique_ptr<net::FileNetLogObserver> file_net_log_observer_;

  DISALLOW_COPY_AND_ASSIGN(ShellNetLog);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_SHELL_NET_LOG_H_
