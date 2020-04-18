// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/policy_applicator.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/shill_profile_client.h"
#include "chromeos/network/network_type_pattern.h"
#include "chromeos/network/network_ui_data.h"
#include "chromeos/network/onc/onc_signature.h"
#include "chromeos/network/onc/onc_translator.h"
#include "chromeos/network/policy_util.h"
#include "chromeos/network/shill_property_util.h"
#include "components/onc/onc_constants.h"
#include "dbus/object_path.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

void LogErrorMessage(const base::Location& from_where,
                     const std::string& error_name,
                     const std::string& error_message) {
  LOG(ERROR) << from_where.ToString() << ": " << error_message;
}

const base::DictionaryValue* GetByGUID(
    const PolicyApplicator::GuidToPolicyMap& policies,
    const std::string& guid) {
  auto it = policies.find(guid);
  if (it == policies.end())
    return NULL;
  return it->second.get();
}

}  // namespace

PolicyApplicator::PolicyApplicator(
    const NetworkProfile& profile,
    const GuidToPolicyMap& all_policies,
    const base::DictionaryValue& global_network_config,
    ConfigurationHandler* handler,
    std::set<std::string>* modified_policies)
    : handler_(handler), profile_(profile), weak_ptr_factory_(this) {
  global_network_config_.MergeDictionary(&global_network_config);
  remaining_policies_.swap(*modified_policies);
  for (const auto& policy_pair : all_policies) {
    all_policies_.insert(std::make_pair(policy_pair.first,
                                        policy_pair.second->CreateDeepCopy()));
  }
}

PolicyApplicator::~PolicyApplicator() {
  VLOG(1) << "Destroying PolicyApplicator for " << profile_.userhash;
}

void PolicyApplicator::Run() {
  DBusThreadManager::Get()->GetShillProfileClient()->GetProperties(
      dbus::ObjectPath(profile_.path),
      base::Bind(&PolicyApplicator::GetProfilePropertiesCallback,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&PolicyApplicator::GetProfilePropertiesError,
                 weak_ptr_factory_.GetWeakPtr()));
}

void PolicyApplicator::ProfileEntryFinished(const std::string& entry) {
  pending_get_entry_calls_.erase(entry);
  if (pending_get_entry_calls_.empty()) {
    ApplyRemainingPolicies();
    NotifyConfigurationHandlerAndFinish();
  }
}

void PolicyApplicator::GetProfilePropertiesCallback(
    const base::DictionaryValue& profile_properties) {
  VLOG(2) << "Received properties for profile " << profile_.ToDebugString();
  const base::ListValue* entries = NULL;
  if (!profile_properties.GetListWithoutPathExpansion(
           shill::kEntriesProperty, &entries)) {
    LOG(ERROR) << "Profile " << profile_.ToDebugString()
               << " doesn't contain the property "
               << shill::kEntriesProperty;
    NotifyConfigurationHandlerAndFinish();
    return;
  }

  for (base::ListValue::const_iterator it = entries->begin();
       it != entries->end(); ++it) {
    std::string entry;
    it->GetAsString(&entry);

    pending_get_entry_calls_.insert(entry);
    DBusThreadManager::Get()->GetShillProfileClient()->GetEntry(
        dbus::ObjectPath(profile_.path),
        entry,
        base::Bind(&PolicyApplicator::GetEntryCallback,
                   weak_ptr_factory_.GetWeakPtr(),
                   entry),
        base::Bind(&PolicyApplicator::GetEntryError,
                   weak_ptr_factory_.GetWeakPtr(),
                   entry));
  }
  if (pending_get_entry_calls_.empty()) {
    ApplyRemainingPolicies();
    NotifyConfigurationHandlerAndFinish();
  }
}

void PolicyApplicator::GetProfilePropertiesError(
    const std::string& error_name,
    const std::string& error_message) {
  LOG(ERROR) << "Could not retrieve properties of profile " << profile_.path
             << ": " << error_message;
  NotifyConfigurationHandlerAndFinish();
}

void PolicyApplicator::GetEntryCallback(
    const std::string& entry,
    const base::DictionaryValue& entry_properties) {
  VLOG(2) << "Received properties for entry " << entry << " of profile "
          << profile_.ToDebugString();

  std::unique_ptr<base::DictionaryValue> onc_part(
      onc::TranslateShillServiceToONCPart(
          entry_properties, ::onc::ONC_SOURCE_UNKNOWN,
          &onc::kNetworkWithStateSignature, nullptr /* network_state */));

  std::string old_guid;
  if (!onc_part->GetStringWithoutPathExpansion(::onc::network_config::kGUID,
                                               &old_guid)) {
    VLOG(1) << "Entry " << entry << " of profile " << profile_.ToDebugString()
            << " doesn't contain a GUID.";
    // This might be an entry of an older ChromeOS version. Assume it to be
    // unmanaged.
  }

  std::unique_ptr<NetworkUIData> ui_data =
      shill_property_util::GetUIDataFromProperties(entry_properties);
  if (!ui_data) {
    VLOG(1) << "Entry " << entry << " of profile " << profile_.ToDebugString()
            << " contains no or no valid UIData.";
    // This might be an entry of an older ChromeOS version. Assume it to be
    // unmanaged. It's an inconsistency if there is a GUID but no UIData, thus
    // clear the GUID just in case.
    old_guid.clear();
  }

  bool was_managed = !old_guid.empty() && ui_data &&
                     (ui_data->onc_source() ==
                          ::onc::ONC_SOURCE_DEVICE_POLICY ||
                      ui_data->onc_source() == ::onc::ONC_SOURCE_USER_POLICY);

  const base::DictionaryValue* new_policy = NULL;
  if (was_managed) {
    // If we have a GUID that might match a current policy, do a lookup using
    // that GUID at first. In particular this is necessary, as some networks
    // can't be matched to policies by properties (e.g. VPN).
    new_policy = GetByGUID(all_policies_, old_guid);
  }

  if (!new_policy) {
    // If we didn't find a policy by GUID, still a new policy might match.
    new_policy = policy_util::FindMatchingPolicy(all_policies_, *onc_part);
  }

  if (new_policy) {
    std::string new_guid;
    new_policy->GetStringWithoutPathExpansion(::onc::network_config::kGUID,
                                              &new_guid);

    VLOG_IF(1, was_managed && old_guid != new_guid)
        << "Updating configuration previously managed by policy " << old_guid
        << " with new policy " << new_guid << ".";
    VLOG_IF(1, !was_managed) << "Applying policy " << new_guid
                             << " to previously unmanaged "
                             << "configuration.";

    if (old_guid == new_guid &&
        remaining_policies_.find(new_guid) == remaining_policies_.end()) {
      VLOG(1) << "Not updating existing managed configuration with guid "
              << new_guid << " because the policy didn't change.";
    } else {
      const base::DictionaryValue* user_settings =
          ui_data ? ui_data->user_settings() : NULL;
      std::unique_ptr<base::DictionaryValue> new_shill_properties =
          policy_util::CreateShillConfiguration(profile_, new_guid,
                                                &global_network_config_,
                                                new_policy, user_settings);
      // A new policy has to be applied to this profile entry. In order to keep
      // implicit state of Shill like "connected successfully before", keep the
      // entry if a policy is reapplied (e.g. after reboot) or is updated.
      // However, some Shill properties are used to identify the network and
      // cannot be modified after initial configuration, so we have to delete
      // the profile entry in these cases. Also, keeping Shill's state if the
      // SSID changed might not be a good idea anyways. If the policy GUID
      // changed, or there was no policy before, we delete the entry at first to
      // ensure that no old configuration remains.
      if (old_guid == new_guid &&
          shill_property_util::DoIdentifyingPropertiesMatch(
              *new_shill_properties, entry_properties)) {
        VLOG(1) << "Updating previously managed configuration with the "
                << "updated policy " << new_guid << ".";
      } else {
        VLOG(1) << "Deleting profile entry before writing new policy "
                << new_guid << " because of identifying properties changed.";
        DeleteEntry(entry);
      }

      // In general, old entries should at first be deleted before new
      // configurations are written to prevent inconsistencies. Therefore, we
      // delay the writing of the new config here until ~PolicyApplicator.
      // E.g. one problematic case is if a policy { {GUID=X, SSID=Y} } is
      // applied to the profile entries
      // { ENTRY1 = {GUID=X, SSID=X, USER_SETTINGS=X},
      //   ENTRY2 = {SSID=Y, ... } }.
      // At first ENTRY1 and ENTRY2 should be removed, then the new config be
      // written and the result should be:
      // { {GUID=X, SSID=Y, USER_SETTINGS=X} }
      WriteNewShillConfiguration(
          *new_shill_properties, *new_policy, true /* write later */);
      remaining_policies_.erase(new_guid);
    }
  } else if (was_managed) {
    VLOG(1) << "Removing configuration previously managed by policy "
            << old_guid << ", because the policy was removed.";

    // Remove the entry, because the network was managed but isn't anymore.
    // Note: An alternative might be to preserve the user settings, but it's
    // unclear which values originating the policy should be removed.
    DeleteEntry(entry);
  } else {
    // The entry wasn't managed and doesn't match any current policy. Global
    // network settings have to be applied.
    base::DictionaryValue shill_properties_to_update;
    policy_util::SetShillPropertiesForGlobalPolicy(
        entry_properties, global_network_config_, &shill_properties_to_update);
    if (shill_properties_to_update.empty()) {
      VLOG(2) << "Ignore unmanaged entry.";
      // Calling a SetProperties of Shill with an empty dictionary is a no op.
    } else {
      VLOG(2) << "Apply global network config to unmanaged entry.";
      handler_->UpdateExistingConfigurationWithPropertiesFromPolicy(
          entry_properties, shill_properties_to_update);
    }
  }

  ProfileEntryFinished(entry);
}

void PolicyApplicator::GetEntryError(const std::string& entry,
                                     const std::string& error_name,
                                     const std::string& error_message) {
  LOG(ERROR) << "Could not retrieve entry " << entry << " of profile "
             << profile_.path << ": " << error_message;
  ProfileEntryFinished(entry);
}

void PolicyApplicator::DeleteEntry(const std::string& entry) {
  DBusThreadManager::Get()->GetShillProfileClient()->DeleteEntry(
      dbus::ObjectPath(profile_.path), entry, base::DoNothing(),
      base::Bind(&LogErrorMessage, FROM_HERE));
}

void PolicyApplicator::WriteNewShillConfiguration(
    const base::DictionaryValue& shill_dictionary,
    const base::DictionaryValue& policy,
    bool write_later) {
  // Ethernet (non EAP) settings, like GUID or UIData, cannot be stored per
  // user. Abort in that case.
  std::string type;
  policy.GetStringWithoutPathExpansion(::onc::network_config::kType, &type);
  if (type == ::onc::network_type::kEthernet &&
      profile_.type() == NetworkProfile::TYPE_USER) {
    const base::DictionaryValue* ethernet = NULL;
    policy.GetDictionaryWithoutPathExpansion(::onc::network_config::kEthernet,
                                             &ethernet);
    std::string auth;
    ethernet->GetStringWithoutPathExpansion(::onc::ethernet::kAuthentication,
                                            &auth);
    if (auth == ::onc::ethernet::kAuthenticationNone)
      return;
  }

  if (write_later)
    new_shill_configurations_.push_back(shill_dictionary.CreateDeepCopy());
  else
    handler_->CreateConfigurationFromPolicy(shill_dictionary);
}

void PolicyApplicator::ApplyRemainingPolicies() {
  DCHECK(pending_get_entry_calls_.empty());

  // Write all queued configurations now.
  for (const auto& configuration : new_shill_configurations_) {
    handler_->CreateConfigurationFromPolicy(*configuration);
  }
  new_shill_configurations_.clear();

  VLOG_IF(2, !remaining_policies_.empty())
      << "Create new managed network configurations in profile"
      << profile_.ToDebugString() << ".";

  // All profile entries were compared to policies. |remaining_policies_|
  // contains all modified policies that didn't match any entry. For these
  // remaining policies, new configurations have to be created.
  for (std::set<std::string>::iterator it = remaining_policies_.begin();
       it != remaining_policies_.end(); ++it) {
    const base::DictionaryValue* network_policy = GetByGUID(all_policies_, *it);
    DCHECK(network_policy);

    VLOG(1) << "Creating new configuration managed by policy " << *it
            << " in profile " << profile_.ToDebugString() << ".";

    std::unique_ptr<base::DictionaryValue> shill_dictionary =
        policy_util::CreateShillConfiguration(
            profile_, *it, &global_network_config_, network_policy,
            NULL /* no user settings */);
    WriteNewShillConfiguration(
        *shill_dictionary, *network_policy, false /* write now */);
  }
  remaining_policies_.clear();
}

void PolicyApplicator::NotifyConfigurationHandlerAndFinish() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  handler_->OnPoliciesApplied(profile_);
}

}  // namespace chromeos
