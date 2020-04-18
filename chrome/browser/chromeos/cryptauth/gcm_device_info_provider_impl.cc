// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/cryptauth/gcm_device_info_provider_impl.h"

#include "base/linux_util.h"
#include "base/no_destructor.h"
#include "base/sys_info.h"
#include "base/version.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/chromeos/cryptauth/cryptauth_device_id_provider_impl.h"
#include "components/cryptauth/cryptauth_enrollment_utils.h"
#include "components/version_info/version_info.h"

namespace chromeos {

// static
const GcmDeviceInfoProviderImpl* GcmDeviceInfoProviderImpl::GetInstance() {
  static const base::NoDestructor<GcmDeviceInfoProviderImpl> provider;
  return provider.get();
}

const cryptauth::GcmDeviceInfo& GcmDeviceInfoProviderImpl::GetGcmDeviceInfo()
    const {
  static const base::NoDestructor<cryptauth::GcmDeviceInfo> gcm_device_info([] {
    static const google::protobuf::int64 kSoftwareVersionCode =
        cryptauth::HashStringToInt64(version_info::GetLastChange());

    cryptauth::GcmDeviceInfo gcm_device_info;

    gcm_device_info.set_long_device_id(
        cryptauth::CryptAuthDeviceIdProviderImpl::GetInstance()->GetDeviceId());
    gcm_device_info.set_device_type(cryptauth::CHROME);
    gcm_device_info.set_device_software_version(
        version_info::GetVersionNumber());
    gcm_device_info.set_device_software_version_code(kSoftwareVersionCode);
    gcm_device_info.set_locale(
        ChromeContentBrowserClient().GetApplicationLocale());
    gcm_device_info.set_device_model(base::SysInfo::GetLsbReleaseBoard());
    gcm_device_info.set_device_os_version(base::GetLinuxDistro());
    // The Chrome OS version tracks the Chrome version, so fill in the same
    // value as |device_kSoftwareVersionCode|.
    gcm_device_info.set_device_os_version_code(kSoftwareVersionCode);
    // |device_display_diagonal_mils| is unused because it only applies to
    // phones/tablets, but it must be set due to server API verification.
    gcm_device_info.set_device_display_diagonal_mils(0);

    // Set all supported features.
    gcm_device_info.add_supported_software_features(
        cryptauth::SoftwareFeature::BETTER_TOGETHER_CLIENT);
    gcm_device_info.add_supported_software_features(
        cryptauth::SoftwareFeature::EASY_UNLOCK_CLIENT);
    gcm_device_info.add_supported_software_features(
        cryptauth::SoftwareFeature::MAGIC_TETHER_CLIENT);
    gcm_device_info.add_supported_software_features(
        cryptauth::SoftwareFeature::SMS_CONNECT_CLIENT);

    return gcm_device_info;
  }());

  return *gcm_device_info;
}

GcmDeviceInfoProviderImpl::GcmDeviceInfoProviderImpl() = default;

}  // namespace chromeos
