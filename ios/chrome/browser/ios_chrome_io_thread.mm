// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ios_chrome_io_thread.h"

#include "ios/chrome/browser/net/ios_chrome_network_delegate.h"
#include "ios/chrome/common/channel_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

IOSChromeIOThread::IOSChromeIOThread(PrefService* local_state,
                                     net_log::ChromeNetLog* net_log)
    : IOSIOThread(local_state, net_log) {
  IOSChromeNetworkDelegate::InitializePrefsOnUIThread(nullptr, local_state);
}

IOSChromeIOThread::~IOSChromeIOThread() = default;

std::unique_ptr<net::NetworkDelegate>
IOSChromeIOThread::CreateSystemNetworkDelegate() {
  return std::make_unique<IOSChromeNetworkDelegate>();
}

std::string IOSChromeIOThread::GetChannelString() const {
  return ::GetChannelString();
}
