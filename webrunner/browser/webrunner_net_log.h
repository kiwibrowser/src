// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBRUNNER_BROWSER_WEBRUNNER_NET_LOG_H_
#define WEBRUNNER_BROWSER_WEBRUNNER_NET_LOG_H_

#include <memory>

#include "base/macros.h"
#include "net/log/net_log.h"

namespace base {
class FilePath;
}  // namespace base

namespace net {
class FileNetLogObserver;
}  // namespace net

namespace webrunner {

class WebRunnerNetLog : public net::NetLog {
 public:
  explicit WebRunnerNetLog(const base::FilePath& log_path);
  ~WebRunnerNetLog() override;

 private:
  std::unique_ptr<net::FileNetLogObserver> file_net_log_observer_;

  DISALLOW_COPY_AND_ASSIGN(WebRunnerNetLog);
};

}  // namespace webrunner

#endif  // WEBRUNNER_BROWSER_WEBRUNNER_NET_LOG_H_
