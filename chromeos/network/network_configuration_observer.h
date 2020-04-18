// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_CONFIGURATION_OBSERVER_H_
#define CHROMEOS_NETWORK_NETWORK_CONFIGURATION_OBSERVER_H_

#include <string>

#include "base/macros.h"

namespace base {
class DictionaryValue;
}

namespace chromeos {

// Observer class for network configuration events.
class NetworkConfigurationObserver {
 public:
  // Indicates whether a configuration change is triggered by a user action,
  // by an extension or by policy.
  enum Source {
    // Triggered by a user action.
    SOURCE_USER_ACTION,
    // Triggered by an extension to install a configuration.
    SOURCE_EXTENSION_INSTALL,
    // Triggered by a policy.
    SOURCE_POLICY
  };

  // Called whenever a network configuration is created, or an existing
  // configuration is replaced (see comment for CreateConfiguration).
  // |service_path| provides the Shill current identifier for the network.
  // Use properties[GUID] to get the global unique identifier. |profile_path|
  // can be used to determine whether or not the network is shared.
  // |properties| contains the Shill properties that were passed to
  // NetworkConfigurationHandler::CreateConfiguration.
  virtual void OnConfigurationCreated(
      const std::string& service_path,
      const std::string& profile_path,
      const base::DictionaryValue& properties,
      Source source) = 0;

  // Called whenever a network configuration is removed. |service_path|
  // provides the Shill current identifier for the network. |guid| will be set
  // to the corresponding GUID for the network if known at the time of removal,
  // otherwise it will be empty.
  virtual void OnConfigurationRemoved(const std::string& service_path,
                                      const std::string& guid,
                                      Source source) = 0;

  // Called whenever network properties are set. |service_path| provides the
  // Shill current identifier for the network. |guid| will be set to the
  // corresponding GUID for the network. |set_properties| contains the Shill
  // properties that were passed to NetworkConfigurationHandler::SetProperties.
  virtual void OnPropertiesSet(const std::string& service_path,
                               const std::string& guid,
                               const base::DictionaryValue& set_properties,
                               Source source) = 0;

  // Called whenever the profile (e.g. shared or user) that a configuration is
  // associated with changes (see comment for OnConfigurationCreated).
  virtual void OnConfigurationProfileChanged(
      const std::string& service_path,
      const std::string& profile_path,
      Source source) = 0;

 protected:
  virtual ~NetworkConfigurationObserver() {}

 private:
  DISALLOW_ASSIGN(NetworkConfigurationObserver);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_CONFIGURATION_OBSERVER_H_
