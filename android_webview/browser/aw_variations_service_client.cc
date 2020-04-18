// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_variations_service_client.h"

#include "base/bind.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "components/version_info/android/channel_getter.h"
#include "components/version_info/version_info.h"

namespace android_webview {
namespace {

// Gets the version number to use for variations seed simulation. Must be called
// on a thread where IO is allowed.
base::Version GetVersionForSimulation() {
  base::AssertBlockingAllowed();
  return version_info::GetVersion();
}

}  // namespace

AwVariationsServiceClient::AwVariationsServiceClient() {}

AwVariationsServiceClient::~AwVariationsServiceClient() {}

std::string AwVariationsServiceClient::GetApplicationLocale() {
  return std::string();
}

base::Callback<base::Version(void)>
AwVariationsServiceClient::GetVersionForSimulationCallback() {
  return base::BindRepeating(&GetVersionForSimulation);
}

net::URLRequestContextGetter*
AwVariationsServiceClient::GetURLRequestContext() {
  return nullptr;
}

network_time::NetworkTimeTracker*
AwVariationsServiceClient::GetNetworkTimeTracker() {
  return nullptr;
}

version_info::Channel AwVariationsServiceClient::GetChannel() {
  return version_info::GetChannel();
}

bool AwVariationsServiceClient::OverridesRestrictParameter(
    std::string* parameter) {
  return false;
}

}  // namespace android_webview
