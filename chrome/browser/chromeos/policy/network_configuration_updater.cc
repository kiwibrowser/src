// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/network_configuration_updater.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/values.h"
#include "chromeos/network/onc/onc_utils.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"

namespace policy {

NetworkConfigurationUpdater::~NetworkConfigurationUpdater() {
  policy_service_->RemoveObserver(POLICY_DOMAIN_CHROME, this);
}

void NetworkConfigurationUpdater::OnPolicyUpdated(const PolicyNamespace& ns,
                                                  const PolicyMap& previous,
                                                  const PolicyMap& current) {
  // Ignore this call. Policy changes are already observed by the registrar.
}

void NetworkConfigurationUpdater::OnPolicyServiceInitialized(
    PolicyDomain domain) {
  if (domain != POLICY_DOMAIN_CHROME)
    return;

  if (policy_service_->IsInitializationComplete(POLICY_DOMAIN_CHROME)) {
    VLOG(1) << LogHeader() << " initialized.";
    policy_service_->RemoveObserver(POLICY_DOMAIN_CHROME, this);
    ApplyPolicy();
  }
}

NetworkConfigurationUpdater::NetworkConfigurationUpdater(
    onc::ONCSource onc_source,
    std::string policy_key,
    PolicyService* policy_service,
    chromeos::ManagedNetworkConfigurationHandler* network_config_handler)
    : onc_source_(onc_source),
      network_config_handler_(network_config_handler),
      policy_key_(policy_key),
      policy_change_registrar_(policy_service,
                               PolicyNamespace(POLICY_DOMAIN_CHROME,
                                               std::string())),
      policy_service_(policy_service) {
}

void NetworkConfigurationUpdater::Init() {
  policy_change_registrar_.Observe(
      policy_key_,
      base::Bind(&NetworkConfigurationUpdater::OnPolicyChanged,
                 base::Unretained(this)));

  if (policy_service_->IsInitializationComplete(POLICY_DOMAIN_CHROME)) {
    VLOG(1) << LogHeader() << " is already initialized.";
    ApplyPolicy();
  } else {
    policy_service_->AddObserver(POLICY_DOMAIN_CHROME, this);
  }
}

void NetworkConfigurationUpdater::ParseCurrentPolicy(
    base::ListValue* network_configs,
    base::DictionaryValue* global_network_config,
    base::ListValue* certificates) {
  const PolicyMap& policies = policy_service_->GetPolicies(
      PolicyNamespace(POLICY_DOMAIN_CHROME, std::string()));
  const base::Value* policy_value = policies.GetValue(policy_key_);

  std::string onc_blob;
  if (!policy_value)
    VLOG(2) << LogHeader() << " is not set.";
  else if (!policy_value->GetAsString(&onc_blob))
    LOG(ERROR) << LogHeader() << " is not a string value.";

  chromeos::onc::ParseAndValidateOncForImport(
      onc_blob, onc_source_, std::string() /* no passphrase */, network_configs,
      global_network_config, certificates);
}

void NetworkConfigurationUpdater::OnPolicyChanged(const base::Value* previous,
                                                  const base::Value* current) {
  ApplyPolicy();
}

void NetworkConfigurationUpdater::ApplyPolicy() {
  base::ListValue network_configs;
  base::DictionaryValue global_network_config;
  base::ListValue certificates;
  ParseCurrentPolicy(&network_configs, &global_network_config, &certificates);

  ImportCertificates(certificates);
  ApplyNetworkPolicy(&network_configs, &global_network_config);
}

std::string NetworkConfigurationUpdater::LogHeader() const {
  return chromeos::onc::GetSourceAsString(onc_source_);
}

}  // namespace policy
