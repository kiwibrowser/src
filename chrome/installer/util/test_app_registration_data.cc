// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/test_app_registration_data.h"

TestAppRegistrationData::TestAppRegistrationData() {
}

TestAppRegistrationData::~TestAppRegistrationData() {
}

base::string16 TestAppRegistrationData::GetStateKey() const {
  return L"Software\\Chromium\\ClientState\\test_app_guid";
}

base::string16 TestAppRegistrationData::GetStateMediumKey() const {
  return L"Software\\Chromium\\ClientStateMedium\\test_app_guid";
}

base::string16 TestAppRegistrationData::GetVersionKey() const {
  return L"Software\\Chromium\\Clients\\test_app_guid";
}
