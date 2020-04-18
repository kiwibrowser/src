// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/aw_network_change_notifier_factory.h"

#include "android_webview/browser/net/aw_network_change_notifier.h"

namespace android_webview {

AwNetworkChangeNotifierFactory::AwNetworkChangeNotifierFactory() {}

AwNetworkChangeNotifierFactory::~AwNetworkChangeNotifierFactory() {}

net::NetworkChangeNotifier* AwNetworkChangeNotifierFactory::CreateInstance() {
  return new AwNetworkChangeNotifier(&delegate_);
}

}  // namespace android_webview
