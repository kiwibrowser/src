// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/chrome_features.h"

#include "base/command_line.h"
#include "build/build_config.h"
#include "chrome/common/chrome_switches.h"
#include "extensions/buildflags/buildflags.h"
#include "ppapi/buildflags/buildflags.h"

namespace features {

// All features in alphabetical order.

// Enables Ads Metrics.
const base::Feature kAdsFeature{"AdsMetrics", base::FEATURE_ENABLED_BY_DEFAULT};

#if defined(OS_ANDROID)
const base::Feature kAllowAutoplayUnmutedInWebappManifestScope{
    "AllowAutoplayUnmutedInWebappManifestScope",
    base::FEATURE_ENABLED_BY_DEFAULT};
#endif  // defined(OS_ANDROID)

#if defined(OS_MACOSX)
// Enables the menu item for Javascript execution via AppleScript.
const base::Feature kAppleScriptExecuteJavaScriptMenuItem{
    "AppleScriptExecuteJavaScriptMenuItem", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables the "this OS is obsolete" infobar on Mac 10.9.
// TODO(ellyjones): Remove this after the last 10.9 release.
const base::Feature kShow10_9ObsoleteInfobar{"Show109ObsoleteInfobar",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

// Enables the fullscreen toolbar to reveal itself if it's hidden.
const base::Feature kFullscreenToolbarReveal{"FullscreenToolbarReveal",
                                             base::FEATURE_ENABLED_BY_DEFAULT};

// Use the Toolkit-Views Task Manager window.
const base::Feature kViewsTaskManager{"ViewsTaskManager",
                                      base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // defined(OS_MACOSX)

#if !defined(OS_ANDROID)
const base::Feature kAnimatedAppMenuIcon{"AnimatedAppMenuIcon",
                                         base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kAppBanners {
  "AppBanners",
#if defined(OS_CHROMEOS)
      base::FEATURE_ENABLED_BY_DEFAULT,
#else
      base::FEATURE_DISABLED_BY_DEFAULT,
#endif  // defined(OS_CHROMEOS)
};
#endif  // !defined(OS_ANDROID)

#if defined(OS_CHROMEOS)
// Whether to handle low memory kill of ARC apps by Chrome.
const base::Feature kArcMemoryManagement{
    "ArcMemoryManagement", base::FEATURE_ENABLED_BY_DEFAULT};
#endif  // defined(OS_CHROMEOS)

// If enabled, the list of content suggestions on the New Tab page will contain
// assets (e.g. books, pictures, audio) that the user downloaded for later use.
// DO NOT check directly whether this feature is enabled (i.e. do not use
// base::FeatureList::IsEnabled()). It is enabled conditionally. Use
// |AreAssetDownloadsEnabled| instead.
const base::Feature kAssetDownloadSuggestionsFeature{
    "NTPAssetDownloadSuggestions", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables the built-in DNS resolver.
const base::Feature kAsyncDns {
  "AsyncDns",
#if defined(OS_CHROMEOS) || defined(OS_MACOSX)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

#if defined(OS_WIN) || defined(OS_MACOSX)
// Enables automatic tab discarding, when the system is in low memory state.
const base::Feature kAutomaticTabDiscarding{"AutomaticTabDiscarding",
                                            base::FEATURE_ENABLED_BY_DEFAULT};
#endif  // defined(OS_WIN) || defined(OS_MACOSX)

#if defined(OS_WIN) || defined(OS_LINUX)
// Enables the Restart background mode optimization. When all Chrome UI is
// closed and it goes in the background, allows to restart the browser to
// discard memory.
const base::Feature kBackgroundModeAllowRestart{
    "BackgroundModeAllowRestart", base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // defined(OS_WIN) || defined(OS_LINUX)

// Enables or disables whether permission prompts are automatically blocked
// after the user has explicitly dismissed them too many times.
const base::Feature kBlockPromptsIfDismissedOften{
    "BlockPromptsIfDismissedOften", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables or disables whether permission prompts are automatically blocked
// after the user has ignored them too many times.
const base::Feature kBlockPromptsIfIgnoredOften{
    "BlockPromptsIfIgnoredOften", base::FEATURE_DISABLED_BY_DEFAULT};

#if defined(OS_MACOSX)
// Enables the new bookmark app system (e.g. Add To Applications on Mac).
const base::Feature kBookmarkApps{"BookmarkAppsMac",
                                  base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// Fixes for browser hang bugs are deployed in a field trial in order to measure
// their impact. See crbug.com/478209.
const base::Feature kBrowserHangFixesExperiment{
    "BrowserHangFixesExperiment", base::FEATURE_DISABLED_BY_DEFAULT};

#if defined(OS_MACOSX)
// Enables or disables the browser's touch bar.
const base::Feature kBrowserTouchBar{"BrowserTouchBar",
                                     base::FEATURE_ENABLED_BY_DEFAULT};
#endif

// Enables or disables redirecting users who get an interstitial when
// accessing https://support.google.com/chrome/answer/6098869 to local
// connection help content.
const base::Feature kBundledConnectionHelpFeature{
    "BundledConnectionHelp", base::FEATURE_DISABLED_BY_DEFAULT};

#if defined(OS_MACOSX)
// Enables or disables touch bar support for dialogs.
const base::Feature kDialogTouchBar{"DialogTouchBar",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

// Enables or disables keyboard focus for the tab strip.
const base::Feature kTabStripKeyboardFocus{"TabStripKeyboardFocus",
                                           base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // defined(OS_MACOSX)

#if !defined(OS_ANDROID)
// Enables logging UKMs for background tab activity by TabActivityWatcher.
const base::Feature kTabMetricsLogging{"TabMetricsLogging",
                                       base::FEATURE_ENABLED_BY_DEFAULT};
#endif

#if (defined(OS_LINUX) && !defined(OS_CHROMEOS)) || defined(OS_MACOSX)
// Enables the dual certificate verification trial feature.
// https://crbug.com/649026
const base::Feature kCertDualVerificationTrialFeature{
    "CertDualVerificationTrial", base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// Enables change picture video mode.
const base::Feature kChangePictureVideoMode{"ChangePictureVideoMode",
                                            base::FEATURE_ENABLED_BY_DEFAULT};

#if defined(OS_ANDROID)
// Enables clearing of browsing data which is older than given time period.
const base::Feature kClearOldBrowsingData{"ClearOldBrowsingData",
                                          base::FEATURE_DISABLED_BY_DEFAULT};
#endif

const base::Feature kClickToOpenPDFPlaceholder{
    "ClickToOpenPDFPlaceholder", base::FEATURE_ENABLED_BY_DEFAULT};

#if defined(OS_MACOSX)
const base::Feature kContentFullscreen{"ContentFullscreen",
                                       base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// Enables a site-wide permission in the UI which controls access to the
// asynchronous Clipboard web API.
const base::Feature kClipboardContentSetting{"ClipboardContentSetting",
                                             base::FEATURE_ENABLED_BY_DEFAULT};

#if defined(OS_CHROMEOS)
// Enable project Crostini, Linux VMs on Chrome OS.
const base::Feature kCrostini{"Crostini", base::FEATURE_DISABLED_BY_DEFAULT};

// Whether the UsageTimeLimit policy should be applied to the user.
const base::Feature kUsageTimeLimitPolicy{"UsageTimeLimitPolicy",
                                          base::FEATURE_DISABLED_BY_DEFAULT};
#endif

#if defined(OS_WIN)
// Enables or disables desktop ios promotion, which shows a promotion to the
// user promoting Chrome for iOS.
const base::Feature kDesktopIOSPromotion{"DesktopIOSPromotion",
                                         base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// Enables or disables windowing related features for desktop PWAs.
const base::Feature kDesktopPWAWindowing {
  "DesktopPWAWindowing",
#if defined(OS_CHROMEOS)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

// Enables or disables Desktop PWAs capturing links.
const base::Feature kDesktopPWAsLinkCapturing{
    "DesktopPWAsLinkCapturing", base::FEATURE_DISABLED_BY_DEFAULT};

// Disables downloads of unsafe file types over HTTP.
const base::Feature kDisallowUnsafeHttpDownloads{
    "DisallowUnsafeHttpDownloads", base::FEATURE_DISABLED_BY_DEFAULT};
const char kDisallowUnsafeHttpDownloadsParamName[] = "MimeTypeList";

#if !defined(OS_ANDROID)
const base::Feature kDoodlesOnLocalNtp{"DoodlesOnLocalNtp",
                                       base::FEATURE_DISABLED_BY_DEFAULT};
#endif

#if defined(OS_ANDROID)
// Enables downloads as a foreground service for all versions of Android.
const base::Feature kDownloadsForeground{"DownloadsForeground",
                                         base::FEATURE_ENABLED_BY_DEFAULT};
#endif

#if defined(OS_ANDROID)
// Enable changing default downloads storage location on Android.
const base::Feature kDownloadsLocationChange{"DownloadsLocationChange",
                                             base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// Enables Expect CT reporting, which sends reports for opted-in sites
// that don't serve sufficient Certificate Transparency information.
const base::Feature kExpectCTReporting{"ExpectCTReporting",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

// An experimental way of showing app banners, which has modal banners and gives
// developers more control over when to show them.
const base::Feature kExperimentalAppBanners {
  "ExperimentalAppBanners",
#if defined(OS_CHROMEOS) || defined(OS_ANDROID)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

#if defined(OS_CHROMEOS)
extern const base::Feature kExperimentalCrostiniUI{
    "ExperimentalCrostiniUI", base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// If enabled, this feature's |kExternalInstallDefaultButtonKey| field trial
// parameter value controls which |ExternalInstallBubbleAlert| button is the
// default.
const base::Feature kExternalExtensionDefaultButtonControl{
    "ExternalExtensionDefaultButtonControl", base::FEATURE_DISABLED_BY_DEFAULT};

#if BUILDFLAG(ENABLE_VR)
// Controls features related to VR browsing that are under development.
const base::Feature kVrBrowsingExperimentalFeatures{
    "VrBrowsingExperimentalFeatures", base::FEATURE_DISABLED_BY_DEFAULT};

// Controls experimental rendering features for VR browsing.
const base::Feature kVrBrowsingExperimentalRendering{
    "VrBrowsingExperimentalRendering", base::FEATURE_DISABLED_BY_DEFAULT};

#if BUILDFLAG(ENABLE_OCULUS_VR)
// Controls Oculus support.
const base::Feature kOculusVR{"OculusVR", base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // ENABLE_OCULUS_VR

#if BUILDFLAG(ENABLE_OPENVR)
// Controls OpenVR support.
const base::Feature kOpenVR{"OpenVR", base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // ENABLE_OPENVR

#endif  // BUILDFLAG(ENABLE_VR)

// Enables a floating action button-like full screen exit UI to allow exiting
// fullscreen using mouse or touch.
const base::Feature kFullscreenExitUI{"FullscreenExitUI",
                                      base::FEATURE_ENABLED_BY_DEFAULT};

#if defined(OS_WIN)
// Enables using GDI to print text as simply text.
const base::Feature kGdiTextPrinting {"GdiTextPrinting",
                                      base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// Controls whether the GeoLanguage system is enabled. GeoLanguage uses IP-based
// coarse geolocation to provide an estimate (for use by other Chrome features
// such as Translate) of the local/regional language(s) corresponding to the
// device's location. If this feature is disabled, the GeoLanguage provider is
// not initialized at startup, and clients calling it will receive an empty list
// of languages.
const base::Feature kGeoLanguage{"GeoLanguage",
                                 base::FEATURE_DISABLED_BY_DEFAULT};

#if defined(OS_ANDROID)
const base::Feature kGrantNotificationsToDSE{"GrantNotificationsToDSE",
                                             base::FEATURE_ENABLED_BY_DEFAULT};
#endif  // defined(OS_ANDROID)

#if defined (OS_CHROMEOS)
// Enables or disables the Happiness Tracking System for the device.
const base::Feature kHappinessTrackingSystem {
    "HappinessTrackingSystem", base::FEATURE_DISABLED_BY_DEFAULT};
#endif

#if !defined(OS_ANDROID)
// Replaces the WebUI Cast dialog with a Views toolkit one.
const base::Feature kViewsCastDialog{"ViewsCastDialog",
                                     base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // !defined(OS_ANDROID)

const base::Feature kImportantSitesInCbd{"ImportantSitesInCBD",
                                         base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether the "improved recovery component" is used. The improved
// recovery component is a redesigned Chrome component intended to restore
// a broken Chrome updater in more scenarios than before.
const base::Feature kImprovedRecoveryComponent{
    "ImprovedRecoveryComponent", base::FEATURE_DISABLED_BY_DEFAULT};

#if defined(OS_WIN) && defined(GOOGLE_CHROME_BUILD)
// A feature that controls whether Chrome warns about incompatible applications.
const base::Feature kIncompatibleApplicationsWarning{
    "IncompatibleApplicationsWarning", base::FEATURE_DISABLED_BY_DEFAULT};
#endif

#if !defined(OS_ANDROID)
// Enables Casting a Presentation API-enabled website to a secondary display.
const base::Feature kLocalScreenCasting{"LocalScreenCasting",
                                        base::FEATURE_ENABLED_BY_DEFAULT};
#endif

// Enables or disables the Location Settings Dialog (LSD). The LSD is an Android
// system-level geolocation permission prompt.
const base::Feature kLsdPermissionPrompt{"LsdPermissionPrompt",
                                         base::FEATURE_ENABLED_BY_DEFAULT};

#if defined(OS_MACOSX)
// Enables RTL layout in macOS top chrome.
const base::Feature kMacRTL{"MacRTL", base::FEATURE_ENABLED_BY_DEFAULT};

// Uses NSFullSizeContentViewWindowMask where available instead of adding our
// own views to the window frame. This is a temporary kill switch, it can be
// removed once we feel okay about leaving it on.
const base::Feature kMacFullSizeContentView{"MacFullSizeContentView",
                                            base::FEATURE_ENABLED_BY_DEFAULT};

// Enables "Share" submenu in File menu.
const base::Feature kMacSystemShareMenu{"MacSystemShareMenu",
                                        base::FEATURE_ENABLED_BY_DEFAULT};
#endif

#if defined(OS_MACOSX)
// Enables the Material Design download shelf on Mac.
const base::Feature kMacMaterialDesignDownloadShelf{
    "MacMDDownloadShelf", base::FEATURE_ENABLED_BY_DEFAULT};
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
// Enabled or disabled the Material Design version of chrome://extensions.
const base::Feature kMaterialDesignExtensions{"MaterialDesignExtensions",
                                              base::FEATURE_ENABLED_BY_DEFAULT};

// Sets whether dismissing the new-tab-page override bubble counts as
// acknowledgement.
const base::Feature kAcknowledgeNtpOverrideOnDeactivate{
    "AcknowledgeNtpOverrideOnDeactivate", base::FEATURE_DISABLED_BY_DEFAULT};
#endif

#if defined(OS_WIN) || (defined(OS_LINUX) && !defined(OS_CHROMEOS))
const base::Feature kWarnBeforeQuitting{"WarnBeforeQuitting",
                                        base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// The material redesign of the Incognito NTP.
const base::Feature kMaterialDesignIncognitoNTP{
  "MaterialDesignIncognitoNTP",
#if defined(OS_ANDROID)
      base::FEATURE_DISABLED_BY_DEFAULT
#else
      base::FEATURE_ENABLED_BY_DEFAULT
#endif
};

// Enables or disables modal permission prompts.
const base::Feature kModalPermissionPrompts{"ModalPermissionPrompts",
                                            base::FEATURE_ENABLED_BY_DEFAULT};

#if defined(OS_WIN)
// Enables or disables the ModuleDatabase backend for the conflicts UI.
const base::Feature kModuleDatabase{"ModuleDatabase",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
#endif

#if defined(OS_CHROMEOS)
// Enables or disables multidevice features and corresponding UI on Chrome OS.
const base::Feature kMultidevice{"Multidevice",
                                 base::FEATURE_DISABLED_BY_DEFAULT};
// Enables or disables the MultiDevice API on Chrome OS.
const base::Feature kMultiDeviceApi{"MultiDeviceApi",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// Enables the use of native notification centers instead of using the Message
// Center for displaying the toasts. Note that OS_LINUX includes Chrome OS.
#if BUILDFLAG(ENABLE_NATIVE_NOTIFICATIONS)
#if defined(OS_MACOSX) || defined(OS_ANDROID) || defined(OS_LINUX)
const base::Feature kNativeNotifications{"NativeNotifications",
                                         base::FEATURE_ENABLED_BY_DEFAULT};
#else
const base::Feature kNativeNotifications{"NativeNotifications",
                                         base::FEATURE_DISABLED_BY_DEFAULT};
#endif
#endif  // BUILDFLAG(ENABLE_NATIVE_NOTIFICATIONS)

const base::Feature kNetworkPrediction{"NetworkPrediction",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

#if defined(OS_POSIX)
// Enables NTLMv2, which implicitly disables NTLMv1.
const base::Feature kNtlmV2Enabled{"NtlmV2Enabled",
                                   base::FEATURE_ENABLED_BY_DEFAULT};
#endif

// If enabled, the user will see a configuration UI, and be able to select
// background images to set on the New Tab Page.
const base::Feature kNtpBackgrounds{"NewTabPageBackgrounds",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

// If enabled, the user will see custom link icons instead of Most Visited tiles
// on the New Tab Page.
const base::Feature kNtpIcons{"NewTabPageIcons",
                              base::FEATURE_DISABLED_BY_DEFAULT};

// If enabled, the user will see the New Tab Page updated with Material Design
// elements.
const base::Feature kNtpUIMd{"NewTabPageUIMd",
                             base::FEATURE_DISABLED_BY_DEFAULT};

// If enabled, the list of content suggestions on the New Tab page will contain
// pages that the user downloaded for later use.
// DO NOT check directly whether this feature is enabled (i.e. do not use
// base::FeatureList::IsEnabled()). It is enabled conditionally. Use
// |AreOfflinePageDownloadsEnabled| instead.
const base::Feature kOfflinePageDownloadSuggestionsFeature{
    "NTPOfflinePageDownloadSuggestions", base::FEATURE_ENABLED_BY_DEFAULT};

#if defined(OS_ANDROID)
// Enables or disabled the OOM intervention.
const base::Feature kOomIntervention{"OomIntervention",
                                     base::FEATURE_DISABLED_BY_DEFAULT};
#endif

#if !defined(OS_ANDROID)
// Enables or disabled the OneGoogleBar on the local NTP.
const base::Feature kOneGoogleBarOnLocalNtp{"OneGoogleBarOnLocalNtp",
                                            base::FEATURE_ENABLED_BY_DEFAULT};
#endif

// Adds the base language code to the Language-Accept headers if at least one
// corresponding language+region code is present in the user preferences.
// For example: "en-US, fr-FR" --> "en-US, en, fr-FR, fr".
const base::Feature kUseNewAcceptLanguageHeader{
    "UseNewAcceptLanguageHeader", base::FEATURE_ENABLED_BY_DEFAULT};

// Delegate permissions to cross-origin iframes when the feature has been
// allowed by feature policy.
const base::Feature kPermissionDelegation{"PermissionDelegation",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

// Disables PostScript generation when printing to PostScript capable printers
// and instead sends Emf files.
#if defined(OS_WIN)
const base::Feature kDisablePostScriptPrinting{
    "DisablePostScriptPrinting", base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// Enables a page for manaing policies at chrome://policy-tool.
#if !defined(OS_ANDROID)
const base::Feature kPolicyTool{"PolicyTool",
                                base::FEATURE_DISABLED_BY_DEFAULT};
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
// Prefer HTML content by hiding Flash from the list of plugins.
// https://crbug.com/626728
const base::Feature kPreferHtmlOverPlugins{"PreferHtmlOverPlugins",
                                           base::FEATURE_ENABLED_BY_DEFAULT};
#endif

#if defined(OS_CHROMEOS)
// The lock screen will be preloaded so it is instantly available when the user
// locks the Chromebook device.
const base::Feature kPreloadLockScreen{"PreloadLockScreen",
                                       base::FEATURE_ENABLED_BY_DEFAULT};
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
// Enables the new Print Preview UI.
const base::Feature kNewPrintPreview{"NewPrintPreview",
                                     base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kNupPrinting{"NupPrinting",
                                 base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// Enables or disables push subscriptions keeping Chrome running in the
// background when closed.
const base::Feature kPushMessagingBackgroundMode{
    "PushMessagingBackgroundMode", base::FEATURE_DISABLED_BY_DEFAULT};

#if !defined(OS_ANDROID)
const base::Feature kRemoveUsageOfDeprecatedGaiaSigninEndpoint{
    "RemoveUsageOfDeprecatedGaiaSigninEndpoint",
    base::FEATURE_ENABLED_BY_DEFAULT};
#endif

const base::Feature kSafeSearchUrlReporting{"SafeSearchUrlReporting",
                                            base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether the user is prompted when sites request attestation.
const base::Feature kSecurityKeyAttestationPrompt{
    "SecurityKeyAttestationPrompt", base::FEATURE_ENABLED_BY_DEFAULT};

#if defined(OS_MACOSX)
// Whether to show all dialogs with toolkit-views on Mac, rather than Cocoa. A
// subset of "pilot" dialogs can be enabled with kSecondaryUiMd.
const base::Feature kShowAllDialogsWithViewsToolkit{
    "ShowAllDialogsWithViewsToolkit", base::FEATURE_ENABLED_BY_DEFAULT};
#endif  // defined(OS_MACOSX)

#if defined(OS_ANDROID)
const base::Feature kShowTrustedPublisherURL{"ShowTrustedPublisherURL",
                                             base::FEATURE_ENABLED_BY_DEFAULT};
#endif

// Alternative to switches::kSitePerProcess, for turning on full site isolation.
// Launch bug: https://crbug.com/739418.  This is a //chrome-layer feature to
// avoid turning on site-per-process by default for *all* //content embedders
// (e.g. this approach lets ChromeCast avoid site-per-process mode).
const base::Feature kSitePerProcess{"site-per-process",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

// kSitePerProcessOnlyForHighMemoryClients is checked before kSitePerProcess,
// and (if enabled) can restrict if kSitePerProcess feature is checked at all -
// no check will be made on devices with low memory (these devices will have no
// Site Isolation via kSitePerProcess trials and won't activate either the
// control or the experiment group).  The threshold for what is considered a
// "low memory" device is set (in MB) via a field trial param with the name
// defined below ("site-per-process-low-memory-cutoff-mb") and compared against
// base::SysInfo::AmountOfPhysicalMemoryMB().
const base::Feature kSitePerProcessOnlyForHighMemoryClients{
    "site-per-process-only-for-high-memory-clients",
    base::FEATURE_DISABLED_BY_DEFAULT};
const char kSitePerProcessOnlyForHighMemoryClientsParamName[] =
    "site-per-process-low-memory-cutoff-mb";

// The site settings all sites list page in the Chrome settings UI.
const base::Feature kSiteSettings{"SiteSettings",
                                  base::FEATURE_DISABLED_BY_DEFAULT};

#if defined(OS_ANDROID)
// Enables separate notification channels in Android O for notifications from
// different origins, instead of sending them all to a single 'Sites' channel.
const base::Feature kSiteNotificationChannels{"SiteNotificationChannels",
                                              base::FEATURE_ENABLED_BY_DEFAULT};
#endif  // defined(OS_ANDROID)

#if defined(OS_CHROMEOS)
// Enables or disables the ability to add a Samba Share to the Files app
const base::Feature kNativeSmb{"NativeSmb", base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // defined(OS_CHROMEOS)

// Enables or disables the ability to use the sound content setting to mute a
// website.
const base::Feature kSoundContentSetting{"SoundContentSetting",
                                         base::FEATURE_ENABLED_BY_DEFAULT};

// Enables or disabled committed interstitials for Supervised User
// interstitials.
const base::Feature kSupervisedUserCommittedInterstitials{
    "SupervisedUserCommittedInterstitials", base::FEATURE_DISABLED_BY_DEFAULT};

#if defined(OS_CHROMEOS)
// Enables or disables chrome://sys-internals.
const base::Feature kSysInternals{"SysInternals",
                                  base::FEATURE_DISABLED_BY_DEFAULT};
#endif

// Enable TopSites to source and sort its site data using site engagement.
const base::Feature kTopSitesFromSiteEngagement{
    "TopSitesFromSiteEngagement", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables using the local NTP if Google is the default search engine.
const base::Feature kUseGoogleLocalNtp{"UseGoogleLocalNtp",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

#if defined(OS_CHROMEOS)
// Enables or disables logging for adaptive screen brightness on Chrome OS.
const base::Feature kAdaptiveScreenBrightnessLogging{
    "AdaptiveScreenBrightnessLogging", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables or disables user activity event logging for power management on
// Chrome OS.
const base::Feature kUserActivityEventLogging{"UserActivityEventLogging",
                                              base::FEATURE_ENABLED_BY_DEFAULT};
#endif

// Enables using the main HTTP cache for media files as well.
const base::Feature kUseSameCacheForMedia{"UseSameCacheForMedia",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

#if !defined(OS_ANDROID)
// Enables or disables Voice Search on the local NTP.
const base::Feature kVoiceSearchOnLocalNtp{"VoiceSearchOnLocalNtp",
                                           base::FEATURE_ENABLED_BY_DEFAULT};
#endif

#if defined(OS_CHROMEOS)
// Enables or disables the opt-in IME menu in the language settings page.
const base::Feature kOptInImeMenu{"OptInImeMenu",
                                  base::FEATURE_ENABLED_BY_DEFAULT};

// Enables or disables pin quick unlock.
const base::Feature kQuickUnlockPin{"QuickUnlockPin",
                                    base::FEATURE_ENABLED_BY_DEFAULT};

// Enables pin on the login screen.
const base::Feature kQuickUnlockPinSignin{"QuickUnlockPinSignin",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

// Enables or disables fingerprint quick unlock.
const base::Feature kQuickUnlockFingerprint{"QuickUnlockFingerprint",
                                            base::FEATURE_DISABLED_BY_DEFAULT};

// Enables or disables emoji, handwriting and voice input on opt-in IME menu.
const base::Feature kEHVInputOnImeMenu{"EmojiHandwritingVoiceInput",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

// Enables or disables the bulk printer policies on Chrome OS.
const base::Feature kBulkPrinters{"BulkPrinters",
                                  base::FEATURE_ENABLED_BY_DEFAULT};

// Enables or disables flash component updates on Chrome OS.
const base::Feature kCrosCompUpdates{"CrosCompUpdates",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

// Enables or disables Chrome OS Component updates on Chrome OS.
const base::Feature kCrOSComponent{"CrOSComponent",
                                   base::FEATURE_ENABLED_BY_DEFAULT};

// Enables or disables Instant Tethering on Chrome OS.
const base::Feature kInstantTethering{"InstantTethering",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

// Enables or disables EasyUnlock promotions on Chrome OS.
const base::Feature kEasyUnlockPromotions{"EasyUnlockPromotions",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

// Enables or disables TPM firmware update capability on Chrome OS.
const base::Feature kTPMFirmwareUpdate{"TPMFirmwareUpdate",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

// Enables or disables "usm" service in the list of user services returned by
// userInfo Gaia message.
const base::Feature kCrOSEnableUSMUserService{"CrOSEnableUSMUserService",
                                              base::FEATURE_ENABLED_BY_DEFAULT};

#endif  // defined(OS_CHROMEOS)

#if defined(OS_WIN)
// Enables the accelerated default browser flow for Windows 10.
const base::Feature kWin10AcceleratedDefaultBrowserFlow{
    "Win10AcceleratedDefaultBrowserFlow", base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // defined(OS_WIN)

}  // namespace features
