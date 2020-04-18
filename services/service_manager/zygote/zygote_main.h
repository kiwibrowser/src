// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_ZYGOTE_ZYGOTE_MAIN_H_
#define SERVICES_SERVICE_MANAGER_ZYGOTE_ZYGOTE_MAIN_H_

#include <memory>
#include <vector>

#include "base/component_export.h"
#include "build/build_config.h"

namespace service_manager {

class ZygoteForkDelegate;

// |delegate| must outlive this call.
COMPONENT_EXPORT(SERVICE_MANAGER_ZYGOTE)
bool ZygoteMain(
    std::vector<std::unique_ptr<ZygoteForkDelegate>> fork_delegates);

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_ZYGOTE_ZYGOTE_MAIN_H_
