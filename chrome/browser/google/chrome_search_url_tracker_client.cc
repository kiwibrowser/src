// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/google/chrome_search_url_tracker_client.h"

#include "base/command_line.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

ChromeSearchURLTrackerClient::ChromeSearchURLTrackerClient(Profile* profile)
    : profile_(profile) {
}

ChromeSearchURLTrackerClient::~ChromeSearchURLTrackerClient() {
}

bool ChromeSearchURLTrackerClient::IsBackgroundNetworkingEnabled() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableBackgroundNetworking);
}

PrefService* ChromeSearchURLTrackerClient::GetPrefs() {
  return profile_->GetPrefs();
}

network::mojom::URLLoaderFactory*
ChromeSearchURLTrackerClient::GetURLLoaderFactory() {
  return content::BrowserContext::GetDefaultStoragePartition(profile_)
      ->GetURLLoaderFactoryForBrowserProcess()
      .get();
}
