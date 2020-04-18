// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/non_updating_app_registration_data.h"

NonUpdatingAppRegistrationData::NonUpdatingAppRegistrationData(
    const base::string16& key_path) : key_path_(key_path) {}

NonUpdatingAppRegistrationData::~NonUpdatingAppRegistrationData() {}

base::string16 NonUpdatingAppRegistrationData::GetStateKey() const {
  return key_path_;
}

base::string16 NonUpdatingAppRegistrationData::GetStateMediumKey() const {
  return key_path_;
}

base::string16 NonUpdatingAppRegistrationData::GetVersionKey() const {
  return key_path_;
}
