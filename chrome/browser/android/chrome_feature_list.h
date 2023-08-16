// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CHROME_FEATURE_LIST_H_
#define CHROME_BROWSER_ANDROID_CHROME_FEATURE_LIST_H_

#include <base/feature_list.h>
#include <jni.h>

namespace chrome {
namespace android {

// Alphabetical:
extern const base::Feature kAdjustWebApkInstallationSpace;
extern const base::Feature kAllowReaderForAccessibility;
extern const base::Feature kAndroidPayIntegrationV1;
extern const base::Feature kAndroidPayIntegrationV2;
extern const base::Feature kAndroidPaymentApps;
extern const base::Feature kCCTBackgroundTab;
extern const base::Feature kCCTExternalLinkHandling;
extern const base::Feature kCCTParallelRequest;
extern const base::Feature kCCTPostMessageAPI;
extern const base::Feature kCCTRedirectPreconnect;
extern const base::Feature kChromeDuplexFeature;
extern const base::Feature kChromeHomeSwipeLogic;
extern const base::Feature kChromeHomeSwipeLogicVelocity;
extern const base::Feature kChromeMemexFeature;
extern const base::Feature kChromeModernAlternateCardLayout;
extern const base::Feature kChromeModernDesign;
extern const base::Feature kChromeSmartSelection;
extern const base::Feature kCommandLineOnNonRooted;
extern const base::Feature kContentSuggestionsScrollToLoad;
extern const base::Feature kContentSuggestionsSettings;
extern const base::Feature kContentSuggestionsThumbnailDominantColor;
extern const base::Feature kContextualSearchMlTapSuppression;
extern const base::Feature kContextualSearchSecondTap;
extern const base::Feature kContextualSearchTapDisableOverride;
extern const base::Feature kCustomContextMenu;
extern const base::Feature kCustomFeedbackUi;
extern const base::Feature kDontPrefetchLibraries;
extern const base::Feature kDownloadAutoResumptionThrottling;
extern const base::Feature kDownloadProgressInfoBar;
extern const base::Feature kDownloadHomeShowStorageInfo;
extern const base::Feature kFullscreenActivity;
extern const base::Feature kHomePageButtonForceEnabled;
extern const base::Feature kHorizontalTabSwitcherAndroid;
extern const base::Feature kImprovedA2HS;
extern const base::Feature kLanguagesPreference;
extern const base::Feature kLongPressBackForHistory;
extern const base::Feature kModalPermissionDialogView;
extern const base::Feature kSearchEnginePromoExistingDevice;
extern const base::Feature kSearchEnginePromoNewDevice;
extern const base::Feature kNewPhotoPicker;
extern const base::Feature kNoCreditCardAbort;
extern const base::Feature kNTPButton;
extern const base::Feature kNTPLaunchAfterInactivity;
extern const base::Feature kNTPModernLayoutFeature;
extern const base::Feature kNTPShowGoogleGInOmniboxFeature;
extern const base::Feature kOmniboxSpareRenderer;
extern const base::Feature kOmniboxVoiceSearchAlwaysVisible;
extern const base::Feature kPayWithGoogleV1;
extern const base::Feature kProgressBarThrottleFeature;
extern const base::Feature kPwaImprovedSplashScreen;
extern const base::Feature kPwaPersistentNotification;
extern const base::Feature kQueryInOmnibox;
extern const base::Feature kReaderModeInCCT;
extern const base::Feature kSimplifiedNTP;
extern const base::Feature kSoleIntegration;
extern const base::Feature kSpannableInlineAutocomplete;
extern const base::Feature kSpecialLocaleFeature;
extern const base::Feature kSpecialLocaleWrapper;
extern const base::Feature kTabModalJsDialog;
extern const base::Feature kTabReparenting;
extern const base::Feature kTrustedWebActivity;
extern const base::Feature kUseClientCert;
extern const base::Feature kUserMediaScreenCapturing;
extern const base::Feature kVideoPersistence;
extern const base::Feature kVrBrowsingFeedback;
extern const base::Feature kVrBrowsingInCustomTab;
extern const base::Feature kVrBrowsingNativeAndroidUi;
extern const base::Feature kVrBrowsingTabsView;
extern const base::Feature kWebVrAutopresentFromIntent;
extern const base::Feature kWebVrCardboardSupport;

}  // namespace android
}  // namespace chrome

#endif  // CHROME_BROWSER_ANDROID_CHROME_FEATURE_LIST_H_
