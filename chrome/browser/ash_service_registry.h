// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_SERVICE_REGISTRY_H_
#define CHROME_BROWSER_ASH_SERVICE_REGISTRY_H_

#include <string>

#include "content/public/browser/content_browser_client.h"

namespace ash_service_registry {

// Process group used for the ash service and the ui service. Visible for test.
constexpr char kAshAndUiProcessGroup[] = "ash_and_ui";

// Registers the set of Ash related services that run out of process.
void RegisterOutOfProcessServices(
    content::ContentBrowserClient::OutOfProcessServiceMap* services);

// Registers the set of Ash related services that run in process.
void RegisterInProcessServices(
    content::ContentBrowserClient::StaticServiceMap* services,
    content::ServiceManagerConnection* connection);

// Returns true if |name| identifies an Ash related service.
bool IsAshRelatedServiceName(const std::string& name);

// If |service_name| identifies an Ash related service, returns an arbitrary
// label that identifies the service or group of related services. Otherwise
// returns the empty string.
std::string GetAshRelatedServiceLabel(const std::string& service_name);

// Returns true if the browser should exit when service |name| quits.
bool ShouldTerminateOnServiceQuit(const std::string& name);

}  // namespace ash_service_registry

#endif  // CHROME_BROWSER_ASH_SERVICE_REGISTRY_H_
