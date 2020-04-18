// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/drive_test_util.h"

#include "components/drive/drive.pb.h"
#include "components/drive/drive_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"

namespace drive {
namespace test_util {

void RegisterDrivePrefs(PrefRegistrySimple* pref_registry) {
  pref_registry->RegisterBooleanPref(
      prefs::kDisableDrive,
      false);
  pref_registry->RegisterBooleanPref(
      prefs::kDisableDriveOverCellular,
      true);
  pref_registry->RegisterBooleanPref(
      prefs::kDisableDriveHostedFiles,
      false);
}

FakeNetworkChangeNotifier::FakeNetworkChangeNotifier()
    : type_(CONNECTION_WIFI) {
}

void FakeNetworkChangeNotifier::SetConnectionType(ConnectionType type) {
  type_ = type;
  NotifyObserversOfConnectionTypeChange();
}

net::NetworkChangeNotifier::ConnectionType
FakeNetworkChangeNotifier::GetCurrentConnectionType() const {
  return type_;
}

}  // namespace test_util
}  // namespace drive
