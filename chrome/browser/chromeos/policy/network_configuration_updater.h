// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_NETWORK_CONFIGURATION_UPDATER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_NETWORK_CONFIGURATION_UPDATER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/onc/onc_constants.h"
#include "components/policy/core/common/policy_service.h"

namespace base {
class DictionaryValue;
class ListValue;
class Value;
}

namespace chromeos {
class ManagedNetworkConfigurationHandler;
}

namespace policy {

class PolicyMap;

// Implements the common part of tracking a OpenNetworkConfiguration device or
// user policy. Pushes the network configs to the
// ManagedNetworkConfigurationHandler, which in turn writes configurations to
// Shill. Certificates are imported with the chromeos::onc::CertificateImporter.
// For user policies the subclass UserNetworkConfigurationUpdater must be used.
// Does not handle proxy settings.
class NetworkConfigurationUpdater : public PolicyService::Observer {
 public:
  ~NetworkConfigurationUpdater() override;

  // PolicyService::Observer overrides
  void OnPolicyUpdated(const PolicyNamespace& ns,
                       const PolicyMap& previous,
                       const PolicyMap& current) override;
  void OnPolicyServiceInitialized(PolicyDomain domain) override;

 protected:
  NetworkConfigurationUpdater(
      onc::ONCSource onc_source,
      std::string policy_key,
      PolicyService* policy_service,
      chromeos::ManagedNetworkConfigurationHandler* network_config_handler);

  virtual void Init();

  // Imports the certificates part of the policy.
  virtual void ImportCertificates(const base::ListValue& certificates_onc) = 0;

  // Pushes the network part of the policy to the
  // ManagedNetworkConfigurationHandler. This can be overridden by subclasses to
  // modify |network_configs_onc| before the actual application.
  virtual void ApplyNetworkPolicy(
      base::ListValue* network_configs_onc,
      base::DictionaryValue* global_network_config) = 0;

  // Parses the current value of the ONC policy. Clears |network_configs|,
  // |global_network_config| and |certificates| and fills them with the
  // validated NetworkConfigurations, GlobalNetworkConfiguration and
  // Certificates of the current policy. Callers can pass nullptr to any of
  // |network_configs|, |global_network_config|, |certificates| if they don't
  // need that specific part of the ONC policy.
  void ParseCurrentPolicy(base::ListValue* network_configs,
                          base::DictionaryValue* global_network_config,
                          base::ListValue* certificates);

  onc::ONCSource onc_source_;

  // Pointer to the global singleton or a test instance.
  chromeos::ManagedNetworkConfigurationHandler* network_config_handler_;

 private:
  // Called if the ONC policy changed.
  void OnPolicyChanged(const base::Value* previous, const base::Value* current);

  // Apply the observed policy, i.e. both networks and certificates.
  void ApplyPolicy();

  std::string LogHeader() const;

  std::string policy_key_;

  // Used to register for notifications from the |policy_service_|.
  PolicyChangeRegistrar policy_change_registrar_;

  // Used to retrieve the policies.
  PolicyService* policy_service_;

  DISALLOW_COPY_AND_ASSIGN(NetworkConfigurationUpdater);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_NETWORK_CONFIGURATION_UPDATER_H_
