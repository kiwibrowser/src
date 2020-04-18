// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/updating_app_registration_data.h"

#include "chrome/installer/util/google_update_constants.h"

UpdatingAppRegistrationData::UpdatingAppRegistrationData(
    const base::string16& app_guid) : app_guid_(app_guid) {}

UpdatingAppRegistrationData::~UpdatingAppRegistrationData() {}

base::string16 UpdatingAppRegistrationData::GetStateKey() const {
  return base::string16(google_update::kRegPathClientState)
      .append(1, L'\\')
      .append(app_guid_);
}

base::string16 UpdatingAppRegistrationData::GetStateMediumKey() const {
  return base::string16(google_update::kRegPathClientStateMedium)
      .append(1, L'\\')
      .append(app_guid_);
}

base::string16 UpdatingAppRegistrationData::GetVersionKey() const {
  return base::string16(google_update::kRegPathClients)
      .append(1, L'\\')
      .append(app_guid_);
}
