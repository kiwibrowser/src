// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_CORS_CORS_LEGACY_H_
#define SERVICES_NETWORK_PUBLIC_CPP_CORS_CORS_LEGACY_H_

#include <vector>

#include "base/component_export.h"

namespace network {
namespace cors {

// Functions in namespace legacy are for legacy code path. Pure Network
// Service code should not use it. Since files in public/cpp/cors are shared
// between Network Service and legacy code path in content, but Network Service
// should not know content dependent concepts, we need to provide abstracted
// interfaces to implement extra checks for the case code runs in content.
//
// TODO(toyoshim): Remove all functions after Network Service is enabled.
namespace legacy {

// Registers whitelisted secure origins and hostname patterns for CORS checks in
// CORSURLLoader.
COMPONENT_EXPORT(NETWORK_CPP)
void RegisterSecureOrigins(const std::vector<std::string>& secure_origins);

// Refers the registered whitelisted secure origins and hostname patterns.
COMPONENT_EXPORT(NETWORK_CPP)
const std::vector<std::string>& GetSecureOrigins();

}  // namespace legacy

}  // namespace cors
}  // namespace network

#endif  // SERVICES_NETWORK_PUBLIC_CPP_CORS_CORS_LEGACY_H_
