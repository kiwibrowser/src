// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_UPDATING_APP_REGISTRATION_DATA_H_
#define CHROME_INSTALLER_UTIL_UPDATING_APP_REGISTRATION_DATA_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/installer/util/app_registration_data.h"

// Registration data for an app that is updated by Google Update.
class UpdatingAppRegistrationData : public AppRegistrationData {
 public:
  explicit UpdatingAppRegistrationData(const base::string16& app_guid);
  ~UpdatingAppRegistrationData() override;
  base::string16 GetStateKey() const override;
  base::string16 GetStateMediumKey() const override;
  base::string16 GetVersionKey() const override;

 private:
  const base::string16 app_guid_;

  DISALLOW_COPY_AND_ASSIGN(UpdatingAppRegistrationData);
};

#endif  // CHROME_INSTALLER_UTIL_UPDATING_APP_REGISTRATION_DATA_H_
