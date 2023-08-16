// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/chrome_feature_list.h"

#include <stddef.h>

#include <string>

#include "base/android/jni_string.h"
#include "base/feature_list.h"
#include "base/macros.h"
#include "base/metrics/field_trial_params.h"
#include "chrome/common/chrome_features.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_features.h"
#include "components/feed/feed_feature_list.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_features.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_tiles/constants.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/payments/core/features.h"
#include "components/safe_browsing/features.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "content/public/common/content_features.h"
#include "jni/ChromeFeatureList_jni.h"
#include "media/base/media_switches.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace chrome {
namespace android {

namespace {

// Array of features exposed through the Java ChromeFeatureList API. Entries in
// this array may either refer to features defined in the header of this file or
// in other locations in the code base (e.g. chrome/, components/, etc).
const base::Feature* kFeaturesExposedToJava[] = {
    &autofill::kAutofillScanCardholderName,
    &contextual_suggestions::kContextualSuggestionsBottomSheet,
    &contextual_suggestions::kContextualSuggestionsEnterprisePolicyBypass,
    &contextual_suggestions::kContextualSuggestionsSlimPeekUI,
    &features::kClearOldBrowsingData,
    &features::kClipboardContentSetting,
    &features::kDownloadsForeground,
    &features::kDownloadsLocationChange,
    &features::kExperimentalAppBanners,
    &features::kImportantSitesInCbd,
    &features::kMaterialDesignIncognitoNTP,
    &features::kPermissionDelegation,
    &features::kServiceWorkerPaymentApps,
    &features::kShowTrustedPublisherURL,
    &features::kSiteNotificationChannels,
    &features::kSoundContentSetting,
    &features::kWebAuth,
    &features::kWebPayments,
    &feed::kInterestFeedContentSuggestions,
    &kAdjustWebApkInstallationSpace,
    &kAllowReaderForAccessibility,
    &kAndroidPayIntegrationV1,
    &kAndroidPayIntegrationV2,
    &kAndroidPaymentApps,
    &kCCTBackgroundTab,
    &kCCTExternalLinkHandling,
    &kCCTParallelRequest,
    &kCCTPostMessageAPI,
    &kCCTRedirectPreconnect,
    &kChromeDuplexFeature,
    &kChromeHomeSwipeLogic,
    &kChromeHomeSwipeLogicVelocity,
    &kChromeSmartSelection,
    &kChromeMemexFeature,
    &kChromeModernAlternateCardLayout,
    &kChromeModernDesign,
    &kCommandLineOnNonRooted,
    &kContentSuggestionsScrollToLoad,
    &kContentSuggestionsSettings,
    &kContentSuggestionsThumbnailDominantColor,
    &kContextualSearchMlTapSuppression,
    &kContextualSearchSecondTap,
    &kContextualSearchTapDisableOverride,
    &kCustomContextMenu,
    &kCustomFeedbackUi,
    &kDontPrefetchLibraries,
    &kDownloadProgressInfoBar,
    &kDownloadHomeShowStorageInfo,
    &data_reduction_proxy::features::kDataReductionMainMenu,
    &kFullscreenActivity,
    &kHomePageButtonForceEnabled,
    &kHorizontalTabSwitcherAndroid,
    &kImprovedA2HS,
    &kLanguagesPreference,
    &kLongPressBackForHistory,
    &kModalPermissionDialogView,
    &kNewPhotoPicker,
    &kNoCreditCardAbort,
    &kNTPButton,
    &kNTPLaunchAfterInactivity,
    &kNTPModernLayoutFeature,
    &kSimplifiedNTP,
    &kNTPShowGoogleGInOmniboxFeature,
    &kOmniboxSpareRenderer,
    &kOmniboxVoiceSearchAlwaysVisible,
    &kPayWithGoogleV1,
    &kProgressBarThrottleFeature,
    &kPwaImprovedSplashScreen,
    &kPwaPersistentNotification,
    &kQueryInOmnibox,
    &kReaderModeInCCT,
    &kSearchEnginePromoExistingDevice,
    &kSearchEnginePromoNewDevice,
    &kSoleIntegration,
    &kSpannableInlineAutocomplete,
    &kSpecialLocaleFeature,
    &kSpecialLocaleWrapper,
    &kTabReparenting,
    &kTrustedWebActivity,
    &kVideoPersistence,
    &kVrBrowsingFeedback,
    &kVrBrowsingInCustomTab,
    &kVrBrowsingNativeAndroidUi,
    &payments::features::kWebPaymentsMethodSectionOrderV2,
    &payments::features::kWebPaymentsModifiers,
    &payments::features::kWebPaymentsSingleAppUiSkip,
    &kWebVrAutopresentFromIntent,
    &kWebVrCardboardSupport,
    &media::kCafMediaRouterImpl,
    &ntp_snippets::kArticleSuggestionsExpandableHeader,
    &ntp_snippets::kIncreasedVisibility,
    &ntp_snippets::kForeignSessionsSuggestionsFeature,
    &ntp_snippets::kNotificationsFeature,
    &ntp_snippets::kPublisherFaviconsFromNewServerFeature,
    &ntp_tiles::kSiteExplorationUiFeature,
    &offline_pages::kBackgroundLoaderForDownloadsFeature,
    &offline_pages::kOfflinePagesCTFeature,    // See crbug.com/620421.
    &offline_pages::kOfflinePagesCTV2Feature,  // See crbug.com/734753.
    &offline_pages::kOfflinePagesDescriptiveFailStatusFeature,
    &offline_pages::kOfflinePagesDescriptivePendingStatusFeature,
    &offline_pages::kOfflinePagesSharingFeature,
    &omnibox::kUIExperimentHideSteadyStateUrlSchemeAndSubdomains,
    &password_manager::features::kPasswordExport,
    &password_manager::features::kPasswordSearchMobile,
    &password_manager::features::kPasswordsKeyboardAccessory,
    &signin::kUnifiedConsent,
    &subresource_filter::kSafeBrowsingSubresourceFilter,
};

const base::Feature* FindFeatureExposedToJava(const std::string& feature_name) {
  for (size_t i = 0; i < arraysize(kFeaturesExposedToJava); ++i) {
    if (kFeaturesExposedToJava[i]->name == feature_name)
      return kFeaturesExposedToJava[i];
  }
  NOTREACHED() << "Queried feature cannot be found in ChromeFeatureList: "
               << feature_name;
  return nullptr;
}

}  // namespace

// Alphabetical:
const base::Feature kAdjustWebApkInstallationSpace = {
    "AdjustWebApkInstallationSpace", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kAllowReaderForAccessibility = {
    "AllowReaderForAccessibility", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAndroidPayIntegrationV1{"AndroidPayIntegrationV1",
                                             base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAndroidPayIntegrationV2{"AndroidPayIntegrationV2",
                                             base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAndroidPaymentApps{"AndroidPaymentApps",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kCCTBackgroundTab{"CCTBackgroundTab",
                                      base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kCCTExternalLinkHandling{"CCTExternalLinkHandling",
                                             base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kCCTParallelRequest{"CCTParallelRequest",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kCCTPostMessageAPI{"CCTPostMessageAPI",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kCCTRedirectPreconnect{"CCTRedirectPreconnect",
                                           base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kChromeDuplexFeature{"ChromeDuplex",
                                         base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kChromeHomeSwipeLogic{"ChromeHomeSwipeLogic",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kChromeHomeSwipeLogicVelocity{
    "ChromeHomeSwipeLogicVelocity", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kChromeMemexFeature{"ChromeMemex",
                                        base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kChromeModernAlternateCardLayout{
    "ChromeModernAlternateCardLayout", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kChromeModernDesign{"ChromeModernDesign",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kChromeSmartSelection{"ChromeSmartSelection",
                                          base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kCommandLineOnNonRooted{"CommandLineOnNonRooted",
                                            base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kContentSuggestionsScrollToLoad{
    "ContentSuggestionsScrollToLoad", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kContentSuggestionsSettings{
    "ContentSuggestionsSettings", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kContentSuggestionsThumbnailDominantColor{
    "ContentSuggestionsThumbnailDominantColor",
    base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kContextualSearchMlTapSuppression{
    "ContextualSearchMlTapSuppression", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kContextualSearchSecondTap{
    "ContextualSearchSecondTap", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kContextualSearchTapDisableOverride{
    "ContextualSearchTapDisableOverride", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kCustomContextMenu{"CustomContextMenu",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kCustomFeedbackUi{"CustomFeedbackUi",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kDontPrefetchLibraries{"DontPrefetchLibraries",
                                           base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kDownloadAutoResumptionThrottling{
    "DownloadAutoResumptionThrottling", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kDownloadProgressInfoBar{"DownloadProgressInfoBar",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kDownloadHomeShowStorageInfo{
    "DownloadHomeShowStorageInfo", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kFullscreenActivity{"FullscreenActivity",
                                        base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kHomePageButtonForceEnabled{
    "HomePageButtonForceEnabled", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kHorizontalTabSwitcherAndroid{
    "HorizontalTabSwitcherAndroid", base::FEATURE_DISABLED_BY_DEFAULT};

// Makes "Add to Home screen" in the app menu generate an APK for the shortcut
// URL which opens Chrome in fullscreen.
// This feature is kept around so that we have a kill-switch in case of server
// issues.
const base::Feature kImprovedA2HS{"ImprovedA2HS",
                                  base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kLanguagesPreference{"LanguagesPreference",
                                         base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kLongPressBackForHistory{"LongPressBackForHistory",
                                             base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kModalPermissionDialogView{
    "ModalPermissionDialogView", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kSearchEnginePromoExistingDevice{
    "SearchEnginePromo.ExistingDevice", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kSearchEnginePromoNewDevice{
    "SearchEnginePromo.NewDevice", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kNewPhotoPicker{"NewPhotoPicker",
                                    base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kNoCreditCardAbort{"NoCreditCardAbort",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kNTPButton{"NTPButton", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kNTPModernLayoutFeature{"NTPModernLayout",
                                            base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kNTPLaunchAfterInactivity{
    "NTPLaunchAfterInactivity", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kNTPShowGoogleGInOmniboxFeature{
    "NTPShowGoogleGInOmnibox", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kOmniboxSpareRenderer{"OmniboxSpareRenderer",
                                          base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kOmniboxVoiceSearchAlwaysVisible{
    "OmniboxVoiceSearchAlwaysVisible", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kPayWithGoogleV1{"PayWithGoogleV1",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kProgressBarThrottleFeature{
    "ProgressBarThrottle", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kPwaImprovedSplashScreen{"PwaImprovedSplashScreen",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kPwaPersistentNotification{
    "PwaPersistentNotification", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kQueryInOmnibox{"QueryInOmnibox",
                                    base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kReaderModeInCCT{"ReaderModeInCCT",
                                     base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kSimplifiedNTP{"SimplifiedNTP",
                                   base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kSoleIntegration{"SoleIntegration",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kSpannableInlineAutocomplete{
    "SpannableInlineAutocomplete", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kSpecialLocaleFeature{"SpecialLocale",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kSpecialLocaleWrapper{"SpecialLocaleWrapper",
                                          base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kTabModalJsDialog{"TabModalJsDialog",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kTabReparenting{"TabReparenting",
                                    base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kTrustedWebActivity{"TrustedWebActivity",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kUseClientCert{"UseClientCertificates",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kUserMediaScreenCapturing{
    "UserMediaScreenCapturing", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kVideoPersistence{"VideoPersistence",
                                      base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kVrBrowsingFeedback{"VrBrowsingFeedback",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kVrBrowsingInCustomTab{"VrBrowsingInCustomTab",
                                           base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kVrBrowsingNativeAndroidUi{
    "VrBrowsingNativeAndroidUi", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kVrBrowsingTabsView{"VrBrowsingTabsView",
                                        base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kWebVrAutopresentFromIntent{
    "WebVrAutopresentFromIntent", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kWebVrCardboardSupport{"WebVrCardboardSupport",
                                           base::FEATURE_ENABLED_BY_DEFAULT};

static jboolean JNI_ChromeFeatureList_IsInitialized(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  return !!base::FeatureList::GetInstance();
}

static jboolean JNI_ChromeFeatureList_IsEnabled(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& jfeature_name) {
  const base::Feature* feature =
      FindFeatureExposedToJava(ConvertJavaStringToUTF8(env, jfeature_name));
  return base::FeatureList::IsEnabled(*feature);
}

static ScopedJavaLocalRef<jstring>
JNI_ChromeFeatureList_GetFieldTrialParamByFeature(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& jfeature_name,
    const JavaParamRef<jstring>& jparam_name) {
  const base::Feature* feature =
      FindFeatureExposedToJava(ConvertJavaStringToUTF8(env, jfeature_name));
  const std::string& param_name = ConvertJavaStringToUTF8(env, jparam_name);
  const std::string& param_value =
      base::GetFieldTrialParamValueByFeature(*feature, param_name);
  return ConvertUTF8ToJavaString(env, param_value);
}

static jint JNI_ChromeFeatureList_GetFieldTrialParamByFeatureAsInt(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& jfeature_name,
    const JavaParamRef<jstring>& jparam_name,
    const jint jdefault_value) {
  const base::Feature* feature =
      FindFeatureExposedToJava(ConvertJavaStringToUTF8(env, jfeature_name));
  const std::string& param_name = ConvertJavaStringToUTF8(env, jparam_name);
  return base::GetFieldTrialParamByFeatureAsInt(*feature, param_name,
                                                jdefault_value);
}

static jdouble JNI_ChromeFeatureList_GetFieldTrialParamByFeatureAsDouble(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& jfeature_name,
    const JavaParamRef<jstring>& jparam_name,
    const jdouble jdefault_value) {
  const base::Feature* feature =
      FindFeatureExposedToJava(ConvertJavaStringToUTF8(env, jfeature_name));
  const std::string& param_name = ConvertJavaStringToUTF8(env, jparam_name);
  return base::GetFieldTrialParamByFeatureAsDouble(*feature, param_name,
                                                   jdefault_value);
}

static jboolean JNI_ChromeFeatureList_GetFieldTrialParamByFeatureAsBoolean(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& jfeature_name,
    const JavaParamRef<jstring>& jparam_name,
    const jboolean jdefault_value) {
  const base::Feature* feature =
      FindFeatureExposedToJava(ConvertJavaStringToUTF8(env, jfeature_name));
  const std::string& param_name = ConvertJavaStringToUTF8(env, jparam_name);
  return base::GetFieldTrialParamByFeatureAsBool(*feature, param_name,
                                                 jdefault_value);
}

}  // namespace android
}  // namespace chrome
