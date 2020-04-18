// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_ACCESSIBILITY_ARC_ACCESSIBILITY_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_ARC_ACCESSIBILITY_ARC_ACCESSIBILITY_UTIL_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace arc {
namespace mojom {
class AccessibilityNodeInfoData;
enum class AccessibilityBooleanProperty;
enum class AccessibilityIntProperty;
enum class AccessibilityStringProperty;
enum class AccessibilityIntListProperty;
enum class AccessibilityStringListProperty;
}  // namespace mojom

bool GetProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityBooleanProperty prop);
bool GetProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityIntProperty prop,
                 int32_t* out_value);
bool HasProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityStringProperty prop);
bool GetProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityStringProperty prop,
                 std::string* out_value);
bool GetProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityIntListProperty prop,
                 std::vector<int32_t>* out_value);
bool GetProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityStringListProperty prop,
                 std::vector<std::string>* out_value);

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_ACCESSIBILITY_ARC_ACCESSIBILITY_UTIL_H_
