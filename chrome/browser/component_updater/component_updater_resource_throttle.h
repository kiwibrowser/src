// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_COMPONENT_UPDATER_RESOURCE_THROTTLE_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_COMPONENT_UPDATER_RESOURCE_THROTTLE_H_

#include <string>

namespace content {
class ResourceThrottle;
}

namespace component_updater {

class ComponentUpdateService;

// Returns a network resource throttle. It means that a component will be
// downloaded and installed before the resource is unthrottled. This function
// can be called from the IO thread.
content::ResourceThrottle* GetOnDemandResourceThrottle(
    ComponentUpdateService* cus,
    const std::string& crx_id);

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_COMPONENT_UPDATER_RESOURCE_THROTTLE_H_
