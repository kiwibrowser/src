// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_IOS_CHROME_IO_THREAD_H_
#define IOS_CHROME_BROWSER_IOS_CHROME_IO_THREAD_H_

#include <memory>

#include "ios/components/io_thread/ios_io_thread.h"

class PrefService;

namespace net_log {
class ChromeNetLog;
}  // namespace net_log

// Contains state associated with, initialized and cleaned up on, and
// primarily used on, the IO thread.
class IOSChromeIOThread : public io_thread::IOSIOThread {
 public:
  IOSChromeIOThread(PrefService* local_state, net_log::ChromeNetLog* net_log);
  ~IOSChromeIOThread() override;

 protected:
  // io_thread::IOSIOThread overrides
  std::unique_ptr<net::NetworkDelegate> CreateSystemNetworkDelegate() override;
  std::string GetChannelString() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(IOSChromeIOThread);
};

#endif  // IOS_CHROME_BROWSER_IOS_CHROME_IO_THREAD_H_
