// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/advanced_firewall_manager_win.h"

#include <objbase.h>
#include <stddef.h>

#include "base/guid.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_bstr.h"
#include "base/win/scoped_variant.h"

namespace installer {

AdvancedFirewallManager::AdvancedFirewallManager() {}

AdvancedFirewallManager::~AdvancedFirewallManager() {}

bool AdvancedFirewallManager::Init(const base::string16& app_name,
                                   const base::FilePath& app_path) {
  firewall_rules_ = nullptr;
  HRESULT hr = ::CoCreateInstance(CLSID_NetFwPolicy2, nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&firewall_policy_));
  if (FAILED(hr)) {
    DLOG(ERROR) << logging::SystemErrorCodeToString(hr);
    firewall_policy_ = nullptr;
    return false;
  }
  hr = firewall_policy_->get_Rules(firewall_rules_.GetAddressOf());
  if (FAILED(hr)) {
    DLOG(ERROR) << logging::SystemErrorCodeToString(hr);
    firewall_rules_ = nullptr;
    return false;
  }
  app_name_ = app_name;
  app_path_ = app_path;
  return true;
}

bool AdvancedFirewallManager::IsFirewallEnabled() {
  long profile_types = 0;
  HRESULT hr = firewall_policy_->get_CurrentProfileTypes(&profile_types);
  if (FAILED(hr))
    return false;
  // The most-restrictive active profile takes precedence.
  const NET_FW_PROFILE_TYPE2 kProfileTypes[] = {
    NET_FW_PROFILE2_PUBLIC,
    NET_FW_PROFILE2_PRIVATE,
    NET_FW_PROFILE2_DOMAIN
  };
  for (size_t i = 0; i < arraysize(kProfileTypes); ++i) {
    if ((profile_types & kProfileTypes[i]) != 0) {
      VARIANT_BOOL enabled = VARIANT_TRUE;
      hr = firewall_policy_->get_FirewallEnabled(kProfileTypes[i], &enabled);
      // Assume the firewall is enabled if we can't determine.
      if (FAILED(hr) || enabled != VARIANT_FALSE)
        return true;
    }
  }
  return false;
}

bool AdvancedFirewallManager::HasAnyRule() {
  std::vector<Microsoft::WRL::ComPtr<INetFwRule>> rules;
  GetAllRules(&rules);
  return !rules.empty();
}

bool AdvancedFirewallManager::AddUDPRule(const base::string16& rule_name,
                                         const base::string16& description,
                                         uint16_t port) {
  // Delete the rule. According MDSN |INetFwRules::Add| should replace rule with
  // same "rule identifier". It's not clear what is "rule identifier", but it
  // can successfully create many rule with same name.
  DeleteRuleByName(rule_name);

  // Create the rule and add it to the rule set (only succeeds if elevated).
  Microsoft::WRL::ComPtr<INetFwRule> udp_rule =
      CreateUDPRule(rule_name, description, port);
  if (!udp_rule.Get())
    return false;

  HRESULT hr = firewall_rules_->Add(udp_rule.Get());
  DLOG_IF(ERROR, FAILED(hr)) << logging::SystemErrorCodeToString(hr);
  return SUCCEEDED(hr);
}

void AdvancedFirewallManager::DeleteRuleByName(
    const base::string16& rule_name) {
  std::vector<Microsoft::WRL::ComPtr<INetFwRule>> rules;
  GetAllRules(&rules);
  for (size_t i = 0; i < rules.size(); ++i) {
    base::win::ScopedBstr name;
    HRESULT hr = rules[i]->get_Name(name.Receive());
    if (SUCCEEDED(hr) && name && base::string16(name) == rule_name) {
      DeleteRule(rules[i]);
    }
  }
}

void AdvancedFirewallManager::DeleteRule(
    Microsoft::WRL::ComPtr<INetFwRule> rule) {
  // Rename rule to unique name and delete by unique name. We can't just delete
  // rule by name. Multiple rules with the same name and different app are
  // possible.
  base::win::ScopedBstr unique_name(
      base::UTF8ToUTF16(base::GenerateGUID()).c_str());
  rule->put_Name(unique_name);
  firewall_rules_->Remove(unique_name);
}

void AdvancedFirewallManager::DeleteAllRules() {
  std::vector<Microsoft::WRL::ComPtr<INetFwRule>> rules;
  GetAllRules(&rules);
  for (size_t i = 0; i < rules.size(); ++i) {
    DeleteRule(rules[i]);
  }
}

Microsoft::WRL::ComPtr<INetFwRule> AdvancedFirewallManager::CreateUDPRule(
    const base::string16& rule_name,
    const base::string16& description,
    uint16_t port) {
  Microsoft::WRL::ComPtr<INetFwRule> udp_rule;

  HRESULT hr = ::CoCreateInstance(CLSID_NetFwRule, nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&udp_rule));
  if (FAILED(hr)) {
    DLOG(ERROR) << logging::SystemErrorCodeToString(hr);
    return Microsoft::WRL::ComPtr<INetFwRule>();
  }

  udp_rule->put_Name(base::win::ScopedBstr(rule_name.c_str()));
  udp_rule->put_Description(base::win::ScopedBstr(description.c_str()));
  udp_rule->put_ApplicationName(
      base::win::ScopedBstr(app_path_.value().c_str()));
  udp_rule->put_Protocol(NET_FW_IP_PROTOCOL_UDP);
  udp_rule->put_Direction(NET_FW_RULE_DIR_IN);
  udp_rule->put_Enabled(VARIANT_TRUE);
  udp_rule->put_LocalPorts(
      base::win::ScopedBstr(base::StringPrintf(L"%u", port).c_str()));
  udp_rule->put_Grouping(base::win::ScopedBstr(app_name_.c_str()));
  udp_rule->put_Profiles(NET_FW_PROFILE2_ALL);
  udp_rule->put_Action(NET_FW_ACTION_ALLOW);

  return udp_rule;
}

void AdvancedFirewallManager::GetAllRules(
    std::vector<Microsoft::WRL::ComPtr<INetFwRule>>* rules) {
  Microsoft::WRL::ComPtr<IUnknown> rules_enum_unknown;
  HRESULT hr = firewall_rules_->get__NewEnum(rules_enum_unknown.GetAddressOf());
  if (FAILED(hr)) {
    DLOG(ERROR) << logging::SystemErrorCodeToString(hr);
    return;
  }

  Microsoft::WRL::ComPtr<IEnumVARIANT> rules_enum;
  hr = rules_enum_unknown.CopyTo(rules_enum.GetAddressOf());
  if (FAILED(hr)) {
    DLOG(ERROR) << logging::SystemErrorCodeToString(hr);
    return;
  }

  for (;;) {
    base::win::ScopedVariant rule_var;
    hr = rules_enum->Next(1, rule_var.Receive(), NULL);
    DLOG_IF(ERROR, FAILED(hr)) << logging::SystemErrorCodeToString(hr);
    if (hr != S_OK)
      break;
    DCHECK_EQ(VT_DISPATCH, rule_var.type());
    if (VT_DISPATCH != rule_var.type()) {
      DLOG(ERROR) << "Unexpected type";
      continue;
    }
    Microsoft::WRL::ComPtr<INetFwRule> rule;
    hr = V_DISPATCH(rule_var.ptr())->QueryInterface(IID_PPV_ARGS(&rule));
    if (FAILED(hr)) {
      DLOG(ERROR) << logging::SystemErrorCodeToString(hr);
      continue;
    }

    base::win::ScopedBstr path;
    hr = rule->get_ApplicationName(path.Receive());
    if (FAILED(hr)) {
      DLOG(ERROR) << logging::SystemErrorCodeToString(hr);
      continue;
    }

    if (!path ||
        !base::FilePath::CompareEqualIgnoreCase(static_cast<BSTR>(path),
                                                app_path_.value())) {
      continue;
    }

    rules->push_back(rule);
  }
}

}  // namespace installer
