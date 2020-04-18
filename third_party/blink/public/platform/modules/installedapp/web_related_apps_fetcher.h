// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INSTALLEDAPP_WEB_RELATED_APPS_FETCHER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INSTALLEDAPP_WEB_RELATED_APPS_FETCHER_H_

#include "third_party/blink/public/platform/modules/installedapp/web_related_application.h"
#include "third_party/blink/public/platform/web_callbacks.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_vector.h"

#include <memory>

namespace blink {

using AppInstalledCallbacks =
    WebCallbacks<const WebVector<WebRelatedApplication>&, void>;

class WebRelatedAppsFetcher {
 public:
  virtual ~WebRelatedAppsFetcher() = default;

  // Gets the list of related applications from the web frame's manifest.
  virtual void GetManifestRelatedApplications(
      std::unique_ptr<AppInstalledCallbacks>) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INSTALLEDAPP_WEB_RELATED_APPS_FETCHER_H_
