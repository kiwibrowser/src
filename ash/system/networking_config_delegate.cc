// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/networking_config_delegate.h"

namespace ash {

NetworkingConfigDelegate::ExtensionInfo::ExtensionInfo(const std::string& id,
                                                       const std::string& name)
    : extension_id(id), extension_name(name) {}

NetworkingConfigDelegate::ExtensionInfo::~ExtensionInfo() = default;

}  // namespace ash
