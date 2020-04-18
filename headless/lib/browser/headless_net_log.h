// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_NET_LOG_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_NET_LOG_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "net/log/net_log.h"

namespace base {
class FilePath;
}

namespace net {
class FileNetLogObserver;
}

namespace headless {

class HeadlessNetLog : public net::NetLog {
 public:
  explicit HeadlessNetLog(const base::FilePath& log_path);
  ~HeadlessNetLog() override;

 private:
  std::unique_ptr<net::FileNetLogObserver> file_net_log_observer_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessNetLog);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_NET_LOG_H_
