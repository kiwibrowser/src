// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines all the public base::FeatureList features for the chrome
// module.

#ifndef CHROME_COMMON_CHROME_FEATURES_H_
#define CHROME_COMMON_CHROME_FEATURES_H_

#include "base/feature_list.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "chrome/common/buildflags.h"
#include "device/vr/buildflags/buildflags.h"
#include "extensions/buildflags/buildflags.h"
#include "ppapi/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "ui/base/ui_features.h"

namespace features {

// All features in alphabetical order. The features should be documented
// alongside the definition of their values in the .cc file.

extern const base::Feature kAdsFeature;

#if defined(OS_ANDROID)
extern const base::Feature kAllowAutoplayUnmutedInWebappManifestScope;
#endif  // defined(OS_ANDROID)

#if defined(OS_MACOSX)
extern const base::Feature kAppleScriptExecuteJavaScriptMenuItem;
extern const base::Feature kShow10_9ObsoleteInfobar;
extern const base::Feature kViewsTaskManager;
#endif  // defined(OS_MACOSX)

#if !defined(OS_ANDROID)
extern const base::Feature kAnimatedAppMenuIcon;
extern const base::Feature kAppBanners;
#endif  // !defined(OS_ANDROID)

#if defined(OS_CHROMEOS)
extern const base::Feature kArcMemoryManagement;
#endif  // defined(OS_CHROMEOS)

extern const base::Feature kAssetDownloadSuggestionsFeature;

extern const base::Feature kAsyncDns;

#if defined(OS_WIN) || defined(OS_MACOSX)
extern const base::Feature kAutomaticTabDiscarding;
#endif  // defined(OS_WIN) || defined(OS_MACOSX)

#if defined(OS_WIN) || defined(OS_LINUX)
extern const base::Feature kBackgroundModeAllowRestart;
#endif  // defined(OS_WIN) || defined(OS_LINUX)

extern const base::Feature kBlockPromptsIfDismissedOften;
extern const base::Feature kBlockPromptsIfIgnoredOften;

#if defined(OS_MACOSX)
extern const base::Feature kBookmarkApps;
#endif

extern const base::Feature kBrowserHangFixesExperiment;

#if defined(OS_MACOSX)
extern const base::Feature kBrowserTouchBar;
#endif

extern const base::Feature kBundledConnectionHelpFeature;

#if defined(OS_MACOSX)
extern const base::Feature kDialogTouchBar;
extern const base::Feature kTabStripKeyboardFocus;
#endif  // defined(OS_MACOSX)

#if (defined(OS_LINUX) && !defined(OS_CHROMEOS)) || defined(OS_MACOSX)
extern const base::Feature kCertDualVerificationTrialFeature;
#endif

extern const base::Feature kChangePictureVideoMode;

#if defined(OS_ANDROID)
extern const base::Feature kClearOldBrowsingData;
#endif

extern const base::Feature kClickToOpenPDFPlaceholder;

extern const base::Feature kClipboardContentSetting;

#if defined(OS_MACOSX)
extern const base::Feature kContentFullscreen;
#endif

#if defined(OS_CHROMEOS)
extern const base::Feature kCrostini;
extern const base::Feature kUsageTimeLimitPolicy;
#endif

#if defined(OS_WIN)
extern const base::Feature kDesktopIOSPromotion;
#endif  // defined(OS_WIN)

extern const base::Feature kDesktopPWAWindowing;

extern const base::Feature kDesktopPWAsLinkCapturing;

extern const base::Feature kDisallowUnsafeHttpDownloads;
extern const char kDisallowUnsafeHttpDownloadsParamName[];

#if !defined(OS_ANDROID)
extern const base::Feature kDoodlesOnLocalNtp;
#endif

#if defined(OS_ANDROID)
extern const base::Feature kDownloadsForeground;
#endif

#if defined(OS_ANDROID)
extern const base::Feature kDownloadsLocationChange;
#endif

extern const base::Feature kExpectCTReporting;

extern const base::Feature kExperimentalAppBanners;

#if defined(OS_CHROMEOS)
extern const base::Feature kExperimentalCrostiniUI;
#endif

extern const base::Feature kExternalExtensionDefaultButtonControl;

// Android expects this string from Java code, so it is always needed.
// TODO(crbug.com/731802): Use #if BUILDFLAG(ENABLE_VR_BROWSING) instead.
#if BUILDFLAG(ENABLE_VR) || defined(OS_ANDROID)
extern const base::Feature kVrBrowsing;
#endif
#if BUILDFLAG(ENABLE_VR)
extern const base::Feature kVrBrowsingExperimentalFeatures;
extern const base::Feature kVrBrowsingExperimentalRendering;

#if BUILDFLAG(ENABLE_OCULUS_VR)
extern const base::Feature kOculusVR;
#endif  // ENABLE_OCULUS_VR

#if BUILDFLAG(ENABLE_OPENVR)
extern const base::Feature kOpenVR;
#endif  // ENABLE_OPENVR

#endif  // ENABLE_VR

extern const base::Feature kFullscreenExitUI;

#if defined(OS_MACOSX)
extern const base::Feature kFullscreenToolbarReveal;
#endif

#if defined(OS_WIN)
extern const base::Feature kGdiTextPrinting;
#endif

extern const base::Feature kGeoLanguage;

#if defined(OS_ANDROID)
extern const base::Feature kGrantNotificationsToDSE;
#endif

#if defined(OS_CHROMEOS)
extern const base::Feature kHappinessTrackingSystem;
#endif

#if !defined(OS_ANDROID)
extern const base::Feature kViewsCastDialog;
#endif

extern const base::Feature kImportantSitesInCbd;

extern const base::Feature kImprovedRecoveryComponent;

#if defined(OS_WIN) && defined(GOOGLE_CHROME_BUILD)
extern const base::Feature kIncompatibleApplicationsWarning;
#endif

#if !defined(OS_ANDROID)
extern const base::Feature kLocalScreenCasting;
#endif

extern const base::Feature kLsdPermissionPrompt;

#if defined(OS_MACOSX)
extern const base::Feature kMacRTL;
extern const base::Feature kMacFullSizeContentView;
extern const base::Feature kMacSystemShareMenu;
#endif

#if defined(OS_MACOSX)
extern const base::Feature kMacMaterialDesignDownloadShelf;
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
extern const base::Feature kMaterialDesignExtensions;
extern const base::Feature kAcknowledgeNtpOverrideOnDeactivate;
#endif

#if defined(OS_WIN) || (defined(OS_LINUX) && !defined(OS_CHROMEOS))
extern const base::Feature kWarnBeforeQuitting;
#endif

extern const base::Feature kMaterialDesignIncognitoNTP;

extern const base::Feature kModalPermissionPrompts;

#if defined(OS_WIN)
extern const base::Feature kModuleDatabase;
#endif

#if defined(OS_CHROMEOS)
extern const base::Feature kMultidevice;
extern const base::Feature kMultiDeviceApi;
#endif

#if BUILDFLAG(ENABLE_NATIVE_NOTIFICATIONS)
extern const base::Feature kNativeNotifications;
#endif

extern const base::Feature kNetworkPrediction;

#if defined(OS_POSIX)
extern const base::Feature kNtlmV2Enabled;
#endif

extern const base::Feature kNtpBackgrounds;

extern const base::Feature kNtpIcons;

extern const base::Feature kNtpUIMd;

extern const base::Feature kOfflinePageDownloadSuggestionsFeature;

#if defined(OS_ANDROID)
extern const base::Feature kOomIntervention;
#endif

#if !defined(OS_ANDROID)
extern const base::Feature kOneGoogleBarOnLocalNtp;
#endif

extern const base::Feature kUseNewAcceptLanguageHeader;

extern const base::Feature kPermissionDelegation;

#if defined(OS_WIN)
extern const base::Feature kDisablePostScriptPrinting;
#endif

#if !defined(OS_ANDROID)
extern const base::Feature kPolicyTool;
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
extern const base::Feature kPreferHtmlOverPlugins;
#endif

#if defined(OS_CHROMEOS)
extern const base::Feature kPreloadLockScreen;
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
extern const base::Feature kNewPrintPreview;
extern const base::Feature kNupPrinting;
#endif

extern const base::Feature kPushMessagingBackgroundMode;

#if !defined(OS_ANDROID)
extern const base::Feature kRemoveUsageOfDeprecatedGaiaSigninEndpoint;
#endif

extern const base::Feature kSafeSearchUrlReporting;

extern const base::Feature kSecurityKeyAttestationPrompt;

#if defined(OS_MACOSX)
extern const base::Feature kShowAllDialogsWithViewsToolkit;
#endif

#if defined(OS_ANDROID)
extern const base::Feature kShowTrustedPublisherURL;
#endif

extern const base::Feature kSiteSettings;

#if defined(OS_ANDROID)
extern const base::Feature kSiteNotificationChannels;
#endif

extern const base::Feature kSitePerProcess;

extern const base::Feature kSitePerProcessOnlyForHighMemoryClients;
extern const char kSitePerProcessOnlyForHighMemoryClientsParamName[];

#if defined(OS_CHROMEOS)
extern const base::Feature kNativeSmb;
#endif

extern const base::Feature kSoundContentSetting;

extern const base::Feature kSupervisedUserCommittedInterstitials;

#if defined(OS_CHROMEOS)
extern const base::Feature kSysInternals;
#endif

#if !defined(OS_ANDROID)
extern const base::Feature kTabMetricsLogging;
#endif

extern const base::Feature kTopSitesFromSiteEngagement;

extern const base::Feature kUseGoogleLocalNtp;

#if defined(OS_CHROMEOS)
extern const base::Feature kAdaptiveScreenBrightnessLogging;

extern const base::Feature kUserActivityEventLogging;
#endif

extern const base::Feature kUseSameCacheForMedia;

#if !defined(OS_ANDROID)
extern const base::Feature kVoiceSearchOnLocalNtp;
#endif

#if defined(OS_CHROMEOS)
extern const base::Feature kOptInImeMenu;

extern const base::Feature kQuickUnlockPin;

extern const base::Feature kQuickUnlockPinSignin;

extern const base::Feature kQuickUnlockFingerprint;

extern const base::Feature kEHVInputOnImeMenu;

extern const base::Feature kBulkPrinters;

extern const base::Feature kCrosCompUpdates;

extern const base::Feature kCrOSComponent;

extern const base::Feature kInstantTethering;

extern const base::Feature kEasyUnlockPromotions;

extern const base::Feature kTPMFirmwareUpdate;

extern const base::Feature kCrOSEnableUSMUserService;
#endif  // defined(OS_CHROMEOS)

#if defined(OS_WIN)
extern const base::Feature kWin10AcceleratedDefaultBrowserFlow;
#endif  // defined(OS_WIN)

bool PrefServiceEnabled();

// DON'T ADD RANDOM STUFF HERE. Put it in the main section above in
// alphabetical order, or in one of the ifdefs (also in order in each section).

}  // namespace features

#endif  // CHROME_COMMON_CHROME_FEATURES_H_
