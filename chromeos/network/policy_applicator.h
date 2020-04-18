// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_POLICY_APPLICATOR_H_
#define CHROMEOS_NETWORK_POLICY_APPLICATOR_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "chromeos/network/network_profile.h"

namespace chromeos {

// This class compares (entry point is Run()) |modified_policies| with the
// existing entries in the provided Shill profile |profile|. It fetches all
// entries in parallel (GetProfilePropertiesCallback), compares each entry with
// the current policies (GetEntryCallback) and adds all missing policies
// (~PolicyApplicator).
class PolicyApplicator {
 public:
  class ConfigurationHandler {
   public:
    virtual ~ConfigurationHandler() {}
    // Write the new configuration with the properties |shill_properties| to
    // Shill. This configuration comes from a policy. Any conflicting or
    // existing configuration for the same network will have been removed
    // before.
    virtual void CreateConfigurationFromPolicy(
        const base::DictionaryValue& shill_properties) = 0;

    virtual void UpdateExistingConfigurationWithPropertiesFromPolicy(
        const base::DictionaryValue& existing_properties,
        const base::DictionaryValue& new_properties) = 0;

    // Called after all policies for |profile| were applied. At this point, the
    // list of networks should be updated.
    virtual void OnPoliciesApplied(const NetworkProfile& profile) = 0;

   private:
    DISALLOW_ASSIGN(ConfigurationHandler);
  };

  using GuidToPolicyMap =
      std::map<std::string, std::unique_ptr<base::DictionaryValue>>;

  // |handler| must outlive this object.
  // |modified_policies| must not be NULL and will be empty afterwards.
  PolicyApplicator(const NetworkProfile& profile,
                   const GuidToPolicyMap& all_policies,
                   const base::DictionaryValue& global_network_config,
                   ConfigurationHandler* handler,
                   std::set<std::string>* modified_policies);

  ~PolicyApplicator();

  void Run();

 private:
  // Removes |entry| from the list of pending profile entries.
  // If all entries were processed, applies the remaining policies and notifies
  // |handler_|.
  void ProfileEntryFinished(const std::string& entry);

  // Called with the properties of the profile |profile_|. Requests the
  // properties of each entry, which are processed by GetEntryCallback.
  void GetProfilePropertiesCallback(
      const base::DictionaryValue& profile_properties);
  void GetProfilePropertiesError(const std::string& error_name,
                                 const std::string& error_message);

  // Called with the properties of the profile entry |entry|. Checks whether the
  // entry was previously managed, whether a current policy applies and then
  // either updates, deletes or not touches the entry.
  void GetEntryCallback(const std::string& entry,
                        const base::DictionaryValue& entry_properties);
  void GetEntryError(const std::string& entry,
                     const std::string& error_name,
                     const std::string& error_message);

  // Sends Shill the command to delete profile entry |entry| from |profile_|.
  void DeleteEntry(const std::string& entry);

  // Sends the Shill configuration |shill_dictionary| to Shill. If |write_later|
  // is true, the configuration is queued for sending until ~PolicyApplicator.
  void WriteNewShillConfiguration(const base::DictionaryValue& shill_dictionary,
                                  const base::DictionaryValue& policy,
                                  bool write_later);

  // Creates new entries for all remaining policies, i.e. for which no matching
  // Profile entry was found.
  // This should only be called if all profile entries were processed.
  void ApplyRemainingPolicies();

  // Called after all policies are applied or an error occurred. Notifies
  // |handler_|.
  void NotifyConfigurationHandlerAndFinish();

  std::set<std::string> remaining_policies_;
  std::set<std::string> pending_get_entry_calls_;
  ConfigurationHandler* handler_;
  NetworkProfile profile_;
  GuidToPolicyMap all_policies_;
  base::DictionaryValue global_network_config_;
  std::vector<std::unique_ptr<base::DictionaryValue>> new_shill_configurations_;
  base::WeakPtrFactory<PolicyApplicator> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PolicyApplicator);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_POLICY_APPLICATOR_H_
