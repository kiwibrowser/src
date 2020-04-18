// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/shared_pref_registry.h"

#include "components/prefs/pref_registry.h"
#include "components/prefs/pref_store.h"
#include "services/preferences/scoped_pref_connection_builder.h"
#include "services/service_manager/public/cpp/identity.h"

namespace prefs {

class SharedPrefRegistry::PendingConnection {
 public:
  PendingConnection(SharedPrefRegistry* owner,
                    scoped_refptr<ScopedPrefConnectionBuilder> connection,
                    std::vector<std::string> foreign_prefs,
                    std::set<std::string> pending_prefs)
      : owner_(owner),
        connection_(std::move(connection)),
        foreign_prefs_(std::move(foreign_prefs)),
        pending_prefs_(std::move(pending_prefs)) {}

  PendingConnection(PendingConnection&& other) = default;
  PendingConnection& operator=(PendingConnection&& other) = default;

  bool NewPublicPrefsRegistered(const std::vector<std::string>& keys) {
    for (auto& key : keys) {
      pending_prefs_.erase(key);
    }
    if (pending_prefs_.empty()) {
      owner_->ProvideDefaultPrefs(connection_.get(), std::move(foreign_prefs_));
      return true;
    }
    return false;
  }

 private:
  SharedPrefRegistry* owner_;
  scoped_refptr<ScopedPrefConnectionBuilder> connection_;
  std::vector<std::string> foreign_prefs_;
  std::set<std::string> pending_prefs_;

  DISALLOW_COPY_AND_ASSIGN(PendingConnection);
};

SharedPrefRegistry::SharedPrefRegistry(scoped_refptr<PrefRegistry> registry)
    : registry_(std::move(registry)) {
  for (const auto& pref : *registry_) {
#if DCHECK_IS_ON()
    all_registered_pref_keys_.insert(pref.first);
#endif
    if (registry_->GetRegistrationFlags(pref.first) & PrefRegistry::PUBLIC)
      public_pref_keys_.insert(std::move(pref.first));
  }
}

SharedPrefRegistry::~SharedPrefRegistry() = default;

scoped_refptr<ScopedPrefConnectionBuilder>
SharedPrefRegistry::CreateConnectionBuilder(
    mojom::PrefRegistryPtr pref_registry,
    const service_manager::Identity& identity,
    mojom::PrefStoreConnector::ConnectCallback callback) {
  bool is_initial_connection = connected_services_.insert(identity).second;
#if DCHECK_IS_ON()
  if (is_initial_connection) {
    for (const auto& key : pref_registry->private_registrations) {
      bool inserted = all_registered_pref_keys_.insert(key).second;
      DCHECK(inserted) << "Multiple services claimed ownership of pref \""
                       << key << "\"";
    }
  }
#endif

  std::vector<std::string>& observed_prefs =
      pref_registry->private_registrations;
  observed_prefs.reserve(observed_prefs.size() +
                         pref_registry->foreign_registrations.size() +
                         pref_registry->public_registrations.size());

  std::set<std::string> remaining_foreign_registrations =
      GetPendingForeignPrefs(pref_registry->foreign_registrations,
                             &observed_prefs);

  ProcessPublicPrefs(std::move(pref_registry->public_registrations),
                     is_initial_connection, &observed_prefs);
#if DCHECK_IS_ON()
  if (is_initial_connection) {
    per_service_registered_pref_keys_.emplace(
        identity,
        std::set<std::string>(observed_prefs.begin(), observed_prefs.end()));
  } else {
    DCHECK(per_service_registered_pref_keys_[identity] ==
           std::set<std::string>(observed_prefs.begin(), observed_prefs.end()));
  }
#endif

  auto connection_builder = base::MakeRefCounted<ScopedPrefConnectionBuilder>(
      std::move(observed_prefs), std::move(callback));
  if (remaining_foreign_registrations.empty()) {
    ProvideDefaultPrefs(connection_builder.get(),
                        pref_registry->foreign_registrations);
  } else {
    pending_connections_.emplace_back(
        this, connection_builder, pref_registry->foreign_registrations,
        std::move(remaining_foreign_registrations));
  }
  return connection_builder;
}

std::set<std::string> SharedPrefRegistry::GetPendingForeignPrefs(
    std::vector<std::string> foreign_prefs,
    std::vector<std::string>* observed_prefs) const {
  std::set<std::string> pending_foreign_prefs;
  if (foreign_prefs.empty())
    return pending_foreign_prefs;

  observed_prefs->insert(observed_prefs->end(), foreign_prefs.begin(),
                         foreign_prefs.end());
  std::sort(foreign_prefs.begin(), foreign_prefs.end());
  std::set_difference(
      std::make_move_iterator(foreign_prefs.begin()),
      std::make_move_iterator(foreign_prefs.end()), public_pref_keys_.begin(),
      public_pref_keys_.end(),
      std::inserter(pending_foreign_prefs, pending_foreign_prefs.end()));
  return pending_foreign_prefs;
}

void SharedPrefRegistry::ProcessPublicPrefs(
    std::vector<mojom::PrefRegistrationPtr> public_pref_registrations,
    bool is_initial_connection,
    std::vector<std::string>* observed_prefs) {
  if (public_pref_registrations.empty())
    return;

  if (is_initial_connection) {
    std::vector<std::string> new_public_prefs;
    for (auto& registration : public_pref_registrations) {
      auto& key = registration->key;
      auto& default_value = registration->default_value;
#if DCHECK_IS_ON()
      bool inserted = all_registered_pref_keys_.insert(key).second;
      DCHECK(inserted) << "Multiple services claimed ownership of pref \""
                       << key << "\"";
#endif
      registry_->RegisterForeignPref(key);
      registry_->SetDefaultForeignPrefValue(
          key, base::Value::ToUniquePtrValue(std::move(default_value)),
          registration->flags);

      observed_prefs->push_back(key);
      new_public_prefs.push_back(key);
      public_pref_keys_.insert(std::move(key));
    }
    pending_connections_.erase(
        std::remove_if(pending_connections_.begin(), pending_connections_.end(),
                       [&](PendingConnection& connection) {
                         return connection.NewPublicPrefsRegistered(
                             new_public_prefs);
                       }),
        pending_connections_.end());
  } else {
    for (auto& registration : public_pref_registrations) {
      observed_prefs->push_back(registration->key);
#if DCHECK_IS_ON()
      const base::Value* existing_default_value = nullptr;
      DCHECK(registry_->defaults()->GetValue(registration->key,
                                             &existing_default_value));
      DCHECK_EQ(*existing_default_value, registration->default_value);
#endif
    }
  }
}

void SharedPrefRegistry::ProvideDefaultPrefs(
    ScopedPrefConnectionBuilder* connection,
    std::vector<std::string> foreign_prefs) {
  std::vector<mojom::PrefRegistrationPtr> defaults;
  for (const auto& key : foreign_prefs) {
    const base::Value* value = nullptr;
    registry_->defaults()->GetValue(key, &value);
    defaults.emplace_back(base::in_place, key, value->Clone(),
                          registry_->GetRegistrationFlags(key));
  }

  connection->ProvideDefaults(std::move(defaults));
}

}  // namespace prefs
