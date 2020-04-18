// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_LOGIN_MANAGER_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_LOGIN_MANAGER_DBUS_CONSTANTS_H_

namespace login_manager {
const char kSessionManagerInterface[] = "org.chromium.SessionManagerInterface";
const char kSessionManagerServicePath[] = "/org/chromium/SessionManager";
const char kSessionManagerServiceName[] = "org.chromium.SessionManager";
// Methods
const char kSessionManagerEmitLoginPromptVisible[] = "EmitLoginPromptVisible";
const char kSessionManagerEmitAshInitialized[] = "EmitAshInitialized";
const char kSessionManagerEnableChromeTesting[] = "EnableChromeTesting";
const char kSessionManagerSaveLoginPassword[] = "SaveLoginPassword";
const char kSessionManagerStartSession[] = "StartSession";
const char kSessionManagerStopSession[] = "StopSession";
const char kSessionManagerRestartJob[] = "RestartJob";
const char kSessionManagerStorePolicy[] = "StorePolicy";
const char kSessionManagerStorePolicyEx[] = "StorePolicyEx";
const char kSessionManagerStoreUnsignedPolicy[] = "StoreUnsignedPolicy";
const char kSessionManagerStoreUnsignedPolicyEx[] = "StoreUnsignedPolicyEx";
const char kSessionManagerRetrievePolicy[] = "RetrievePolicy";
const char kSessionManagerRetrievePolicyEx[] = "RetrievePolicyEx";
const char kSessionManagerStorePolicyForUser[] = "StorePolicyForUser";
const char kSessionManagerStoreUnsignedPolicyForUser[] =
    "StoreUnsignedPolicyForUser";
const char kSessionManagerRetrievePolicyForUser[] = "RetrievePolicyForUser";
const char kSessionManagerRetrievePolicyForUserWithoutSession[] =
    "RetrievePolicyForUserWithoutSession";
const char kSessionManagerStoreDeviceLocalAccountPolicy[] =
    "StoreDeviceLocalAccountPolicy";
const char kSessionManagerRetrieveDeviceLocalAccountPolicy[] =
    "RetrieveDeviceLocalAccountPolicy";
const char kSessionManagerRetrieveSessionState[] = "RetrieveSessionState";
const char kSessionManagerRetrieveActiveSessions[] = "RetrieveActiveSessions";
const char kSessionManagerStartTPMFirmwareUpdate[] = "StartTPMFirmwareUpdate";
const char kSessionManagerStartDeviceWipe[] = "StartDeviceWipe";
const char kSessionManagerHandleSupervisedUserCreationStarting[] =
    "HandleSupervisedUserCreationStarting";
const char kSessionManagerHandleSupervisedUserCreationFinished[] =
    "HandleSupervisedUserCreationFinished";
const char kSessionManagerLockScreen[] = "LockScreen";
const char kSessionManagerHandleLockScreenShown[] = "HandleLockScreenShown";
const char kSessionManagerHandleLockScreenDismissed[] =
    "HandleLockScreenDismissed";
const char kSessionManagerSetFlagsForUser[] = "SetFlagsForUser";
const char kSessionManagerGetServerBackedStateKeys[] =
    "GetServerBackedStateKeys";
const char kSessionManagerInitMachineInfo[] = "InitMachineInfo";
const char kSessionManagerCheckArcAvailability[] = "CheckArcAvailability";
const char kSessionManagerStartArcInstance[] = "StartArcInstance";
const char kSessionManagerStartArcMiniContainer[] = "StartArcMiniContainer";
const char kSessionManagerUpgradeArcContainer[] = "UpgradeArcContainer";
const char kSessionManagerStopArcInstance[] = "StopArcInstance";
const char kSessionManagerSetArcCpuRestriction[] = "SetArcCpuRestriction";
const char kSessionManagerEmitArcBooted[] = "EmitArcBooted";
const char kSessionManagerGetArcStartTimeTicks[] = "GetArcStartTimeTicks";
const char kSessionManagerRemoveArcData[] = "RemoveArcData";
const char kSessionManagerStartContainer[] = "StartContainer";
const char kSessionManagerStopContainer[] = "StopContainer";
// Signals
const char kLoginPromptVisibleSignal[] = "LoginPromptVisible";
const char kSessionStateChangedSignal[] = "SessionStateChanged";
// ScreenLock signals.
const char kScreenIsLockedSignal[] = "ScreenIsLocked";
const char kScreenIsUnlockedSignal[] = "ScreenIsUnlocked";
// Ownership API signals.
const char kOwnerKeySetSignal[] = "SetOwnerKeyComplete";
const char kPropertyChangeCompleteSignal[] = "PropertyChangeComplete";
// ARC instance signals.
const char kArcInstanceStopped[] = "ArcInstanceStopped";
const char kArcInstanceRebooted[] = "ArcInstanceRebooted";

// D-Bus error codes
namespace dbus_error {
#define INTERFACE "org.chromium.SessionManagerInterface"

const char kNone[] = INTERFACE ".None";
const char kInvalidParameter[] = INTERFACE ".InvalidParameter";
const char kArcCpuCgroupFail[] = INTERFACE ".ArcCpuCgroupFail";
const char kArcInstanceRunning[] = INTERFACE ".ArcInstanceRunning";
const char kArcContainerNotFound[] = INTERFACE ".ArcContainerNotFound";
const char kContainerStartupFail[] = INTERFACE ".ContainerStartupFail";
const char kContainerShutdownFail[] = INTERFACE ".ContainerShutdownFail";
const char kEmitFailed[] = INTERFACE ".EmitFailed";
const char kGetServiceFail[] = INTERFACE ".kGetServiceFail";
const char kInitMachineInfoFail[] = INTERFACE ".InitMachineInfoFail";
const char kInvalidAccount[] = INTERFACE ".InvalidAccount";
const char kLowFreeDisk[] = INTERFACE ".LowFreeDisk";
const char kNoOwnerKey[] = INTERFACE ".NoOwnerKey";
const char kNoUserNssDb[] = INTERFACE ".NoUserNssDb";
const char kNotAvailable[] = INTERFACE ".NotAvailable";
const char kNotStarted[] = INTERFACE ".NotStarted";
const char kPolicyInitFail[] = INTERFACE ".PolicyInitFail";
const char kPubkeySetIllegal[] = INTERFACE ".PubkeySetIllegal";
const char kPolicySignatureRequired[] = INTERFACE ".PolicySignatureRequired";
const char kSessionDoesNotExist[] = INTERFACE ".SessionDoesNotExist";
const char kSessionExists[] = INTERFACE ".SessionExists";
const char kSigDecodeFail[] = INTERFACE ".SigDecodeFail";
const char kSigEncodeFail[] = INTERFACE ".SigEncodeFail";
const char kTestingChannelError[] = INTERFACE ".TestingChannelError";
const char kUnknownPid[] = INTERFACE ".UnknownPid";
const char kVerifyFail[] = INTERFACE ".VerifyFail";
const char kVpdUpdateFailed[] = INTERFACE ".VpdUpdateFailed";

#undef INTERFACE
}  // namespace dbus_error

// Values
enum ContainerCpuRestrictionState {
  CONTAINER_CPU_RESTRICTION_FOREGROUND = 0,
  CONTAINER_CPU_RESTRICTION_BACKGROUND = 1,
  NUM_CONTAINER_CPU_RESTRICTION_STATES = 2,
};

enum class ArcContainerStopReason {
  // The ARC container is crashed.
  CRASH = 0,

  // Stopped by the user request, e.g. disabling ARC.
  USER_REQUEST = 1,

  // Session manager is shut down. So, ARC is also shut down along with it.
  SESSION_MANAGER_SHUTDOWN = 2,

  // Browser was shut down. ARC is also shut down along with it.
  BROWSER_SHUTDOWN = 3,

  // Disk space is too small to upgrade ARC.
  LOW_DISK_SPACE = 4,

  // Failed to upgrade ARC mini container into full container.
  // Note that this will be used if the reason is other than low-disk-space.
  UPGRADE_FAILURE = 5,
};

}  // namespace login_manager

#endif  // SYSTEM_API_DBUS_LOGIN_MANAGER_DBUS_CONSTANTS_H_
