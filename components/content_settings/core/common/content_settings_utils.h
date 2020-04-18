// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UTILS_H_
#define COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UTILS_H_

#include <memory>

#include "components/content_settings/core/common/content_settings.h"

namespace base {
class Value;
}

namespace content_settings {

// Converts |value| to |ContentSetting|.
ContentSetting ValueToContentSetting(const base::Value* value);

// Returns a base::Value representation of |setting| if |setting| is
// a valid content setting. Otherwise, returns a nullptr.
std::unique_ptr<base::Value> ContentSettingToValue(ContentSetting setting);

}  // namespace content_settings

#endif  // COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UTILS_H_
