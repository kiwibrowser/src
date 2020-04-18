// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/accessibility_tree_formatter_browser.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/browser/accessibility/browser_accessibility_auralinux.h"
#include "ui/accessibility/platform/ax_platform_node_auralinux.h"

namespace content {

class AccessibilityTreeFormatterAuraLinux
    : public AccessibilityTreeFormatterBrowser {
 public:
  explicit AccessibilityTreeFormatterAuraLinux();
  ~AccessibilityTreeFormatterAuraLinux() override;

 private:
  const base::FilePath::StringType GetExpectedFileSuffix() override;
  const std::string GetAllowEmptyString() override;
  const std::string GetAllowString() override;
  const std::string GetDenyString() override;
  void AddProperties(const BrowserAccessibility& node,
                     base::DictionaryValue* dict) override;
  base::string16 ProcessTreeForOutput(
      const base::DictionaryValue& node,
      base::DictionaryValue* filtered_dict_result = nullptr) override;
};

// static
AccessibilityTreeFormatter* AccessibilityTreeFormatter::Create() {
  return new AccessibilityTreeFormatterAuraLinux();
}

AccessibilityTreeFormatterAuraLinux::AccessibilityTreeFormatterAuraLinux() {
}

AccessibilityTreeFormatterAuraLinux::~AccessibilityTreeFormatterAuraLinux() {
}

void AccessibilityTreeFormatterAuraLinux::AddProperties(
    const BrowserAccessibility& node,
    base::DictionaryValue* dict) {
  dict->SetInteger("id", node.GetId());
  BrowserAccessibilityAuraLinux* acc_obj =
      ToBrowserAccessibilityAuraLinux(const_cast<BrowserAccessibility*>(&node));

  acc_obj->GetNode()->AddAccessibilityTreeProperties(dict);
}

base::string16 AccessibilityTreeFormatterAuraLinux::ProcessTreeForOutput(
    const base::DictionaryValue& node,
    base::DictionaryValue* filtered_dict_result) {
  base::string16 error_value;
  if (node.GetString("error", &error_value))
    return error_value;

  base::string16 line;
  std::string role_value;
  node.GetString("role", &role_value);
  if (!role_value.empty()) {
    WriteAttribute(true, base::StringPrintf("[%s]", role_value.c_str()), &line);
  }

  std::string name_value;
  if (node.GetString("name", &name_value))
    WriteAttribute(true, base::StringPrintf("name='%s'", name_value.c_str()),
                   &line);

  std::string description_value;
  node.GetString("description", &description_value);
  WriteAttribute(
      false, base::StringPrintf("description='%s'", description_value.c_str()),
      &line);

  const base::ListValue* states_value;
  node.GetList("states", &states_value);
  for (base::ListValue::const_iterator it = states_value->begin();
       it != states_value->end(); ++it) {
    std::string state_value;
    if (it->GetAsString(&state_value))
      WriteAttribute(true, state_value, &line);
  }

  int id_value;
  node.GetInteger("id", &id_value);
  WriteAttribute(false, base::StringPrintf("id=%d", id_value), &line);

  return line + base::ASCIIToUTF16("\n");
}

const base::FilePath::StringType
AccessibilityTreeFormatterAuraLinux::GetExpectedFileSuffix() {
  return FILE_PATH_LITERAL("-expected-auralinux.txt");
}

const std::string AccessibilityTreeFormatterAuraLinux::GetAllowEmptyString() {
  return "@AURALINUX-ALLOW-EMPTY:";
}

const std::string AccessibilityTreeFormatterAuraLinux::GetAllowString() {
  return "@AURALINUX-ALLOW:";
}

const std::string AccessibilityTreeFormatterAuraLinux::GetDenyString() {
  return "@AURALINUX-DENY:";
}

}
