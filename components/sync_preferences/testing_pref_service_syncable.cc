// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_preferences/testing_pref_service_syncable.h"

#include <memory>

#include "base/bind.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_notifier_impl.h"
#include "components/prefs/pref_value_store.h"
#include "testing/gtest/include/gtest/gtest.h"

template <>
TestingPrefServiceBase<sync_preferences::PrefServiceSyncable,
                       user_prefs::PrefRegistrySyncable>::
    TestingPrefServiceBase(TestingPrefStore* managed_prefs,
                           TestingPrefStore* extension_prefs,
                           TestingPrefStore* user_prefs,
                           TestingPrefStore* recommended_prefs,
                           user_prefs::PrefRegistrySyncable* pref_registry,
                           PrefNotifierImpl* pref_notifier)
    : sync_preferences::PrefServiceSyncable(
          std::unique_ptr<PrefNotifierImpl>(pref_notifier),
          std::make_unique<PrefValueStore>(managed_prefs,
                                           nullptr,  // supervised_user_prefs
                                           extension_prefs,  // extension_prefs
                                           nullptr,  // command_line_prefs
                                           user_prefs,
                                           recommended_prefs,
                                           pref_registry->defaults().get(),
                                           pref_notifier),
          user_prefs,
          pref_registry,
          /*pref_model_associator_client=*/nullptr,
          base::Bind(&TestingPrefServiceBase<
                     PrefServiceSyncable,
                     user_prefs::PrefRegistrySyncable>::HandleReadError),
          false),
      managed_prefs_(managed_prefs),
      extension_prefs_(extension_prefs),
      user_prefs_(user_prefs),
      recommended_prefs_(recommended_prefs) {}

namespace sync_preferences {

TestingPrefServiceSyncable::TestingPrefServiceSyncable()
    : TestingPrefServiceBase<PrefServiceSyncable,
                             user_prefs::PrefRegistrySyncable>(
          new TestingPrefStore(),
          new TestingPrefStore(),
          new TestingPrefStore(),
          new TestingPrefStore(),
          new user_prefs::PrefRegistrySyncable(),
          new PrefNotifierImpl()) {}

TestingPrefServiceSyncable::TestingPrefServiceSyncable(
    TestingPrefStore* managed_prefs,
    TestingPrefStore* extension_prefs,
    TestingPrefStore* user_prefs,
    TestingPrefStore* recommended_prefs,
    user_prefs::PrefRegistrySyncable* pref_registry,
    PrefNotifierImpl* pref_notifier)
    : TestingPrefServiceBase<PrefServiceSyncable,
                             user_prefs::PrefRegistrySyncable>(
          managed_prefs,
          extension_prefs,
          user_prefs,
          recommended_prefs,
          pref_registry,
          pref_notifier) {}

TestingPrefServiceSyncable::~TestingPrefServiceSyncable() {}

user_prefs::PrefRegistrySyncable* TestingPrefServiceSyncable::registry() {
  return static_cast<user_prefs::PrefRegistrySyncable*>(
      DeprecatedGetPrefRegistry());
}

}  // namespace sync_preferences
