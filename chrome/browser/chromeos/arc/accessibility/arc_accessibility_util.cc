// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/accessibility/arc_accessibility_util.h"

#include "components/arc/common/accessibility_helper.mojom.h"

namespace arc {

bool GetProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityBooleanProperty prop) {
  if (!node->boolean_properties)
    return false;

  auto it = node->boolean_properties->find(prop);
  if (it == node->boolean_properties->end())
    return false;

  return it->second;
}

bool GetProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityIntProperty prop,
                 int32_t* out_value) {
  if (!node->int_properties)
    return false;

  auto it = node->int_properties->find(prop);
  if (it == node->int_properties->end())
    return false;

  *out_value = it->second;
  return true;
}

bool HasProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityStringProperty prop) {
  if (!node->string_properties)
    return false;

  auto it = node->string_properties->find(prop);
  return it != node->string_properties->end();
}

bool GetProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityStringProperty prop,
                 std::string* out_value) {
  if (!HasProperty(node, prop))
    return false;

  auto it = node->string_properties->find(prop);
  *out_value = it->second;
  return true;
}

bool GetProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityIntListProperty prop,
                 std::vector<int32_t>* out_value) {
  if (!node->int_list_properties)
    return false;

  auto it = node->int_list_properties->find(prop);
  if (it == node->int_list_properties->end())
    return false;

  *out_value = it->second;
  return true;
}

bool GetProperty(mojom::AccessibilityNodeInfoData* node,
                 mojom::AccessibilityStringListProperty prop,
                 std::vector<std::string>* out_value) {
  if (!node->string_list_properties)
    return false;

  auto it = node->string_list_properties->find(prop);
  if (it == node->string_list_properties->end())
    return false;

  *out_value = it->second;
  return true;
}

}  // namespace arc
