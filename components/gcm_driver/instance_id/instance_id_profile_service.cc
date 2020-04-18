// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/instance_id/instance_id_profile_service.h"

#include "base/logging.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/gcm_driver/gcm_profile_service.h"
#include "components/gcm_driver/instance_id/instance_id_driver.h"
#include "components/prefs/pref_service.h"

namespace instance_id {

// static
bool InstanceIDProfileService::IsInstanceIDEnabled(PrefService* prefs) {
  // Instance ID depends on GCM which has to been enabled.
  if (!gcm::GCMProfileService::IsGCMEnabled(prefs))
    return false;

  return InstanceIDDriver::IsInstanceIDEnabled();
}

InstanceIDProfileService::InstanceIDProfileService(gcm::GCMDriver* driver,
                                                   bool is_off_the_record) {
  DCHECK(!is_off_the_record);

  driver_ = std::make_unique<InstanceIDDriver>(driver);
}

InstanceIDProfileService::~InstanceIDProfileService() {}

}  // namespace instance_id
