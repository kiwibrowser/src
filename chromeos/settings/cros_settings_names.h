// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SETTINGS_CROS_SETTINGS_NAMES_H_
#define CHROMEOS_SETTINGS_CROS_SETTINGS_NAMES_H_

#include "chromeos/chromeos_export.h"

namespace chromeos {

CHROMEOS_EXPORT extern const char kCrosSettingsPrefix[];

CHROMEOS_EXPORT extern const char kAccountsPrefAllowGuest[];
CHROMEOS_EXPORT extern const char kAccountsPrefAllowNewUser[];
CHROMEOS_EXPORT extern const char kAccountsPrefShowUserNamesOnSignIn[];
CHROMEOS_EXPORT extern const char kAccountsPrefUsers[];
CHROMEOS_EXPORT extern const char kAccountsPrefEphemeralUsersEnabled[];
CHROMEOS_EXPORT extern const char kAccountsPrefDeviceLocalAccounts[];
CHROMEOS_EXPORT extern const char kAccountsPrefDeviceLocalAccountsKeyId[];
CHROMEOS_EXPORT extern const char kAccountsPrefDeviceLocalAccountsKeyType[];
CHROMEOS_EXPORT extern const char
    kAccountsPrefDeviceLocalAccountsKeyKioskAppId[];
CHROMEOS_EXPORT extern const char
    kAccountsPrefDeviceLocalAccountsKeyKioskAppUpdateURL[];
CHROMEOS_EXPORT extern const char
    kAccountsPrefDeviceLocalAccountsKeyArcKioskPackage[];
CHROMEOS_EXPORT extern const char
    kAccountsPrefDeviceLocalAccountsKeyArcKioskClass[];
CHROMEOS_EXPORT extern const char
    kAccountsPrefDeviceLocalAccountsKeyArcKioskAction[];
CHROMEOS_EXPORT extern const char
    kAccountsPrefDeviceLocalAccountsKeyArcKioskDisplayName[];
CHROMEOS_EXPORT extern const char kAccountsPrefDeviceLocalAccountAutoLoginId[];
CHROMEOS_EXPORT extern const char
    kAccountsPrefDeviceLocalAccountAutoLoginDelay[];
CHROMEOS_EXPORT extern const char
    kAccountsPrefDeviceLocalAccountAutoLoginBailoutEnabled[];
CHROMEOS_EXPORT extern const char
    kAccountsPrefDeviceLocalAccountPromptForNetworkWhenOffline[];
CHROMEOS_EXPORT extern const char kAccountsPrefSupervisedUsersEnabled[];
CHROMEOS_EXPORT extern const char kAccountsPrefTransferSAMLCookies[];
CHROMEOS_EXPORT extern const char kAccountsPrefLoginScreenDomainAutoComplete[];

CHROMEOS_EXPORT extern const char kSignedDataRoamingEnabled[];

CHROMEOS_EXPORT extern const char kUpdateDisabled[];
CHROMEOS_EXPORT extern const char kTargetVersionPrefix[];
CHROMEOS_EXPORT extern const char kAllowedConnectionTypesForUpdate[];

CHROMEOS_EXPORT extern const char kSystemTimezonePolicy[];
CHROMEOS_EXPORT extern const char kSystemTimezone[];
CHROMEOS_EXPORT extern const char kSystemUse24HourClock[];

CHROMEOS_EXPORT extern const char kDeviceOwner[];

CHROMEOS_EXPORT extern const char kStatsReportingPref[];

CHROMEOS_EXPORT extern const char kReleaseChannel[];
CHROMEOS_EXPORT extern const char kReleaseChannelDelegated[];

CHROMEOS_EXPORT extern const char kReportDeviceVersionInfo[];
CHROMEOS_EXPORT extern const char kReportDeviceActivityTimes[];
CHROMEOS_EXPORT extern const char kReportDeviceBootMode[];
CHROMEOS_EXPORT extern const char kReportDeviceLocation[];
CHROMEOS_EXPORT extern const char kReportDeviceNetworkInterfaces[];
CHROMEOS_EXPORT extern const char kReportDeviceUsers[];
CHROMEOS_EXPORT extern const char kReportDeviceHardwareStatus[];
CHROMEOS_EXPORT extern const char kReportDeviceSessionStatus[];
CHROMEOS_EXPORT extern const char kReportOsUpdateStatus[];
CHROMEOS_EXPORT extern const char kReportRunningKioskApp[];
CHROMEOS_EXPORT extern const char kReportUploadFrequency[];

CHROMEOS_EXPORT extern const char kHeartbeatEnabled[];
CHROMEOS_EXPORT extern const char kHeartbeatFrequency[];

CHROMEOS_EXPORT extern const char kSystemLogUploadEnabled[];

CHROMEOS_EXPORT extern const char kPolicyMissingMitigationMode[];

CHROMEOS_EXPORT extern const char kAllowRedeemChromeOsRegistrationOffers[];

CHROMEOS_EXPORT extern const char kStartUpFlags[];

CHROMEOS_EXPORT extern const char kKioskAppSettingsPrefix[];
CHROMEOS_EXPORT extern const int kKioskAppSettingsPrefixLength;
CHROMEOS_EXPORT extern const char kKioskApps[];
CHROMEOS_EXPORT extern const char kKioskAutoLaunch[];
CHROMEOS_EXPORT extern const char kKioskDisableBailoutShortcut[];

CHROMEOS_EXPORT extern const char kVariationsRestrictParameter[];

CHROMEOS_EXPORT extern const char kDeviceAttestationEnabled[];
CHROMEOS_EXPORT extern const char kAttestationForContentProtectionEnabled[];

CHROMEOS_EXPORT extern const char kServiceAccountIdentity[];

CHROMEOS_EXPORT extern const char kDeviceDisabled[];
CHROMEOS_EXPORT extern const char kDeviceDisabledMessage[];

CHROMEOS_EXPORT extern const char kRebootOnShutdown[];

CHROMEOS_EXPORT extern const char kExtensionCacheSize[];

CHROMEOS_EXPORT extern const char kDisplayRotationDefault[];

CHROMEOS_EXPORT extern const char kLoginAuthenticationBehavior[];

CHROMEOS_EXPORT extern const char kAllowBluetooth[];

CHROMEOS_EXPORT extern const char kDeviceQuirksDownloadEnabled[];

CHROMEOS_EXPORT extern const char kLoginVideoCaptureAllowedUrls[];

CHROMEOS_EXPORT extern const char kDeviceLoginScreenAppInstallList[];

CHROMEOS_EXPORT extern const char kDeviceWallpaperImage[];

CHROMEOS_EXPORT extern const char kDeviceLoginScreenLocales[];
CHROMEOS_EXPORT extern const char kDeviceLoginScreenInputMethods[];

CHROMEOS_EXPORT extern const char kPerUserTimezoneEnabled[];
CHROMEOS_EXPORT extern const char kFineGrainedTimeZoneResolveEnabled[];

CHROMEOS_EXPORT extern const char kDeviceOffHours[];

CHROMEOS_EXPORT extern const char kDevicePrintersConfigurations[];
CHROMEOS_EXPORT extern const char kDevicePrintersAccessMode[];
CHROMEOS_EXPORT extern const char kDevicePrintersBlacklist[];
CHROMEOS_EXPORT extern const char kDevicePrintersWhitelist[];

CHROMEOS_EXPORT extern const char kTPMFirmwareUpdateSettings[];

CHROMEOS_EXPORT extern const char kMinimumRequiredChromeVersion[];

CHROMEOS_EXPORT extern const char kCastReceiverName[];

CHROMEOS_EXPORT extern const char kUnaffiliatedArcAllowed[];

CHROMEOS_EXPORT extern const char kDeviceHostnameTemplate[];

CHROMEOS_EXPORT extern const char kVirtualMachinesAllowed[];

CHROMEOS_EXPORT extern const char kSamlLoginAuthenticationType[];

}  // namespace chromeos

#endif  // CHROMEOS_SETTINGS_CROS_SETTINGS_NAMES_H_
