// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/vm_applications_service_provider_delegate.h"

#include "chrome/browser/chromeos/crostini/crostini_registry_service.h"
#include "chrome/browser/chromeos/crostini/crostini_registry_service_factory.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile_manager.h"

namespace chromeos {

VmApplicationsServiceProviderDelegate::VmApplicationsServiceProviderDelegate() =
    default;

VmApplicationsServiceProviderDelegate::
    ~VmApplicationsServiceProviderDelegate() = default;

void VmApplicationsServiceProviderDelegate::UpdateApplicationList(
    const vm_tools::apps::ApplicationList& app_list) {
  Profile* profile = ProfileManager::GetPrimaryUserProfile();
  if (!ProfileHelper::IsPrimaryProfile(profile))
    return;

  crostini::CrostiniRegistryService* registry_service =
      crostini::CrostiniRegistryServiceFactory::GetForProfile(profile);
  registry_service->UpdateApplicationList(app_list);
}

}  // namespace chromeos
