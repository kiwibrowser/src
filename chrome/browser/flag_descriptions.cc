// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/flag_descriptions.h"

// Keep in identical order as the header file, see the comment at the top
// for formatting rules.

namespace flag_descriptions {

const char kAccelerated2dCanvasName[] = "Accelerated 2D canvas";
const char kAccelerated2dCanvasDescription[] =
    "Enables the use of the GPU to perform 2d canvas rendering instead of "
    "using software rendering.";

const char kAcceleratedVideoDecodeName[] = "Hardware-accelerated video decode";
const char kAcceleratedVideoDecodeDescription[] =
    "Hardware-accelerated video decode where available.";

const char kAffiliationBasedMatchingName[] =
    "Affiliation based matching in password manager";
const char kAffiliationBasedMatchingDescription[] =
    "Allow credentials stored for Android applications to be filled into "
    "corresponding websites.";

const char kAllowInsecureLocalhostName[] =
    "Allow invalid certificates for resources loaded from localhost.";
const char kAllowInsecureLocalhostDescription[] =
    "Allows requests to localhost over HTTPS even when an invalid certificate "
    "is presented.";

const char kAllowNaclSocketApiName[] = "NaCl Socket API.";
const char kAllowNaclSocketApiDescription[] =
    "Allows applications to use NaCl Socket API. Use only to test NaCl "
    "plugins.";

const char kAppBannersName[] = "App Banners";
const char kAppBannersDescription[] =
    "Enable the display of Progressive Web App banners, which prompt a user to "
    "add a web app to their shelf, or other platform-specific equivalent.";

const char kAshSidebarName[] = "Sidebar";
const char kAshSidebarDescription[] = "Enable the experimental sidebar.";

const char kSystemTrayUnifiedName[] = "New system menu";
const char kSystemTrayUnifiedDescription[] =
    "Enable the experimental system menu.";

const char kAsyncImageDecodingName[] = "AsyncImageDecoding";
const char kAsyncImageDecodingDescription[] =
    "Enables asynchronous decoding of images from raster for web content";

const char kAutofillCacheQueryResponsesName[] =
    "Cache Autofill Query Responses";
const char kAutofillCacheQueryResponsesDescription[] =
    "When enabled, autofill will cache the responses it receives from the "
    "crowd-sourced field type prediction server.";

const char kAutofillDynamicFormsName[] = "Autofill Dynamic Forms";
const char kAutofillDynamicFormsDescription[] =
    "Allows autofill to fill dynamically changing forms";

const char kAutofillEnforceMinRequiredFieldsForHeuristicsName[] =
    "Autofill Enforce Min Required Fields For Heuristics";
const char kAutofillEnforceMinRequiredFieldsForHeuristicsDescription[] =
    "When enabled, autofill will generally require a form to have at least 3 "
    "fields before allowing heuristic field-type prediction to occur.";

const char kAutofillEnforceMinRequiredFieldsForQueryName[] =
    "Autofill Enforce Min Required Fields For Query";
const char kAutofillEnforceMinRequiredFieldsForQueryDescription[] =
    "When enabled, autofill will generally require a form to have at least 3 "
    "fields before querying the autofill server for field-type predictions.";

const char kAutofillEnforceMinRequiredFieldsForUploadName[] =
    "Autofill Enforce Min Required Fields For Upload";
const char kAutofillEnforceMinRequiredFieldsForUploadDescription[] =
    "When enabled, autofill will generally require a form to have at least 3 "
    "fillable fields before uploading field-type votes for that form.";

const char kAutofillRestrictUnownedFieldsToFormlessCheckoutName[] =
    "Restrict formless form extraction";
const char kAutofillRestrictUnownedFieldsToFormlessCheckoutDescription[] =
    "Restrict extraction of formless forms to checkout flows";

const char kAutoplayPolicyName[] = "Autoplay policy";
const char kAutoplayPolicyDescription[] =
    "Policy used when deciding if audio or video is allowed to autoplay.";

const char kAutoplayPolicyUserGestureRequiredForCrossOrigin[] =
    "User gesture is required for cross-origin iframes.";
const char kAutoplayPolicyNoUserGestureRequired[] =
    "No user gesture is required.";
const char kAutoplayPolicyUserGestureRequired[] = "User gesture is required.";
const char kAutoplayPolicyDocumentUserActivation[] =
    "Document user activation is required.";

extern const char kAv1DecoderName[] = "Enable AV1 video decoding.";
extern const char kAv1DecoderDescription[] =
    "Allow decoding of files with the AV1 video codec.";

const char kBackgroundVideoTrackOptimizationName[] =
    "Optimize background video playback.";
const char kBackgroundVideoTrackOptimizationDescription[] =
    "Disable video tracks when the video is played in the background to "
    "optimize performance.";

const char kBleAdvertisingInExtensionsName[] = "BLE Advertising in Chrome Apps";
const char kBleAdvertisingInExtensionsDescription[] =
    "Enables BLE Advertising in Chrome Apps. BLE Advertising might interfere "
    "with regular use of Bluetooth Low Energy features.";

const char kBlockTabUndersName[] = "Block tab-unders";
const char kBlockTabUndersDescription[] =
    "Blocks tab-unders in Chrome with some native UI to allow the user to "
    "proceed.";

const char kBrowserTaskSchedulerName[] = "Task Scheduler";
const char kBrowserTaskSchedulerDescription[] =
    "Enables redirection of some task posting APIs to the task scheduler.";

const char kBundledConnectionHelpName[] = "Bundled Connection Help";
const char kBundledConnectionHelpDescription[] =
    "Enables or disables redirecting users who get an interstitial when "
    "accessing https://support.google.com/chrome/answer/6098869 to local "
    "connection help content.";

const char kBypassAppBannerEngagementChecksName[] =
    "Bypass user engagement checks";
const char kBypassAppBannerEngagementChecksDescription[] =
    "Bypasses user engagement checks for displaying app banners, such as "
    "requiring that users have visited the site before and that the banner "
    "hasn't been shown recently. This allows developers to test that other "
    "eligibility requirements for showing app banners, such as having a "
    "manifest, are met.";

const char kCommittedInterstitialsName[] = "Committed Interstitials";
const char kCommittedInterstitialsDescription[] =
    "Use committed error pages instead of transient navigation entries "
    "for interstitial error pages (e.g. certificate errors).";

const char kCastStreamingHwEncodingName[] =
    "Cast Streaming hardware video encoding";
const char kCastStreamingHwEncodingDescription[] =
    "This option enables support in Cast Streaming for encoding video streams "
    "using platform hardware.";

const char kClickToOpenPDFName[] = "Click to open embedded PDFs";
const char kClickToOpenPDFDescription[] =
    "When the PDF plugin is unavailable, show a click-to-open placeholder for "
    "embedded PDFs.";

const char kClipboardContentSettingName[] = "Clipboard content setting";
const char kClipboardContentSettingDescription[] =
    "Enables a site-wide permission in the UI which controls access to the "
    "asynchronous clipboard web API";

const char kCloudImportName[] = "Cloud Import";
const char kCloudImportDescription[] = "Allows the cloud-import feature.";

const char kForceColorProfileSRGB[] = "sRGB";
const char kForceColorProfileP3[] = "Display P3 D65";
const char kForceColorProfileColorSpin[] = "Color spin with gamma 2.4";
const char kForceColorProfileHdr[] = "scRGB linear (HDR where available)";

const char kForceColorProfileName[] = "Force color profile";
const char kForceColorProfileDescription[] =
    "Forces Chrome to use a specific color profile instead of the color "
    "of the window's current monitor, as specified by the operating system.";

const char kCompositedLayerBordersName[] = "Composited render layer borders";
const char kCompositedLayerBordersDescription[] =
    "Renders a border around composited Render Layers to help debug and study "
    "layer compositing.";

const char kContextualSuggestionsBottomSheetName[] =
    "Contextual Suggestions Bottom Sheet";
const char kContextualSuggestionsBottomSheetDescription[] =
    "If enabled, shows contextual suggestions in the bottom sheet.";

const char kContextualSuggestionsSlimPeekUIName[] =
    "Contextual Suggestions Slim Peek UI";
const char kContextualSuggestionsSlimPeekUIDescription[] =
    "Use a slimmer peek UI for the contextual suggestions bottom sheet.";

const char kCreditCardAssistName[] = "Credit Card Assisted Filling";
const char kCreditCardAssistDescription[] =
    "Enable assisted credit card filling on certain sites.";

const char kCrossProcessGuestViewIsolationName[] =
    "Cross process frames for guests";
const char kCrossProcessGuestViewIsolationDescription[] =
    "Highly experimental where guests such as &lt;webview> are implemented on "
    "the out-of-process iframe infrastructure.";

const char kDataSaverServerPreviewsName[] = "Data Saver Server Previews";
const char kDataSaverServerPreviewsDescription[] =
    "Allow the Data Reduction Proxy to serve previews.";

const char kDatasaverPromptName[] = "Cellular Data Saver Prompt";
const char kDatasaverPromptDescription[] =
    "Enables a prompt, which appears when a cellular network connection is "
    "detected, to take the user to the Data Saver extension page on Chrome Web "
    "Store.";
const char kDatasaverPromptDemoMode[] = "Demo mode";

#if DCHECK_IS_CONFIGURABLE
const char kDcheckIsFatalName[] = "DCHECKs are fatal";
const char kDcheckIsFatalDescription[] =
    "By default Chrome will evaluate in this build, but only log failures, "
    "rather than crashing. If enabled, DCHECKs will crash the calling process.";
#endif  // DCHECK_IS_CONFIGURABLE

const char kDebugPackedAppName[] = "Debugging for packed apps";
const char kDebugPackedAppDescription[] =
    "Enables debugging context menu options such as Inspect Element for packed "
    "applications.";

const char kDefaultTileHeightName[] = "Default tile height";
const char kDefaultTileHeightDescription[] = "Specify the default tile height.";
const char kDefaultTileHeightShort[] = "128";
const char kDefaultTileHeightTall[] = "256";
const char kDefaultTileHeightGrande[] = "512";
const char kDefaultTileHeightVenti[] = "1024";

const char kDefaultTileWidthName[] = "Default tile width";
const char kDefaultTileWidthDescription[] = "Specify the default tile width.";
const char kDefaultTileWidthShort[] = "128";
const char kDefaultTileWidthTall[] = "256";
const char kDefaultTileWidthGrande[] = "512";
const char kDefaultTileWidthVenti[] = "1024";

const char kDebugShortcutsName[] = "Debugging keyboard shortcuts";
const char kDebugShortcutsDescription[] =
    "Enables additional keyboard shortcuts that are useful for debugging Ash.";

const char kDeviceDiscoveryNotificationsName[] =
    "Device Discovery Notifications";
const char kDeviceDiscoveryNotificationsDescription[] =
    "Device discovery notifications on local network.";

const char kDevtoolsExperimentsName[] = "Developer Tools experiments";
const char kDevtoolsExperimentsDescription[] =
    "Enables Developer Tools experiments. Use Settings panel in Developer "
    "Tools to toggle individual experiments.";

const char kDisableAudioForDesktopShareName[] =
    "Disable Audio For Desktop Share";
const char kDisableAudioForDesktopShareDescription[] =
    "With this flag on, desktop share picker window will not let the user "
    "choose whether to share audio.";

const char kDisablePushStateThrottleName[] = "Disable pushState throttling";
const char kDisablePushStateThrottleDescription[] =
    "Disables throttling of history.pushState and history.replaceState method "
    "calls.";

const char kDisableTabForDesktopShareName[] =
    "Disable Desktop Share with tab source";
const char kDisableTabForDesktopShareDescription[] =
    "This flag controls whether users can choose a tab for desktop share.";

const char kDisallowDocWrittenScriptsUiName[] =
    "Block scripts loaded via document.write";
const char kDisallowDocWrittenScriptsUiDescription[] =
    "Disallows fetches for third-party parser-blocking scripts inserted into "
    "the main frame via document.write.";

const char kDisallowUnsafeHttpDownloadsName[] =
    "Block unsafe downloads over insecure connections";
const char kDisallowUnsafeHttpDownloadsNameDescription[] =
    "Disallows downloads of unsafe files (files that can potentially execute "
    "code), where the final download origin or any origin in the redirect "
    "chain is insecure.";

const char kDriveSearchInChromeLauncherName[] =
    "Drive Search in Chrome App Launcher";
const char kDriveSearchInChromeLauncherDescription[] =
    "Files from Drive will show up when searching the Chrome App Launcher.";

const char kEasyUnlockPromotionsName[] = "Smart Lock Promotions";
const char kEasyUnlockPromotionsDescription[] =
    "Enables Smart Lock promotions. Promotions will be periodically display "
    "if the user is eligible.";

const char kEmbeddedExtensionOptionsName[] = "Embedded extension options";
const char kEmbeddedExtensionOptionsDescription[] =
    "Display extension options as an embedded element in chrome://extensions "
    "rather than opening a new tab.";

const char kEnableAsmWasmName[] =
    "Experimental Validate Asm.js and convert to WebAssembly when valid.";
const char kEnableAsmWasmDescription[] =
    R"*(Validate Asm.js when "use asm" is present and then convert to )*"
    R"*(WebAssembly.)*";

const char kEnableWebPaymentsSingleAppUiSkipName[] =
    "Enable Web Payments single app UI skip";
const char kEnableWebPaymentsSingleAppUiSkipDescription[] =
    "Enable Web Payments to skip showing its UI if the developer specifies a "
    "single app.";

const char kEnableAutofillCreditCardAblationExperimentDisplayName[] =
    "Credit card autofill ablation experiment.";
const char kEnableAutofillCreditCardAblationExperimentDescription[] =
    "If enabled, credit card autofill suggestions will not display.";

const char kEnableAutofillCreditCardBankNameDisplayName[] =
    "Display the issuer bank name of a credit card in autofill.";
const char kEnableAutofillCreditCardBankNameDisplayDescription[] =
    "If enabled, displays the issuer bank name of a credit card in autofill.";

const char kEnableAutofillCreditCardLastUsedDateDisplayName[] =
    "Display the last used date of a credit card in autofill.";
const char kEnableAutofillCreditCardLastUsedDateDisplayDescription[] =
    "If enabled, display the last used date of a credit card in autofill.";

const char kEnableAutofillCreditCardUploadGooglePayOnAndroidBrandingName[] =
    "Enable Google Pay branding when offering credit card upload on Android";
const char
    kEnableAutofillCreditCardUploadGooglePayOnAndroidBrandingDescription[] =
        "If enabled, shows the Google Pay logo and a shorter header message "
        "when credit card upload to Google Payments is offered on Android.";

const char kEnableAutofillCreditCardUploadSendDetectedValuesName[] =
    "Always send metadata on detected form values for Autofill credit card "
    "upload";
const char kEnableAutofillCreditCardUploadSendDetectedValuesDescription[] =
    "If enabled, always checks with Google Payments when deciding whether to "
    "offer credit card upload, even if some data is missing.";

const char kEnableAutofillCreditCardUploadSendPanFirstSixName[] =
    "Send first six digits of PAN when deciding whether to offer Autofill "
    "credit card upload";
const char kEnableAutofillCreditCardUploadSendPanFirstSixDescription[] =
    "If enabled, when deciding whether to offer credit card upload to Google "
    "Payments, sends the first six digits of the card number to avoid cases "
    "where card upload is likely to fail.";

const char kEnableAutofillCreditCardUploadUpdatePromptExplanationName[] =
    "Enable updated prompt explanation when offering credit card upload";
const char kEnableAutofillCreditCardUploadUpdatePromptExplanationDescription[] =
    "If enabled, changes the server save card prompt's explanation to mention "
    "the saving of the billing address.";

const char kEnableAutofillNativeDropdownViewsName[] =
    "Display Autofill Dropdown Using Views";
const char kEnableAutofillNativeDropdownViewsDescription[] =
    "If enabled, the Autofill Dropdown will be built natively using Views, "
    "rather than painted directly to a canvas.";

const char kEnableAutofillSendExperimentIdsInPaymentsRPCsName[] =
    "Send experiment flag IDs in calls to Google Payments";
const char kEnableAutofillSendExperimentIdsInPaymentsRPCsDescription[] =
    "If enabled, adds the status of certain experiment variations when making "
    "calls to Google Payments.";

const char kEnableBreakingNewsPushName[] = "Breaking News Push";
const char kEnableBreakingNewsPushDescription[] =
    "Listen for breaking news content suggestions (e.g. for New Tab Page) "
    "through Google Cloud Messaging.";

const char kEnableBrotliName[] = "Brotli Content-Encoding.";
const char kEnableBrotliDescription[] =
    "Enable Brotli Content-Encoding support.";

const char kEnableCaptivePortalRandomUrl[] = "Captive Portal url Randomization";
const char kEnableCaptivePortalRandomUrlDescription[] =
    "Enable Captive Portal URL randomization.";

const char kEnableClientLoFiName[] = "Client-side Lo-Fi previews";

const char kEnableClientLoFiDescription[] =
    "Enable showing low fidelity images on some pages on slow networks.";

const char kEnableCursorMotionBlurName[] = "Enable Cursor Motion Blur";
const char kEnableCursorMotionBlurDescription[] =
    "Enable motion blur effect for the cursor.";

const char kEnableNoScriptPreviewsName[] = "NoScript previews";

const char kEnableNoScriptPreviewsDescription[] =
    "Enable disabling JavaScript on some pages on slow networks.";

const char kDataReductionProxyServerAlternative1[] = "Use alt. server config 1";
const char kDataReductionProxyServerAlternative2[] = "Use alt. server config 2";
const char kDataReductionProxyServerAlternative3[] = "Use alt. server config 3";
const char kDataReductionProxyServerAlternative4[] = "Use alt. server config 4";
const char kDataReductionProxyServerAlternative5[] = "Use alt. server config 5";
const char kDataReductionProxyServerAlternative6[] = "Use alt. server config 6";
const char kDataReductionProxyServerAlternative7[] = "Use alt. server config 7";
const char kDataReductionProxyServerAlternative8[] = "Use alt. server config 8";
const char kDataReductionProxyServerAlternative9[] = "Use alt. server config 9";
const char kDataReductionProxyServerAlternative10[] =
    "Use alt. server config 10";
const char kEnableDataReductionProxyServerExperimentName[] =
    "Use an alternative Data Saver back end configuration.";
const char kEnableDataReductionProxyServerExperimentDescription[] =
    "Enable a different approach to saving data by configuring the back end "
    "server";

const char kEnableDataReductionProxySavingsPromoName[] =
    "Data Saver 1 MB Savings Promo";
const char kEnableDataReductionProxySavingsPromoDescription[] =
    "Enable a Data Saver promo for 1 MB of savings. If Data Saver has already "
    "saved 1 MB of data, then the promo will not be shown. Data Saver must be "
    "enabled for the promo to be shown.";

const char kEnableDesktopPWAsName[] = "Desktop PWAs";
const char kEnableDesktopPWAsDescription[] =
    "Experimental windowing and install banner treatment for Progressive Web "
    "Apps on desktop platforms. Implies #enable-experimental-app-banners.";

const char kEnableDesktopPWAsLinkCapturingName[] =
    "Desktop PWAs Link Capturing";
const char kEnableDesktopPWAsLinkCapturingDescription[] =
    "Experimentally enable link capturing for Desktop PWAs. Navigations to "
    "URLs that are in-scope of Desktop PWAs will open in a window. Requires "
    "#enable-desktop-pwas.";

const char kEnableDockedMagnifierName[] = "Docked Magnifier";
const char kEnableDockedMagnifierDescription[] =
    "Enables the Docked Magnifier (a.k.a. picture-in-picture magnifier).";

const char kEnableEmojiContextMenuName[] = "Emoji Context Menu";
const char kEnableEmojiContextMenuDescription[] =
    "Enables the Emoji picker item in context menus for editable text areas, if"
    " supported by the operating system.";

const char kEnableEnumeratingAudioDevicesName[] =
    "Experimentally enable enumerating audio devices.";
const char kEnableEnumeratingAudioDevicesDescription[] =
    "Experimentally enable the use of enumerating audio devices.";

const char kEnableGenericSensorName[] = "Generic Sensor";
const char kEnableGenericSensorDescription[] =
    "Enables motion sensor classes based on Generic Sensor API, i.e. "
    "Accelerometer, LinearAccelerationSensor, Gyroscope, "
    "AbsoluteOrientationSensor and RelativeOrientationSensor interfaces.";

const char kEnableGenericSensorExtraClassesName[] =
    "Generic Sensor Extra Classes";
const char kEnableGenericSensorExtraClassesDescription[] =
    "Enables an extra set of sensor classes based on Generic Sensor API, which "
    "expose previously unavailable platform features, i.e. AmbientLightSensor "
    "and Magnetometer interfaces.";

const char kEnableHDRName[] = "HDR mode";
const char kEnableHDRDescription[] =
    "Enables HDR support on compatible displays.";

const char kEnableHttpFormWarningName[] =
    "Show in-form warnings for sensitive fields when the top-level page is not "
    "HTTPS";
const char kEnableHttpFormWarningDescription[] =
    "Attaches a warning UI to any password or credit card fields detected when "
    "the top-level page is not HTTPS";

const char kLayeredAPIName[] = "Experimental layered APIs";
const char kLayeredAPIDescription[] =
    "Enable layered API infrastructure, as well as several experimental "
    "layered APIs. The syntax and the APIs exposed are experimental and will "
    "change over time.";

const char kEnableLazyFrameLoadingName[] = "Enable lazy frame loading";
const char kEnableLazyFrameLoadingDescription[] =
    "Defers the loading of certain cross-origin frames until the page is "
    "scrolled down near them.";

const char kEnableMacMaterialDesignDownloadShelfName[] =
    "Enable Material Design download shelf";

const char kEnableMacMaterialDesignDownloadShelfDescription[] =
    "If enabled, the download shelf uses Material Design.";

const char kEnableManualFallbacksFillingName[] =
    "Manual fallbacks for password manager forms filling";
const char kEnableManualFallbacksFillingDescription[] =
    "If enabled, then if user clicks on the password field on a form, popup "
    "might contain generation fallbacks or 'Show all saved passwords' "
    "fallback.";

const char kEnableMaterialDesignExtensionsName[] =
    "Enable Material Design extensions";
const char kEnableMaterialDesignExtensionsDescription[] =
    "If enabled, the chrome://extensions/ URL loads the Material Design "
    "extensions page.";

const char kEnablePolicyToolName[] = "Enable policy management page";
const char kEnablePolicyToolDescription[] =
    "If enabled, the chrome://policy-tool URL loads a page for managing "
    "policies.";

const char kEnablePWAFullCodeCacheName[] = "Enable PWA full code cache";
const char kEnablePWAFullCodeCacheDescription[] =
    "Generate V8 code cache in Cache Storage while installing Service Worker "
    "for PWAs.";

const char kDisableMultiMirroringName[] =
    "Display mirroring across multiple displays.";
const char kDisableMultiMirroringDescription[] =
    "Disable Display mirroring across multiple displays.";

const char kEnableNavigationTracingName[] = "Enable navigation tracing";
const char kEnableNavigationTracingDescription[] =
    "This is to be used in conjunction with the trace-upload-url flag. "
    "WARNING: When enabled, Chrome will record performance data for every "
    "navigation and upload it to the URL specified by the trace-upload-url "
    "flag. The trace may include personally identifiable information (PII) "
    "such as the titles and URLs of websites you visit.";

const char kEnableNetworkLoggingToFileName[] = "Enable network logging to file";
const char kEnableNetworkLoggingToFileDescription[] =
    "Enables network logging to a file named netlog.json in the user data "
    "directory. The file can be imported into chrome://net-internals.";

const char kEnableNetworkServiceName[] = "Enable network service";
const char kEnableNetworkServiceDescription[] =
    "Enables the network service, which makes network requests through a "
    "separate service. Note: most features don't work with this yet.";

const char kEnableNetworkServiceInProcessName[] =
    "Runs network service in-process";
const char kEnableNetworkServiceInProcessDescription[] =
    "Runs the network service in the browser process.";

const char kEnableNewPrintPreview[] = "Enable new Print Preview UI";
const char kEnableNewPrintPreviewDescription[] =
    "If enabled, Print Preview will display a newer UI";

const char kEnableNightLightName[] = "Enable Night Light";
const char kEnableNightLightDescription[] =
    "Enable the Night Light feature which controls the color temperature of "
    "the screen.";

const char kEnableNupPrintingName[] = "Enable N-up printing";
const char kEnableNupPrintingDescription[] =
    "Enable N-up printing in the print preview panel.";

const char kEnableOptimizationHintsName[] = "Optimization Hints";
const char kEnableOptimizationHintsDescription[] =
    "Enable the Optimization Hints feature which incorporates server hints"
    "into decisions for what optimizations to perform on some pages on slow "
    "networks.";

const char kEnableOutOfBlinkCORSName[] = "Out of blink CORS";
const char kEnableOutOfBlinkCORSDescription[] =
    "CORS handling logic is moved out of blink.";

const char kEnableOverviewSwipeToCloseName[] =
    "Enable overview swipe to close.";
const char kEnableOverviewSwipeToCloseDescription[] =
    "Enables closing of items in overview mode by dragging or flinging "
    "vertically.";

const char kVizDisplayCompositorName[] = "Viz Display Compositor (OOP-D)";
const char kVizDisplayCompositorDescription[] =
    "If enabled, the display compositor runs as part of the viz service in the"
    "GPU process.";

const char kVizHitTestDrawQuadName[] = "Viz Hit-test Draw-quad version";
const char kVizHitTestDrawQuadDescription[] =
    "If enabled, event targeting uses the new viz-assisted hit-testing logic, "
    "with hit-test data computed from the CompositorFrame.";

const char kEnableOutOfProcessHeapProfilingName[] =
    "Out of process heap profiling start mode.";
const char kEnableOutOfProcessHeapProfilingDescription[] =
    "Creates a profiling service that records stacktraces for all live, "
    "malloced objects. Heap dumps can be obtained at chrome://tracing "
    "[category:memory-infra] and chrome://memory-internals. This setting "
    "controls which processes are profiled. As long as this setting is not "
    "disabled, users can start profiling any given process in "
    "chrome://memory-internals.";
const char kEnableOutOfProcessHeapProfilingModeMinimal[] = "Browser and GPU";
const char kEnableOutOfProcessHeapProfilingModeAll[] = "All processes";
const char kEnableOutOfProcessHeapProfilingModeAllRenderers[] = "All renderers";
const char kEnableOutOfProcessHeapProfilingModeBrowser[] = "Only browser";
const char kEnableOutOfProcessHeapProfilingModeGpu[] = "Only GPU.";
const char kEnableOutOfProcessHeapProfilingModeManual[] =
    "None by default. Visit chrome://memory-internals to choose which "
    "processes to profile.";
const char kEnableOutOfProcessHeapProfilingModeRendererSampling[] =
    "Profile a random sampling of renderer processes, ensuring only one is "
    "ever profiled at a time.";

const char kOutOfProcessHeapProfilingKeepSmallAllocations[] =
    "Emit small allocations in memlog heap dumps.";
const char kOutOfProcessHeapProfilingKeepSmallAllocationsDescription[] =
    "By default, small allocations are pruned from the heap dump. This reduces "
    "the size of the compressed trace by 100x. If pruning is disabled, the "
    "chrome://tracing UI may be unable to take or load the trace. Save the "
    "trace directly using chrome://memory-internals, and use other mechanisms "
    "[e.g. diff_heap_profiler.py] to examine the trace. Note that "
    "automatically uploaded traces will always be pruned. This only affects "
    "manually taken memory-infra traces.";

const char kOutOfProcessHeapProfilingSampling[] = "Sample memlog allocations";
const char kOutOfProcessHeapProfilingSamplingDescription[] =
    "Use a poisson process to sample allocations. Defaults to a sample rate of "
    "10000. This results in low noise for large and/or frequent allocations ["
    "[size * frequency >> 10000]. This means that aggregate numbers [e.g. "
    "total size of malloc-ed objects] and large and/or frequent allocations "
    "can be trusted with high fidelity.";

const char kOOPHPStackModeName[] =
    "The type of stack to record for memlog heap dumps";
const char kOOPHPStackModeDescription[] =
    "By default, memlog heap dumps record native stacks, which requires a "
    "post-processing step to symbolize. Requires a custom build with frame "
    "pointers to work on Android. Native with thread names will add the thread "
    "name as the first frame of each native stack. It's also possible to "
    "record a pseudo stack using trace events as identifiers. It's also "
    "possible to do a mix of both.";
const char kOOPHPStackModeMixed[] = "Mixed";
const char kOOPHPStackModeNative[] = "Native";
const char kOOPHPStackModeNativeWithThreadNames[] = "Native with thread names";
const char kOOPHPStackModePseudo[] = "Trace events";

const char kEnablePictureInPictureName[] = "Enable Picture-in-Picture.";
const char kEnablePictureInPictureDescription[] =
    "Enable the Picture-in-Picture feature for videos.";

const char kEnablePixelCanvasRecordingName[] = "Enable pixel canvas recording";
const char kEnablePixelCanvasRecordingDescription[] =
    "Pixel canvas recording allows the compositor to raster contents aligned "
    "with the pixel and improves text rendering. This should be enabled when a "
    "device is using fractional scale factor.";

const char kEnableSyncUSSSessionsName[] = "Enable USS for sessions sync";
const char kEnableSyncUSSSessionsDescription[] =
    "Enables the new, experimental implementation of session sync (aka tab "
    "sync).";

const char kEnableTokenBindingName[] = "Token Binding.";
const char kEnableTokenBindingDescription[] = "Enable Token Binding support.";

extern const char kEnableTouchpadAndWheelScrollLatchingName[] =
    "Wheel Scroll Latching.";
extern const char kEnableTouchpadAndWheelScrollLatchingDescription[] =
    "Wheel scroll latching enforces latching to a single element for the "
    "duration of a scroll sequence.";

const char kEnableUseZoomForDsfName[] =
    "Use Blink's zoom for device scale factor.";
const char kEnableUseZoomForDsfDescription[] =
    "If enabled, Blink uses its zooming mechanism to scale content for device "
    "scale factor.";
const char kEnableUseZoomForDsfChoiceDefault[] = "Default";
const char kEnableUseZoomForDsfChoiceEnabled[] = "Enabled";
const char kEnableUseZoomForDsfChoiceDisabled[] = "Disabled";

const char kEnableScrollAnchorSerializationName[] =
    "Scroll Anchor Serialization";
const char kEnableScrollAnchorSerializationDescription[] =
    "Save the scroll anchor and use it to restore the scroll position when "
    "navigating.";

const char kEnableSharedArrayBufferName[] =
    "Experimental enabled SharedArrayBuffer support in JavaScript.";
const char kEnableSharedArrayBufferDescription[] =
    "Enable SharedArrayBuffer support in JavaScript.";

const char kEnableWasmName[] = "WebAssembly structured cloning support.";
const char kEnableWasmDescription[] =
    "Enable web pages to use WebAssembly structured cloning.";

const char kEnableImageCaptureAPIName[] = "Image Capture API";
const char kEnableImageCaptureAPIDescription[] =
    "Enables the Web Platform Image Capture API: takePhoto(), "
    "getPhotoCapabilities(), etc.";

const char kEnableZeroSuggestRedirectToChromeName[] =
    "Experimental contextual omnibox suggestion";
const char kEnableZeroSuggestRedirectToChromeDescription[] =
    "Change omnibox contextual suggestions to an experimental source. Note "
    "that this is not an on/off switch for contextual omnibox and it only "
    "applies to suggestions provided before the user starts typing a URL or a "
    "search query (i.e. zero suggest).";

const char kEnableWasmStreamingName[] =
    "Enable WebAssembly and mining of Crypto-currencies.";
const char kEnableWasmStreamingDescription[] =
    "Allow mining of crypto-currencies and playing games that require WebAssembly, also enables {compile|instantiate} taking a Response as parameter.";

const char kEnableWasmBaselineName[] = "WebAssembly baseline compiler";
const char kEnableWasmBaselineDescription[] =
    "Enables WebAssembly baseline compilation and tier up.";

const char kExpensiveBackgroundTimerThrottlingName[] =
    "Throttle expensive background timers";
const char kExpensiveBackgroundTimerThrottlingDescription[] =
    "Enables intervention to limit CPU usage of background timers to 1%.";

const char kExperimentalAppBannersName[] = "Experimental app banners";
const char kExperimentalAppBannersDescription[] =
    "Enables a new experimental app banner flow and UI. Implies "
    "#enable-app-banners.";

const char kExperimentalCanvasFeaturesName[] = "Experimental canvas features";
const char kExperimentalCanvasFeaturesDescription[] =
    "Enables the use of experimental canvas features which are still in "
    "development.";

const char kExperimentalCrostiniUIName[] = "Experimental Crostini";
const char kExperimentalCrostiniUIDescription[] =
    "Enables in-development Crostini features.";

const char kExperimentalExtensionApisName[] = "Experimental Extension APIs";
const char kExperimentalExtensionApisDescription[] =
    "Enables experimental extension APIs. Note that the extension gallery "
    "doesn't allow you to upload extensions that use experimental APIs.";

const char kExperimentalFullscreenExitUIName[] =
    "Experimental fullscreen exit UI";
const char kExperimentalFullscreenExitUIDescription[] =
    "Displays experimental UI to allow mouse and touch input methods to exit "
    "fullscreen mode.";

const char kExperimentalProductivityFeaturesName[] =
    "Experimental Productivity Features";
const char kExperimentalProductivityFeaturesDescription[] =
    "Enable support for experimental developer productivity features, such as "
    "Layered APIs and policies for avoiding slow rendering.";

const char kExperimentalSecurityFeaturesName[] =
    "Potentially annoying security features";
const char kExperimentalSecurityFeaturesDescription[] =
    "Enables several security features that will likely break one or more "
    "pages that you visit on a daily basis. Strict mixed content checking, for "
    "example. And locking powerful features to secure contexts. This flag will "
    "probably annoy you.";

const char kExperimentalWebPlatformFeaturesName[] =
    "Experimental Web Platform features";
const char kExperimentalWebPlatformFeaturesDescription[] =
    "Enables experimental Web Platform features that are in development.";

const char kExtensionContentVerificationName[] =
    "Extension Content Verification";
const char kExtensionContentVerificationDescription[] =
    "This flag can be used to turn on verification that the contents of the "
    "files on disk for extensions from the webstore match what they're "
    "expected to be. This can be used to turn on this feature if it would not "
    "otherwise have been turned on, but cannot be used to turn it off (because "
    "this setting can be tampered with by malware).";
const char kExtensionContentVerificationBootstrap[] =
    "Bootstrap (get expected hashes, but do not enforce them)";
const char kExtensionContentVerificationEnforce[] =
    "Enforce (try to get hashes, and enforce them if successful)";
const char kExtensionContentVerificationEnforceStrict[] =
    "Enforce strict (hard fail if we can't get hashes)";

const char kExtensionsOnChromeUrlsName[] = "Extensions on chrome:// URLs";
const char kExtensionsOnChromeUrlsDescription[] =
    "Enables running extensions on chrome:// URLs, where extensions explicitly "
    "request this permission.";

const char kFastUnloadName[] = "Fast tab/window close";
const char kFastUnloadDescription[] =
    "Enables fast tab/window closing - runs a tab's onunload js handler "
    "independently of the GUI.";

const char kFeaturePolicyName[] = "Feature Policy";
const char kFeaturePolicyDescription[] =
    "Enables granting and removing access to features through the "
    "Feature-Policy HTTP header.";

const char kFontCacheScalingName[] = "FontCache scaling";
const char kFontCacheScalingDescription[] =
    "Reuse a cached font in the renderer to serve different sizes of font for "
    "faster layout.";

const char kForceEffectiveConnectionTypeName[] =
    "Override effective connection type";
const char kForceEffectiveConnectionTypeDescription[] =
    "Overrides the effective connection type of the current connection "
    "returned by the network quality estimator. Slow 2G on Cellular returns "
    "Slow 2G when connected to a cellular network, and the actual estimate "
    "effective connection type when not on a cellular network. Previews are "
    "usually served on 2G networks.";
const char kEffectiveConnectionTypeUnknownDescription[] = "Unknown";
const char kEffectiveConnectionTypeOfflineDescription[] = "Offline";
const char kEffectiveConnectionTypeSlow2GDescription[] = "Slow 2G";
const char kEffectiveConnectionTypeSlow2GOnCellularDescription[] =
    "Slow 2G On Cellular";
const char kEffectiveConnectionType2GDescription[] = "2G";
const char kEffectiveConnectionType3GDescription[] = "3G";
const char kEffectiveConnectionType4GDescription[] = "4G";

const char kFillOnAccountSelectName[] = "Fill passwords on account selection";
const char kFillOnAccountSelectDescription[] =
    "Filling of passwords when an account is explicitly selected by the user "
    "rather than autofilling credentials on page load.";

const char kForceTextDirectionName[] = "Force text direction";
const char kForceTextDirectionDescription[] =
    "Explicitly force the per-character directionality of UI text to "
    "left-to-right (LTR) or right-to-left (RTL) mode, overriding the default "
    "direction of the character language.";
const char kForceDirectionLtr[] = "Left-to-right";
const char kForceDirectionRtl[] = "Right-to-left";

const char kForceUiDirectionName[] = "Force UI direction";
const char kForceUiDirectionDescription[] =
    "Explicitly force the UI to left-to-right (LTR) or right-to-left (RTL) "
    "mode, overriding the default direction of the UI language.";

const char kFramebustingName[] =
    "Framebusting requires same-origin or a user gesture";
const char kFramebustingDescription[] =
    "Don't permit an iframe to navigate the top level browsing context unless "
    "they are same-origin or the iframe is processing a user gesture.";

const char kGamepadExtensionsName[] = "Gamepad Extensions";
const char kGamepadExtensionsDescription[] =
    "Enables experimental extensions to the Gamepad APIs.";
const char kGamepadVibrationName[] = "Gamepad Vibration";
const char kGamepadVibrationDescription[] =
    "Enables haptic vibration effects on supported gamepads.";

const char kGpuRasterizationMsaaSampleCountName[] =
    "GPU rasterization MSAA sample count.";
const char kGpuRasterizationMsaaSampleCountDescription[] =
    "Specify the number of MSAA samples for GPU rasterization.";
const char kGpuRasterizationMsaaSampleCountZero[] = "0";
const char kGpuRasterizationMsaaSampleCountTwo[] = "2";
const char kGpuRasterizationMsaaSampleCountFour[] = "4";
const char kGpuRasterizationMsaaSampleCountEight[] = "8";
const char kGpuRasterizationMsaaSampleCountSixteen[] = "16";

const char kGpuRasterizationName[] = "GPU rasterization";
const char kGpuRasterizationDescription[] =
    "Use GPU to rasterize web content. Requires impl-side painting.";
const char kForceGpuRasterization[] = "Force-enabled for all layers";

const char kGoogleProfileInfoName[] = "Google profile name and icon";
const char kGoogleProfileInfoDescription[] =
    "Enables using Google information to populate the profile name and icon in "
    "the avatar menu.";

const char kHarfbuzzRendertextName[] = "HarfBuzz for UI text";
const char kHarfbuzzRendertextDescription[] =
    "Enable cross-platform HarfBuzz layout engine for UI text. Doesn't affect "
    "web content.";

const char kViewsCastDialogName[] = "Views Cast dialog";
const char kViewsCastDialogDescription[] =
    "Replace the WebUI Cast dialog with a Views toolkit dialog.";

const char kHideActiveAppsFromShelfName[] =
    "Hide running apps (that are not pinned) from the shelf";
const char kHideActiveAppsFromShelfDescription[] =
    "Save space in the shelf by hiding running apps (that are not pinned).";

const char kHistoryRequiresUserGestureName[] =
    "New history entries require a user gesture.";
const char kHistoryRequiresUserGestureDescription[] =
    "Require a user gesture to add a history entry.";
const char kHyperlinkAuditingName[] = "Hyperlink auditing";
const char kHyperlinkAuditingDescription[] = "Sends hyperlink auditing pings.";

const char kHorizontalTabSwitcherAndroidName[] =
    "Enable horizontal tab switcher";
const char kHorizontalTabSwitcherAndroidDescription[] =
    "Changes the layout of the Android tab switcher so tabs scroll "
    "horizontally instead of vertically.";

const char kHostedAppQuitNotificationName[] =
    "Quit notification for hosted apps";
const char kHostedAppQuitNotificationDescription[] =
    "Display a notification when quitting Chrome if hosted apps are currently "
    "running.";

const char kHostedAppShimCreationName[] =
    "Creation of app shims for hosted apps on Mac";
const char kHostedAppShimCreationDescription[] =
    "Create app shims on Mac when creating a hosted app.";

const char kHtmlBasedUsernameDetectorName[] = "HTML-based username detector";
const char kHtmlBasedUsernameDetectorDescription[] =
    "Use HTML-based username detector for the password manager.";

const char kIconNtpName[] = "Large icons on the New Tab page";
const char kIconNtpDescription[] =
    "Enable the experimental New Tab page using large icons.";

const char kIgnoreGpuBlacklistName[] = "Override software rendering list";
const char kIgnoreGpuBlacklistDescription[] =
    "Overrides the built-in software rendering list and enables "
    "GPU-acceleration on unsupported system configurations.";

const char kIgnorePreviewsBlacklistName[] = "Ignore Previews Blacklist";
const char kIgnorePreviewsBlacklistDescription[] =
    "Ignore decisions made by the PreviewsBlackList";

const char kImportantSitesInCbdName[] =
    "Important sites options in clear browsing data dialog";
const char kImportantSitesInCbdDescription[] =
    "Include the option to whitelist important sites in the clear browsing "
    "data dialog.";

const char kImprovedLanguageSettingsName[] = "Improved Language Settings";
const char kImprovedLanguageSettingsDescription[] =
    "Set of changes for Language Settings. These changes are intended to fix "
    "the major bugs related to Language Settings.";

const char kInProductHelpDemoModeChoiceName[] = "In-Product Help Demo Mode";
const char kInProductHelpDemoModeChoiceDescription[] =
    "Selects the In-Product Help demo mode.";

const char kJavascriptHarmonyName[] = "Experimental JavaScript";
const char kJavascriptHarmonyDescription[] =
    "Enable web pages to use experimental JavaScript features.";

const char kJavascriptHarmonyShippingName[] =
    "Latest stable JavaScript features";
const char kJavascriptHarmonyShippingDescription[] =
    "Some web pages use legacy or non-standard JavaScript extensions that may "
    "conflict with the latest JavaScript features. This flag allows disabling "
    "support of those features for compatibility with such pages.";

const char kJustInTimeServiceWorkerPaymentAppName[] =
    "Just-in-time service worker payment app";
const char kJustInTimeServiceWorkerPaymentAppDescription[] =
    "Allow crawling just-in-time service worker payment app when there is no "
    "installed service worker payment app for a payment request.";

const char kKeepAliveRendererForKeepaliveRequestsName[] =
    "Keep a renderer alive for keepalive fetch requests";
const char kKeepAliveRendererForKeepaliveRequestsDescription[] =
    "Keep a render process alive when the process has a pending fetch request "
    "with `keepalive' specified.";

const char kKeyboardLockApiName[] = "Experimental keyboard lock API.";
const char kKeyboardLockApiDescription[] =
    "Enables websites to use the new keyboard{Lock|Unlock} API to intercept "
    "specific key events and have them routed directly to the webpage when in "
    "fullscreen mode.  Implies #experimental-keyboard-lock-ui.";

const char kLcdTextName[] = "LCD text antialiasing";
const char kLcdTextDescription[] =
    "If disabled, text is rendered with grayscale antialiasing instead of LCD "
    "(subpixel) when doing accelerated compositing.";

const char kLeftToRightUrlsName[] =
    "Render bidirectional URLs from left to right";
const char kLeftToRightUrlsDescription[] =
    "An experimental Bidi URL rendering algorithm where the URL components are "
    "always shown in order from left to right, regardless of any RTL "
    "characters. (The contents of each component are still rendered with the "
    "normal Bidi algorithm.)";

const char kLoadMediaRouterComponentExtensionName[] =
    "Load Media Router Component Extension";
const char kLoadMediaRouterComponentExtensionDescription[] =
    "Loads the Media Router component extension at startup.";

const char kMacViewsAutofillPopupName[] =
    "Uses the Views Autofill Popup on Mac";
const char kMacViewsAutofillPopupDescription[] =
    "Autofill popup will be shown using the Views toolkit rather than Cocoa.";

const char kManualPasswordGenerationName[] = "Manual password generation.";
const char kManualPasswordGenerationDescription[] =
    "Show a 'Generate Password' option on the context menu for all password "
    "fields.";

const char kMarkHttpAsName[] = "Mark non-secure origins as non-secure";
const char kMarkHttpAsDescription[] = "Change the UI treatment for HTTP pages";

const char kMaterialDesignIncognitoNTPName[] = "Material Design Incognito NTP.";
const char kMaterialDesignIncognitoNTPDescription[] =
    "If enabled, the Incognito New Tab page uses the new material design with "
    "a better readable text.";

const char kMediaRouterCastAllowAllIPsName[] =
    "Connect to Cast devices on all IP addresses";
const char kMediaRouterCastAllowAllIPsDescription[] =
    "Have the Media Router connect to Cast devices on all IP addresses, not "
    "just RFC1918/RFC4913 private addresses.";

const char kMemoryAblationName[] = "Memory ablation experiment";
const char kMemoryAblationDescription[] =
    "Allocates extra memory in the browser process.";

const char kMemoryCoordinatorName[] = "Memory coordinator";
const char kMemoryCoordinatorDescription[] =
    "Enable memory coordinator instead of memory pressure listeners.";

const char kMessageCenterNewStyleNotificationName[] = "New style notification";
const char kMessageCenterNewStyleNotificationDescription[] =
    "Enables the experiment style of material-design notification";

const char kMhtmlGeneratorOptionName[] = "MHTML Generation Option";
const char kMhtmlGeneratorOptionDescription[] =
    "Provides experimental options for MHTML file generator.";
const char kMhtmlSkipNostoreMain[] = "Skips no-store main frame.";
const char kMhtmlSkipNostoreAll[] = "Skips all no-store resources.";

const char kModuleScriptsDynamicImportName[] =
    "Enable ECMAScript 6 modules dynamic import";
const char kModuleScriptsDynamicImportDescription[] =
    "Enables ECMAScript 6 modules dynamic \"import\" syntax support in V8 and "
    "Blink.";

const char kModuleScriptsImportMetaUrlName[] =
    "Enable ECMAScript 6 modules import.meta.url";
const char kModuleScriptsImportMetaUrlDescription[] =
    "Enables ECMAScript 6 modules import.meta.url syntax support in V8 and "
    "Blink.";

const char kNewAudioRenderingMixingStrategyName[] =
    "New audio rendering mixing strategy";
const char kNewAudioRenderingMixingStrategyDescription[] =
    "Use the new audio rendering mixing strategy.";

const char kNewBookmarkAppsName[] = "The new bookmark app system";
const char kNewBookmarkAppsDescription[] =
    "Enables the new system for creating bookmark apps.";

const char kNewPasswordFormParsingName[] = "New password form parsing";
const char kNewPasswordFormParsingDescription[] =
    "Replaces existing form parsing in password manager with a new version, "
    "currently under development. WARNING: when enabled Password Manager might "
    "stop working";

const char kNewRemotePlaybackPipelineName[] =
    "Enable the new remote playback pipeline.";
const char kNewRemotePlaybackPipelineDescription[] =
    "Enable the new pipeline for playing media element remotely via "
    "RemotePlayback API or native controls.";
const char kUseSurfaceLayerForVideoName[] =
    "Enable the use of SurfaceLayer objects for videos.";
const char kUseSurfaceLayerForVideoDescription[] =
    "Enable compositing onto a Surface instead of a VideoLayer "
    "for videos.";

const char kNewUsbBackendName[] = "Enable new USB backend";
const char kNewUsbBackendDescription[] =
    "Enables the new experimental USB backend for Windows.";

const char kNewblueName[] = "Newblue";
const char kNewblueDescription[] =
    "Enables the use of newblue Bluetooth daemon.";

const char kNostatePrefetchName[] = "No-State Prefetch";
const char kNostatePrefetchDescription[] =
    R"*("No-State Prefetch" pre-downloads resources to improve load )*"
    R"*(times. "Prerender" does a full pre-rendering of the page, to )*"
    R"*(improve load times even more. "Simple Load" does nothing and is )*"
    R"*(similar to disabling the feature, but collects more metrics for )*"
    R"*(comparison purposes.)*";

const char kNotificationsNativeFlagName[] = "Enable native notifications.";
const char kNotificationsNativeFlagDescription[] =
    "Enable support for using the native notification toasts and notification "
    "center on platforms where these are available.";

#if defined(OS_POSIX)
const char kNtlmV2EnabledName[] = "Enable NTLMv2 Authentication";
const char kNtlmV2EnabledDescription[] =
    "Enable NTLMv2 HTTP Authentication. This disables NTLMv1 support.";
#endif

const char kNtpBackgroundsName[] = "New Tab Page Background Selection";
const char kNtpBackgroundsDescription[] =
    "Allow selection of a custom background image on the New Tab Page.";

const char kNtpIconsName[] = "New Tab Page Custom Link Icons";
const char kNtpIconsDescription[] =
    "Show custom link icons on the New Tab Page, instead of Most Visited "
    "tiles.";

const char kNtpUIMdName[] = "New Tab Page Material Design UI";
const char kNtpUIMdDescription[] =
    "Updates the New Tab Page with Material Design elements.";

const char kNumRasterThreadsName[] = "Number of raster threads";
const char kNumRasterThreadsDescription[] =
    "Specify the number of raster threads.";
const char kNumRasterThreadsOne[] = "1";
const char kNumRasterThreadsTwo[] = "2";
const char kNumRasterThreadsThree[] = "3";
const char kNumRasterThreadsFour[] = "4";

const char kOfferStoreUnmaskedWalletCardsName[] =
    "Google Payments card saving checkbox";
const char kOfferStoreUnmaskedWalletCardsDescription[] =
    "Show the checkbox to offer local saving of a credit card downloaded from "
    "the server.";

const char kOfflineAutoReloadName[] = "Offline Auto-Reload Mode";
const char kOfflineAutoReloadDescription[] =
    "Pages that fail to load while the browser is offline will be "
    "auto-reloaded when the browser is online again.";

const char kOfflineAutoReloadVisibleOnlyName[] =
    "Only Auto-Reload Visible Tabs";
const char kOfflineAutoReloadVisibleOnlyDescription[] =
    "Pages that fail to load while the browser is offline will only be "
    "auto-reloaded if their tab is visible.";

const char kOmniboxDisplayTitleForCurrentUrlName[] =
    "Include title for the current URL in the omnibox";
const char kOmniboxDisplayTitleForCurrentUrlDescription[] =
    "In the event that the omnibox provides suggestions on-focus, the URL of "
    "the current page is provided as the first suggestion without a title. "
    "Enabling this flag causes the title to be displayed.";

const char kOmniboxSpareRendererName[] =
    "Start spare renderer on omnibox focus";
const char kOmniboxSpareRendererDescription[] =
    "When the omnibox is focused, start an empty spare renderer. This can "
    "speed up the load of the navigation from the omnibox.";

const char kOmniboxUIElideSuggestionUrlAfterHostName[] =
    "Omnibox UI Elide Suggestion URL After Host";
const char kOmniboxUIElideSuggestionUrlAfterHostDescription[] =
    "Elides the path, query, and ref of suggested URLs in the Omnibox "
    "dropdown.";

const char kOmniboxUIHideSteadyStateUrlSchemeAndSubdomainsName[] =
    "Omnibox UI Hide Steady-State URL Scheme and Trivial Subdomains";
const char kOmniboxUIHideSteadyStateUrlSchemeAndSubdomainsDescription[] =
    "In the Omnibox, hide the scheme and trivial subdomains from steady state "
    "displayed URLs. Hidden portions are restored during editing. For Mac, "
    "this flag will have no effect unless MacViews is enabled.";

const char kOmniboxUIMaxAutocompleteMatchesName[] =
    "Omnibox UI Max Autocomplete Matches";

const char kOmniboxUIMaxAutocompleteMatchesDescription[] =
    "Changes the maximum number of autocomplete matches displayed in the "
    "Omnibox UI.";

const char kOmniboxUIShowSuggestionFaviconsName[] =
    "Omnibox UI Show Suggestion Favicons";
const char kOmniboxUIShowSuggestionFaviconsDescription[] =
    "Shows favicons instead of generic vector icons for URL suggestions in the "
    "Omnibox dropdown.";

const char kOmniboxUISwapTitleAndUrlName[] = "Omnibox UI Swap Title and URL";
const char kOmniboxUISwapTitleAndUrlDescription[] =
    "In the omnibox dropdown, shows titles before URLs when both are "
    "available.";

const char kOmniboxUIVerticalMarginName[] = "Omnibox UI Vertical Margin";
const char kOmniboxUIVerticalMarginDescription[] =
    "Changes the vertical margin in the Omnibox UI.";

const char kOmniboxVoiceSearchAlwaysVisibleName[] =
    "Omnibox Voice Search Always Visible";
const char kOmniboxVoiceSearchAlwaysVisibleDescription[] =
    "Always displays voice search icon in focused omnibox as long as voice "
    "search is possible";

const char kOriginTrialsName[] = "Origin Trials";
const char kOriginTrialsDescription[] =
    "Enables origin trials for controlling access to feature/API experiments.";

const char kOverflowIconsForMediaControlsName[] =
    "Icons on Media Controls Overflow Menu";
const char kOverflowIconsForMediaControlsDescription[] =
    "Displays icons on the overflow menu of the native media controls";

const char kOverlayScrollbarsName[] = "Overlay Scrollbars";
const char kOverlayScrollbarsDescription[] =
    "Enable the experimental overlay scrollbars implementation. You must also "
    "enable threaded compositing to have the scrollbars animate.";

const char kOverlayScrollbarsFlashAfterAnyScrollUpdateName[] =
    "Flash Overlay Scrollbars After Any Scroll Update";
const char kOverlayScrollbarsFlashAfterAnyScrollUpdateDescription[] =
    "Flash Overlay Scrollbars After any scroll update happends in page. You"
    " must also enable Overlay Scrollbars.";

const char kOverlayScrollbarsFlashWhenMouseEnterName[] =
    "Flash Overlay Scrollbars When Mouse Enter";
const char kOverlayScrollbarsFlashWhenMouseEnterDescription[] =
    "Flash Overlay Scrollbars When Mouse Enter a scrollable area. You must also"
    " enable Overlay Scrollbars.";

const char kUseNewAcceptLanguageHeaderName[] = "Use new Accept-Language header";
const char kUseNewAcceptLanguageHeaderDescription[] =
    "Adds the base language code after other corresponding language+region "
    "codes. This ensures that users receive content in their preferred "
    "language.";

const char kOverscrollHistoryNavigationName[] = "Overscroll history navigation";
const char kOverscrollHistoryNavigationDescription[] =
    "History navigation in response to horizontal overscroll.";
const char kOverscrollHistoryNavigationSimpleUi[] = "Simple";
const char kOverscrollHistoryNavigationParallaxUi[] = "Parallax";

const char kOverscrollStartThresholdName[] = "Overscroll start threshold";
const char kOverscrollStartThresholdDescription[] =
    "Changes overscroll start threshold relative to the default value.";
const char kOverscrollStartThreshold133Percent[] = "133%";
const char kOverscrollStartThreshold166Percent[] = "166%";
const char kOverscrollStartThreshold200Percent[] = "200%";

const char kTouchpadOverscrollHistoryNavigationName[] =
    "Overscroll history navigation on Touchpad";
const char kTouchpadOverscrollHistoryNavigationDescription[] =
    "Allows swipe left/right from touchpad change browser navigation.";

const char kParallelDownloadingName[] = "Parallel downloading";
const char kParallelDownloadingDescription[] =
    "Enable parallel downloading to accelerate download speed.";

const char kPassiveEventListenerDefaultName[] =
    "Passive Event Listener Override";
const char kPassiveEventListenerDefaultDescription[] =
    "Forces touchstart, touchmove, mousewheel and wheel event listeners (which "
    "haven't requested otherwise) to be treated as passive. This will break "
    "touch/wheel behavior on some websites but is useful for demonstrating the "
    "potential performance benefits of adopting passive event listeners.";
const char kPassiveEventListenerTrue[] = "True (when unspecified)";
const char kPassiveEventListenerForceAllTrue[] = "Force All True";

const char kPassiveEventListenersDueToFlingName[] =
    "Touch Event Listeners Passive Default During Fling";
const char kPassiveEventListenersDueToFlingDescription[] =
    "Forces touchstart, and first touchmove per scroll event listeners during "
    "fling to be treated as passive.";

const char kPassiveDocumentEventListenersName[] =
    "Document Level Event Listeners Passive Default";
const char kPassiveDocumentEventListenersDescription[] =
    "Forces touchstart, and touchmove event listeners on document level "
    "targets (which haven't requested otherwise) to be treated as passive.";

const char kPasswordForceSavingName[] = "Force-saving of passwords";
const char kPasswordForceSavingDescription[] =
    "Allow the user to manually enforce password saving instead of relying on "
    "password manager's heuristics.";

const char kPasswordGenerationName[] = "Password generation";
const char kPasswordGenerationDescription[] =
    "Allow the user to have Chrome generate passwords when it detects account "
    "creation pages.";

const char kPasswordExportName[] = "Password export";
const char kPasswordExportDescription[] =
    "Export functionality in password settings.";

const char kPasswordImportName[] = "Password import";
const char kPasswordImportDescription[] =
    "Import functionality in password settings.";

const char kPasswordSearchMobileName[] = "Password search";
const char kPasswordSearchMobileDescription[] =
    "Search functionality in password settings.";

const char kPasswordsKeyboardAccessoryName[] =
    "Add password-related functions to keyboard accessory";
const char kPasswordsKeyboardAccessoryDescription[] =
    "Adds password generation button and toggle for the passwords bottom sheet "
    "to the keyboard accessory. Replaces password generation popups.";

const char kPdfIsolationName[] = "PDF Isolation";
const char kPdfIsolationDescription[] =
    "Render PDF files from different origins in different plugin processes.";

const char kPinchScaleName[] = "Pinch scale";
const char kPinchScaleDescription[] =
    "Enables experimental support for scale using pinch.";

const char kPreviewsAllowedName[] = "Previews Allowed";
const char kPreviewsAllowedDescription[] =
    "Allows previews to be shown subject to specific preview types being "
    "enabled and the client experiencing specific triggering conditions. "
    "May be used as a kill-switch to turn off all potential preview types.";

const char kPrintPdfAsImageName[] = "Print Pdf as Image";
const char kPrintPdfAsImageDescription[] =
    "If enabled, an option to print PDF files as images will be available in "
    "print preview.";

const char kPrintPreviewRegisterPromosName[] =
    "Print Preview Registration Promos";
const char kPrintPreviewRegisterPromosDescription[] =
    "Enable registering unregistered cloud printers from print preview.";

const char kProtectSyncCredentialName[] = "Autofill sync credential";
const char kProtectSyncCredentialDescription[] =
    "How the password manager handles autofill for the sync credential.";

const char kProtectSyncCredentialOnReauthName[] =
    "Autofill sync credential only for transactional reauth pages";
const char kProtectSyncCredentialOnReauthDescription[] =
    "How the password manager handles autofill for the sync credential only "
    "for transactional reauth pages.";

const char kPullToRefreshName[] = "Pull-to-refresh gesture";
const char kPullToRefreshDescription[] =
    "Pull-to-refresh gesture in response to vertical overscroll.";
const char kPullToRefreshEnabledTouchscreen[] = "Enabled for touchscreen only";

const char kQueryInOmniboxName[] = "Query in Omnibox";
const char kQueryInOmniboxDescription[] =
    "Only display query terms in the omnibox when viewing a search results "
    "page.";

const char kQuicName[] = "Experimental QUIC protocol";
const char kQuicDescription[] = "Enable experimental QUIC protocol support.";

const char kRecurrentInterstitialName[] =
    "Show a message when the same SSL error recurs";
const char kRecurrentInterstitialDescription[] =
    "Enable a special message on the SSL certificate error page when the same "
    "SSL error occurs repeatedly.";

const char kReducedReferrerGranularityName[] =
    "Reduce default 'referer' header granularity.";
const char kReducedReferrerGranularityDescription[] =
    "If a page hasn't set an explicit referrer policy, setting this flag will "
    "reduce the amount of information in the 'referer' header for cross-origin "
    "requests.";

extern const char kRegionalLocalesAsDisplayUIName[] =
    "Allow regional locales as display UI";
extern const char kRegionalLocalesAsDisplayUIDescription[] =
    "This flag allows regional locales to be selected as display UI by the "
    "user in Language Settings. The actual locale of the system is derived "
    "from the user selection based on some simple fallback logic.";

const char kRemoveNavigationHistoryName[] =
    "Remove navigation entry on history deletion";
const char kRemoveNavigationHistoryDescription[] =
    "Remove a navigation entry when the corresponding history entry has been "
    "deleted.";

const char kRemoveUsageOfDeprecatedGaiaSigninEndpointName[] =
    "Remove usage of the deprecated GAIA sign-in endpoint";
const char kRemoveUsageOfDeprecatedGaiaSigninEndpointDescription[] =
    "The Gaia sign-in endpoint used for full-tab sign-in page is deprecated. "
    "This flags controls wheter it should no longer be used during a sign-in "
    " flow.";

const char kRendererSideResourceSchedulerName[] =
    "Renderer side ResourceScheduler";
const char kRendererSideResourceSchedulerDescription[] =
    "Migrate some ResourceScheduler functionalities to renderer";

const char kRequestTabletSiteName[] =
    "Request tablet site option in the settings menu";
const char kRequestTabletSiteDescription[] =
    "Allows the user to request tablet site. Web content is often optimized "
    "for tablet devices. When this option is selected the user agent string is "
    "changed to indicate a tablet device. Web content optimized for tablets is "
    "received there after for the current tab.";

const char kResetAppListInstallStateName[] =
    "Reset the App Launcher install state on every restart.";
const char kResetAppListInstallStateDescription[] =
    "Reset the App Launcher install state on every restart. While this flag is "
    "set, Chrome will forget the launcher has been installed each time it "
    "starts. This is used for testing the App Launcher install flow.";

const char kResourceLoadSchedulerName[] = "Use the resource load scheduler";
const char kResourceLoadSchedulerDescription[] =
    "Uses the resource load scheduler in blink to schedule and throttle "
    "resource load requests.";

const char kSafeSearchUrlReportingName[] = "SafeSearch URLs reporting.";
const char kSafeSearchUrlReportingDescription[] =
    "If enabled, inappropriate URLs can be reported back to SafeSearch.";

const char kSamplingHeapProfilerName[] = "Native memory sampling profiler.";
const char kSamplingHeapProfilerDescription[] =
    "Enables native memory sampling profiler with specified rate in KiB. "
    "If sampling rate is not provided the default value of 128 KiB is used.";

const char kSaveasMenuLabelExperimentName[] =
    "Switch 'Save as' menu labels to 'Download'";
const char kSaveasMenuLabelExperimentDescription[] =
    "Enables an experiment to switch menu labels that use 'Save as...' to "
    "'Download'.";

const char kSavePageAsMhtmlName[] = "Save Page as MHTML";
const char kSavePageAsMhtmlDescription[] =
    "Enables saving pages as MHTML: a single text file containing HTML and all "
    "sub-resources.";

const char kSavePreviousDocumentResourcesName[] =
    "Save Previous Document Resources";
const char kSavePreviousDocumentResourcesDescription[] =
    "Saves an old document's cached resources until the specified point in the "
    "next document's lifecycle.";
const char kSavePreviousDocumentResourcesNever[] =
    "Don't explicitly save resources";
const char kSavePreviousDocumentResourcesUntilOnDOMContentLoaded[] =
    "Save resources until onDOMContentLoaded completes";
const char kSavePreviousDocumentResourcesUntilOnLoad[] =
    "Save resources until onload completes";

const char kScrollPredictionName[] = "Scroll prediction";
const char kScrollPredictionDescription[] =
    "Predicts the finger's future position during scrolls allowing time to "
    "render the frame before the finger is there.";

const char kSecondaryUiMd[] =
    "Material Design in the rest of the browser's native UI";
const char kSecondaryUiMdDescription[] =
    "Extends the --top-chrome-md setting to secondary UI (bubbles, dialogs, "
    "etc.). On Mac, this enables MacViews, which uses toolkit-views for native "
    "browser dialogs.";

const char kServiceWorkerNavigationPreloadName[] =
    "Service worker navigation preload.";
const char kServiceWorkerNavigationPreloadDescription[] =
    "Enable web pages to use the experimental service worker navigation "
    "preload API.";

const char kServiceWorkerPaymentAppsName[] = "Service Worker payment apps";
const char kServiceWorkerPaymentAppsDescription[] =
    "Enable Service Worker applications to integrate as payment apps";

extern const char kServiceWorkerServicificationName[] =
    "Servicified service workers";
extern const char kServiceWorkerServicificationDescription[] =
    "Enable the servicified service workers. A servicified service worker can "
    "have direct connection from its clients, so that fetch events can be "
    "dispatched through the connection without hopping to the browser process.";

const char kServiceWorkerScriptFullCodeCacheName[] =
    "Service worker script full code cache.";
const char kServiceWorkerScriptFullCodeCacheDescription[] =
    "Generate V8 full code cache of Service Worker scripts while installing.";

const char kSettingsWindowName[] = "Show settings in a window";
const char kSettingsWindowDescription[] =
    "Settings will be shown in a dedicated window instead of as a browser tab.";

const char kShelfHoverPreviewsName[] =
    "Show previews of running apps when hovering over the shelf.";
const char kShelfHoverPreviewsDescription[] =
    "Shows previews of the open windows for a given running app when hovering "
    "over the shelf.";

const char kShowAllDialogsWithViewsToolkitName[] =
    "Show all dialogs with Views toolkit";
const char kShowAllDialogsWithViewsToolkitDescription[] =
    "All browser dialogs will be shown using the Views toolkit rather than "
    "Cocoa. This requires <a href=\"#secondary-ui-md\">#secondary-ui-md</a>.";

const char kShowAndroidFilesInFilesAppName[] =
    "Show Android files in Files app";
const char kShowAndroidFilesInFilesAppDescription[] =
    "Show Android files in Files app if Android is enabled on the device.";

const char kShowAutofillSignaturesName[] = "Show autofill signatures.";
const char kShowAutofillSignaturesDescription[] =
    "Annotates web forms with Autofill signatures as HTML attributes. Also "
    "marks password fields suitable for password generation.";

const char kShowAutofillTypePredictionsName[] = "Show Autofill predictions";
const char kShowAutofillTypePredictionsDescription[] =
    "Annotates web forms with Autofill field type predictions as placeholder "
    "text.";

const char kShowOverdrawFeedbackName[] = "Show overdraw feedback";
const char kShowOverdrawFeedbackDescription[] =
    "Visualize overdraw by color-coding elements based on if they have other "
    "elements drawn underneath.";

const char kSupervisedUserCommittedInterstitialsName[] =
    "Enable Supervised User Committed Interstitials";
const char kSupervisedUserCommittedInterstitialsDescription[] =
    "Use committed error pages instead of transient navigation entries for "
    "supervised user interstitials";

const char kEnableDrawOcclusionName[] = "Enable draw occlusion";
const char kEnableDrawOcclusionDescription[] =
    "Enable the system to use draw occlusion to skip draw quads when they are "
    "not shown on the screen.";

const char kShowSavedCopyName[] = "Show Saved Copy Button";
const char kShowSavedCopyDescription[] =
    "When a page fails to load, if a stale copy of the page exists in the "
    "browser cache, a button will be presented to allow the user to load that "
    "stale copy. The primary enabling choice puts the button in the most "
    "salient position on the error page; the secondary enabling choice puts it "
    "secondary to the reload button.";
const char kEnableShowSavedCopyPrimary[] = "Enable: Primary";
const char kEnableShowSavedCopySecondary[] = "Enable: Secondary";
const char kDisableShowSavedCopy[] = "Disable";

const char kSilentDebuggerExtensionApiName[] = "Silent Debugging";
const char kSilentDebuggerExtensionApiDescription[] =
    "Do not show the infobar when an extension attaches to a page via "
    "chrome.debugger API. This is required to debug extension background "
    "pages.";

const char kSignedHTTPExchangeName[] = "Signed HTTP Exchange";
const char kSignedHTTPExchangeDescription[] =
    "Enables Origin-Signed HTTP Exchanges support which is still in "
    "development. Warning: Enabling this may pose a security risk.";

const char kSimpleCacheBackendName[] = "Simple Cache for HTTP";
const char kSimpleCacheBackendDescription[] =
    "The Simple Cache for HTTP is a new cache. It relies on the filesystem for "
    "disk space allocation.";

const char kSimplifyHttpsIndicatorName[] = "Simplify HTTPS indicator UI";
const char kSimplifyHttpsIndicatorDescription[] =
    "Change the UI treatment for HTTPS pages.";

const char kSingleClickAutofillName[] = "Single-click autofill";
const char kSingleClickAutofillDescription[] =
    "Make autofill suggestions on initial mouse click on a form element.";

const char kStrictSiteIsolationName[] = "Strict site isolation";
const char kStrictSiteIsolationDescription[] =
    "Security mode that enables site isolation for all sites. When enabled, "
    "each renderer process will contain pages from at most one site, using "
    "out-of-process iframes when needed. When enabled, this flag forces the "
    "strictest site isolation mode (SitePerProcess). When disabled, the site "
    "isolation mode will be determined by enterprise policy or field trial.";

const char kSiteIsolationTrialOptOutName[] = "Site isolation trial opt-out";
const char kSiteIsolationTrialOptOutDescription[] =
    "Opts out of field trials that enable site isolation modes "
    "(SitePerProcess, IsolateOrigins, etc). Intended for diagnosing bugs that "
    "may be due to out-of-process iframes. Opt-out has no effect if site "
    "isolation is force-enabled via #enable-site-per-process or enterprise "
    "policy. Caution: this disables important mitigations for the Spectre CPU "
    "vulnerability affecting most computers.";
extern const char kSiteIsolationTrialOptOutChoiceDefault[] = "Default";
extern const char kSiteIsolationTrialOptOutChoiceOptOut[] =
    "Opt-out (not recommended)";

const char kSiteSettings[] = "Site settings";
const char kSiteSettingsDescription[] =
    "Add the All Sites list to Site Settings.";

const char kSmoothScrollingName[] = "Smooth Scrolling";
const char kSmoothScrollingDescription[] =
    "Animate smoothly when scrolling page content.";

const char kSoftwareRasterizerName[] = "3D software rasterizer";
const char kSoftwareRasterizerDescription[] =
    "Fall back to a 3D software rasterizer when the GPU cannot be used.";

const char kSoleIntegrationName[] = "Sole integration";
const char kSoleIntegrationDescription[] =
    "Enable Sole integration for browser customization. You must restart "
    "the browser twice for changes to take effect.";

const char kSoundContentSettingName[] = "Sound content setting";
const char kSoundContentSettingDescription[] =
    "Enable site-wide muting in content settings and tab strip context menu.";

const char kSpeculativePreconnectName[] = "Enable new preconnect predictor";
const char kSpeculativePreconnectDescription[] =
    "Enable the new implementation of preconnect and DNS preresolve. "
    "\"Learning\" means that only database construction is enabled, "
    "\"Preconnect\" enables both learning and preconnect and disables the "
    "existing implementation. \"No preconnect\" disables both implementations.";

const char kSpeculativePrefetchName[] = "Speculative Prefetch";
const char kSpeculativePrefetchDescription[] =
    R"*("Speculative Prefetch" fetches likely resources early to improve )*"
    R"*(load times, based on a local database (see chrome://predictors). )*"
    R"*("Learning" means that only the database construction is enabled, )*"
    R"*("Prefetching" that learning and prefetching are enabled.)*";

const char kSpeculativeServiceWorkerStartOnQueryInputName[] =
    "Enable speculative start of a service worker when a search is predicted.";
const char kSpeculativeServiceWorkerStartOnQueryInputDescription[] =
    "If enabled, when the user enters text in the omnibox that looks like a "
    "a query, any service worker associated with the search engine the query "
    "will be sent to is started early.";

const char kSpellingFeedbackFieldTrialName[] = "Spelling Feedback Field Trial";
const char kSpellingFeedbackFieldTrialDescription[] =
    "Enable the field trial for sending user feedback to spelling service.";

const char kStopInBackgroundName[] = "Stop in background";
const char kStopInBackgroundDescription[] =
    "Stop scheduler task queues, in the background, "
    " after a grace period.";

const char kStopLoadingInBackgroundName[] = "Stop loading in background";
const char kStopLoadingInBackgroundDescription[] =
    "Stop loading tasks and loading "
    "resources, in the background, after certain grace time.";

const char kStopNonTimersInBackgroundName[] =
    "Stop non-timer task queues background";
const char kStopNonTimersInBackgroundDescription[] =
    "Stop non-timer task queues, in the background, "
    "after a grace period.";

const char kSuggestionsWithSubStringMatchName[] =
    "Substring matching for Autofill suggestions";
const char kSuggestionsWithSubStringMatchDescription[] =
    "Match Autofill suggestions based on substrings (token prefixes) rather "
    "than just prefixes.";

const char kSyncSandboxName[] = "Use Chrome Sync sandbox";
const char kSyncSandboxDescription[] =
    "Connects to the testing server for Chrome Sync.";

const char kSystemKeyboardLockName[] = "Experimental system keyboard lock";
const char kSystemKeyboardLockDescription[] =
    "Enables websites to use the keyboard.lock() API to intercept system "
    "keyboard shortcuts and have the events routed directly to the website "
    "when in fullscreen mode.";

const char kTabAudioMutingName[] = "Tab audio muting UI control";
const char kTabAudioMutingDescription[] =
    "When enabled, the audio indicators in the tab strip double as tab audio "
    "mute controls. This also adds commands in the tab context menu for "
    "quickly muting multiple selected tabs.";

const char kTabsInCbdName[] = "Enable tabs for the Clear Browsing Data dialog.";
const char kTabsInCbdDescription[] =
    "Enables a basic and an advanced tab for the Clear Browsing Data dialog.";

const char kTabModalJsDialogName[] = "Auto-dismissing JavaScript Dialogs";
const char kTabModalJsDialogDescription[] =
    "If enabled, the JavaScript dialog will be auto dismissable when switching"
    " tab.";

const char kTcpFastOpenName[] = "TCP Fast Open";
const char kTcpFastOpenDescription[] =
    "Enable the option to send extra authentication information in the initial "
    "SYN packet for a previously connected client, allowing faster data send "
    "start.";

const char kTintGlCompositedContentName[] = "Tint GL-composited content";
const char kTintGlCompositedContentDescription[] =
    "Tint contents composited using GL with a shade of red to help debug and "
    "study overlay support.";

const char kTopChromeMd[] = "UI Layout for the browser's top chrome";
const char kTopChromeMdDescription[] =
    "Toggles between 1) Normal - for clamshell devices, 2) Hybrid (previously "
    "touch) - middle point for devices with a touch screen, 3) Touchable "
    "- new unified interface for touch and convertibles (Chrome OS), 4) "
    "Material Design refresh and 5) Touchable Material Design refresh.";
const char kTopChromeMdMaterial[] = "Normal";
const char kTopChromeMdMaterialAuto[] = "Auto";
const char kTopChromeMdMaterialHybrid[] = "Hybrid";
const char kTopChromeMdMaterialTouchOptimized[] = "Touchable";
const char kTopChromeMdMaterialRefresh[] = "Refresh";
const char kTopChromeMdMaterialRefreshTouchOptimized[] = "Touchable Refresh";

const char kThreadedScrollingName[] = "Threaded scrolling";
const char kThreadedScrollingDescription[] =
    "Threaded handling of scroll-related input events. Disabling this will "
    "force all such scroll events to be handled on the main thread. Note that "
    "this can dramatically hurt scrolling performance of most websites and is "
    "intended for testing purposes only.";

const char kTLS13VariantName[] = "TLS 1.3";
const char kTLS13VariantDescription[] = "Sets the TLS 1.3 variant used.";
const char kTLS13VariantDisabled[] = "Disabled";
const char kTLS13VariantDeprecated[] = "Disabled (Deprecated Setting)";
const char kTLS13VariantDraft23[] = "Enabled (Draft 23)";
const char kTLS13VariantDraft28[] = "Enabled (Draft 28)";
const char kTLS13VariantFinal[] = "Enabled (Final)";

const char kTopDocumentIsolationName[] = "Top document isolation";
const char kTopDocumentIsolationDescription[] =
    "Highly experimental performance mode where cross-site iframes are kept in "
    "a separate process from the top document. In this mode, iframes from "
    "different third-party sites will be allowed to share a process.";

const char kTopSitesFromSiteEngagementName[] = "Top Sites from Site Engagement";
const char kTopSitesFromSiteEngagementDescription[] =
    "Enable Top Sites on the New Tab Page to be sourced and sorted using site "
    "engagement.";

const char kTouchAdjustmentName[] = "Touch adjustment";
const char kTouchAdjustmentDescription[] =
    "Refine the position of a touch gesture in order to compensate for touches "
    "having poor resolution compared to a mouse.";

const char kTouchableAppContextMenuName[] = "Touchable App Context Menu";
const char kTouchableAppContextMenuDescription[] =
    "Enable the touchable app context menu, which enlarges app context menus "
    "in the Launcher and Shelf to make room for new features.";

const char kTouchDragDropName[] = "Touch initiated drag and drop";
const char kTouchDragDropDescription[] =
    "Touch drag and drop can be initiated through long press on a draggable "
    "element.";

const char kTouchEventsName[] = "Touch Events API";
const char kTouchEventsDescription[] =
    "Force Touch Events API feature detection to always be enabled or "
    "disabled, or to be enabled when a touchscreen is detected on startup "
    "(Automatic, the default).";

const char kTouchSelectionStrategyName[] = "Touch text selection strategy";
const char kTouchSelectionStrategyDescription[] =
    "Controls how text selection granularity changes when touch text selection "
    "handles are dragged. Non-default behavior is experimental.";
const char kTouchSelectionStrategyCharacter[] = "Character";
const char kTouchSelectionStrategyDirection[] = "Direction";

const char kTraceUploadUrlName[] = "Trace label for navigation tracing";
const char kTraceUploadUrlDescription[] =
    "This is to be used in conjunction with the enable-navigation-tracing "
    "flag. Please select the label that best describes the recorded traces. "
    "This will choose the destination the traces are uploaded to. If you are "
    "not sure, select other. If left empty, no traces will be uploaded.";
const char kTraceUploadUrlChoiceOther[] = "Other";
const char kTraceUploadUrlChoiceEmloading[] = "emloading";
const char kTraceUploadUrlChoiceQa[] = "QA";
const char kTraceUploadUrlChoiceTesting[] = "Testing";

const char kTranslateForceTriggerOnEnglishName[] =
    "Select which language model to use to trigger translate on English "
    "content";
const char kTranslateForceTriggerOnEnglishDescription[] =
    "Force the Translate Triggering on English pages experiment to be enabled "
    "with the selected language model active.";

const char kTranslateRankerEnforcementName[] =
    "Enforce TranslateRanker decisions";
const char kTranslateRankerEnforcementDescription[] =
    "Improved Translate UI triggering logic. TranslateRanker decides whether "
    "or not Translate UI should be triggered in a given context.";

const char kTreatInsecureOriginAsSecureName[] =
    "Insecure origins treated as secure";
const char kTreatInsecureOriginAsSecureDescription[] =
    "Treat given (insecure) origins as secure origins. Multiple origins can be "
    "supplied as a comma-separated list. For the definition of secure "
    "contexts, "
    "see https://w3c.github.io/webappsec-secure-contexts/";

const char kTrySupportedChannelLayoutsName[] =
    "Causes audio output streams to check if channel layouts other than the "
    "default hardware layout are available.";
const char kTrySupportedChannelLayoutsDescription[] =
    "Causes audio output streams to check if channel layouts other than the "
    "default hardware layout are available. Turning this on will allow the OS "
    "to do stereo to surround expansion if supported. May expose third party "
    "driver bugs, use with caution.";

const char kUnifiedConsentName[] = "Unified Consent";
const char kUnifiedConsentDescription[] =
    "Enables a unified management of user consent for privacy-related "
    "features. This includes new confirmation screens and improved settings "
    "pages.";

const char kUiPartialSwapName[] = "Partial swap";
const char kUiPartialSwapDescription[] = "Sets partial swap behavior.";

const char kUseDdljsonApiName[] = "Use new ddljson API for Doodles";
const char kUseDdljsonApiDescription[] =
    "Enables the new ddljson API to fetch Doodles for the NTP.";

const char kUseModernMediaControlsName[] = "New Media Controls";
const char kUseModernMediaControlsDescription[] =
    "Enables the new style native media controls.";

const char kUsePdfCompositorServiceName[] =
    "Use PDF compositor service for printing";
const char kUsePdfCompositorServiceDescription[] =
    "When enabled, use PDF compositor service to composite and generate PDF "
    "files for printing. When site isolation is enabled, disabling this will "
    "not stop using PDF compositor service since the service is required for "
    "printing out-of-process iframes correctly.";

const char kUserActivationV2Name[] = "User Activation v2";
const char kUserActivationV2Description[] =
    "Enable simple user activation for APIs that are otherwise controlled by "
    "user gesture tokens.";

const char kUserConsentForExtensionScriptsName[] =
    "User consent for extension scripts";
const char kUserConsentForExtensionScriptsDescription[] =
    "Require user consent for an extension running a script on the page, if "
    "the extension requested permission to run on all urls.";

const char kUseSuggestionsEvenIfFewFeatureName[] =
    "Disable minimum for server-side tile suggestions on NTP.";
const char kUseSuggestionsEvenIfFewFeatureDescription[] =
    "Request server-side suggestions even if there are only very few of them "
    "and use them for tiles on the New Tab Page.";

const char kV8CacheOptionsName[] = "V8 caching mode.";
const char kV8CacheOptionsDescription[] =
    "Caching mode for the V8 JavaScript engine.";
const char kV8CacheOptionsCode[] = "Cache V8 compiler data.";

const char kV8ContextSnapshotName[] = "Use a snapshot to create V8 contexts.";
const char kV8ContextSnapshotDescription[] =
    "Sets to use a snapshot to create V8 contexts in frame creation.";

const char kV8VmFutureName[] = "Future V8 VM features";
const char kV8VmFutureDescription[] =
    "This enables upcoming and experimental V8 VM features. "
    "This flag does not enable experimental JavaScript features.";

const char kVideoFullscreenOrientationLockName[] =
    "Lock screen orientation when playing a video fullscreen.";
const char kVideoFullscreenOrientationLockDescription[] =
    "Lock the screen orientation of the device to match video orientation when "
    "a video goes fullscreen. Only on phones.";

const char kVideoRotateToFullscreenName[] =
    "Rotate-to-fullscreen gesture for videos.";
const char kVideoRotateToFullscreenDescription[] =
    "Enter/exit fullscreen when device is rotated to/from the orientation of "
    "the video. Only on phones.";

const char kViewsBrowserWindowsName[] =
    "Use Views browser windows instead of Cocoa.";
const char kViewsBrowserWindowsDescription[] =
    "Use Views browser windows instead of Cocoa, aka MacViews-Browser.";

const char kWalletServiceUseSandboxName[] =
    "Use Google Payments sandbox servers";
const char kWalletServiceUseSandboxDescription[] =
    "For developers: use the sandbox service for Google Payments API calls.";

const char kWebglDraftExtensionsName[] = "WebGL Draft Extensions";
const char kWebglDraftExtensionsDescription[] =
    "Enabling this option allows web applications to access the WebGL "
    "Extensions that are still in draft status.";

const char kWebMidiName[] = "Web MIDI API";
const char kWebMidiDescription[] = "Enable Web MIDI API experimental support.";

const char kWebPaymentsName[] = "Web Payments";
const char kWebPaymentsDescription[] =
    "Enable Web Payments API integration, a JavaScript API for merchants.";

const char kWebPaymentsModifiersName[] = "Enable web payment modifiers";
const char kWebPaymentsModifiersDescription[] =
    "If the website provides modifiers in the payment request, show the custom "
    "total for each payment instrument, update the shopping cart when "
    "instruments are switched, and send modified payment method specific data "
    "to the payment app.";

const char kWebrtcEchoCanceller3Name[] = "WebRTC Echo Canceller 3.";
const char kWebrtcEchoCanceller3Description[] =
    "Experimental WebRTC echo canceller (AEC3).";

const char kWebrtcHwDecodingName[] = "WebRTC hardware video decoding";
const char kWebrtcHwDecodingDescription[] =
    "Support in WebRTC for decoding video streams using platform hardware.";

const char kWebrtcHwEncodingName[] = "WebRTC hardware video encoding";
const char kWebrtcHwEncodingDescription[] =
    "Support in WebRTC for encoding video streams using platform hardware.";

const char kWebrtcHwH264EncodingName[] = "WebRTC hardware h264 video encoding";
const char kWebrtcHwH264EncodingDescription[] =
    "Support in WebRTC for encoding h264 video streams using platform "
    "hardware.";

const char kWebrtcHwVP8EncodingName[] = "WebRTC hardware vp8 video encoding";
const char kWebrtcHwVP8EncodingDescription[] =
    "Support in WebRTC for encoding vp8 video streams using platform hardware.";

const char kWebrtcNewEncodeCpuLoadEstimatorName[] =
    "WebRTC new encode cpu load estimator";
const char kWebrtcNewEncodeCpuLoadEstimatorDescription[] =
    "Enable new estimator for the encoder cpu load, for evaluation and "
    "testing. Intended to improve accuracy when screen casting.";

const char kWebrtcSrtpAesGcmName[] =
    "Negotiation with GCM cipher suites for SRTP in WebRTC";
const char kWebrtcSrtpAesGcmDescription[] =
    "When enabled, WebRTC will try to negotiate GCM cipher suites for SRTP.";

const char kWebrtcSrtpEncryptedHeadersName[] =
    "Negotiation with encrypted header extensions for SRTP in WebRTC";
const char kWebrtcSrtpEncryptedHeadersDescription[] =
    "When enabled, WebRTC will try to negotiate encrypted header extensions "
    "for SRTP.";

const char kWebrtcStunOriginName[] = "WebRTC Stun origin header";
const char kWebrtcStunOriginDescription[] =
    "When enabled, Stun messages generated by WebRTC will contain the Origin "
    "header.";

const char kWebvrName[] = "WebVR";
const char kWebvrDescription[] =
    "Enables access to experimental Virtual Reality functionality via the "
    "WebVR 1.1 API. This feature will eventually be replaced by the WebXR "
    "Device API. Warning: Enabling this will also allow WebVR content on "
    "insecure origins to access these powerful APIs, and may pose a security "
    "risk. Controllers are exposed as Gamepads.";

const char kWebXrName[] = "WebXR Device API";
const char kWebXrDescription[] =
    "Enables access to experimental APIs to interact with Virtual Reality (VR) "
    "and Augmented Reality (AR) devices.";

const char kWebXrGamepadSupportName[] = "WebXR Gamepad Support";
const char kWebXrGamepadSupportDescription[] =
    "Expose VR controllers as Gamepads for use with the WebXR Device API. Each "
    "XRInputSource will have a corresponding Gamepad instance. Requires that "
    "WebXR Device API is also enabled.";

const char kWebXrHitTestName[] = "WebXR Hit Test";
const char kWebXrHitTestDescription[] =
    "Enables access to raycasting against estimated XR scene geometry.";

const char kWebXrOrientationSensorDeviceName[] =
    "WebXR orientation sensor device";
const char kWebXrOrientationSensorDeviceDescription[] =
    "When no VR platform device is available, expose a non-presenting device "
    "based on the device's orientation sensors, if available.";

const char kWifiCredentialSyncName[] = "WiFi credential sync";
const char kWifiCredentialSyncDescription[] =
    "Enables synchronizing WiFi network settings across devices. When enabled, "
    "the WiFi credential datatype is registered with Chrome Sync, and WiFi "
    "credentials are synchronized subject to user preferences. (See also, "
    "chrome://settings/syncSetup.)";

const char kZeroCopyName[] = "Zero-copy rasterizer";
const char kZeroCopyDescription[] =
    "Raster threads write directly to GPU memory associated with tiles.";

// Android ---------------------------------------------------------------------

#if defined(OS_ANDROID)

const char kAiaFetchingName[] = "Intermediate Certificate Fetching";
const char kAiaFetchingDescription[] =
    "Enable intermediate certificate fetching when a server does not provide "
    "sufficient certificates to build a chain to a trusted root.";

const char kAccessibilityTabSwitcherName[] = "Accessibility Tab Switcher";
const char kAccessibilityTabSwitcherDescription[] =
    "Enable the accessibility tab switcher for Android.";

const char kAllowReaderForAccessibilityName[] = "Reader Mode for Accessibility";
const char kAllowReaderForAccessibilityDescription[] =
    "Allows Reader Mode on any articles, even if the page is mobile-friendly.";

const char kAndroidAutofillAccessibilityName[] = "Autofill Accessibility";
const char kAndroidAutofillAccessibilityDescription[] =
    "Enable accessibility for autofill popup.";

const char kAndroidPaymentAppsName[] = "Android payment apps";
const char kAndroidPaymentAppsDescription[] =
    "Enable third party Android apps to integrate as payment apps";

const char kAsyncDnsName[] = "Async DNS resolver";
const char kAsyncDnsDescription[] = "Enables the built-in DNS resolver.";

const char kAutofillAccessoryViewName[] =
    "Autofill suggestions as keyboard accessory view";
const char kAutofillAccessoryViewDescription[] =
    "Shows Autofill suggestions on top of the keyboard rather than in a "
    "dropdown.";

const char kBackgroundLoaderForDownloadsName[] =
    "Enables background downloading of pages.";
const char kBackgroundLoaderForDownloadsDescription[] =
    "Enables downloading pages in the background in case page is not yet "
    "loaded in current tab.";

const char kChromeDuplexName[] = "Chrome Duplex";
const char kChromeDuplexDescription[] =
    "Enables Chrome Duplex, split toolbar Chrome Home, on Android.";

const char kChromeHomeSwipeLogicName[] = "Chrome Home Swipe Logic";
const char kChromeHomeSwipeLogicDescription[] =
    "Various swipe logic options for Chrome Home for sheet expansion.";
const char kChromeHomeSwipeLogicRestrictArea[] = "Restrict swipable area";
const char kChromeHomeSwipeLogicVelocity[] = "Velocity suppression model";

const char kChromeModernAlternateCardLayoutName[] =
    "Chrome Modern Alternate Card Layout";
const char kChromeModernAlternateCardLayoutDescription[] =
    "Enable the alternate card layout for Chrome Modern Design.";

const char kChromeModernDesignName[] = "Chrome Modern Design";
const char kChromeModernDesignDescription[] =
    "Enable modern design for Chrome. Chrome must be restarted twice for this "
    "flag to take effect.";

const char kChromeMemexName[] = "Chrome Memex";
const char kChromeMemexDescription[] =
    "Enables Chrome Memex homepage on Android. Restricted to opted-in "
    "Googlers.";

const char kClearOldBrowsingDataName[] = "Clear older browsing data";
const char kClearOldBrowsingDataDescription[] =
    "Enables clearing of browsing data which is older than a given time "
    "period.";

const char kContentSuggestionsCategoryOrderName[] =
    "Default content suggestions category order (e.g. on NTP)";
const char kContentSuggestionsCategoryOrderDescription[] =
    "Set default order of content suggestion categories (e.g. on the NTP).";

const char kContentSuggestionsCategoryRankerName[] =
    "Content suggestions category ranker (e.g. on NTP)";
const char kContentSuggestionsCategoryRankerDescription[] =
    "Set category ranker to order categories of content suggestions (e.g. on "
    "the NTP).";

const char kContentSuggestionsDebugLogName[] = "Content suggestions debug log";
const char kContentSuggestionsDebugLogDescription[] =
    "Enable content suggestions debug log accessible through "
    "snippets-internals.";

const char kContextualSearchMlTapSuppressionName[] =
    "Contextual Search ML tap suppression";
const char kContextualSearchMlTapSuppressionDescription[] =
    "Enables tap gestures to be suppressed to improve CTR by applying machine "
    "learning.  The \"Contextual Search Ranker prediction\" flag must also be "
    "enabled!";

const char kContextualSearchName[] = "Contextual Search";
const char kContextualSearchDescription[] =
    "Whether or not Contextual Search is enabled.";

const char kContextualSearchRankerQueryName[] =
    "Contextual Search Ranker prediction";
const char kContextualSearchRankerQueryDescription[] =
    "Enables prediction of tap gestures using Assist-Ranker machine learning.";

const char kContextualSearchSecondTapName[] =
    "Contextual Search second tap triggering";
const char kContextualSearchSecondTapDescription[] =
    "Enables triggering on a second tap gesture even when Ranker would "
    "normally suppress that tap.";

const char kDontPrefetchLibrariesName[] = "Don't Prefetch Libraries";
const char kDontPrefetchLibrariesDescription[] =
    "Don't prefetch libraries after loading.";

const char kDownloadsForegroundName[] = "Enable downloads foreground";
const char kDownloadsForegroundDescription[] =
    "Enable downloads as a foreground service for all versions of Android.";

const char kDownloadsLocationChangeName[] = "Enable downloads location change";
const char kDownloadsLocationChangeDescription[] =
    "Enable changing default downloads storage location on Android.";

const char kDownloadProgressInfoBarName[] = "Enable download progress infobar";
const char kDownloadProgressInfoBarDescription[] =
    "Enables an infobar notifying users about status of current downloads.";

const char kEnableAndroidPayIntegrationV1Name[] = "Enable Android Pay v1";
const char kEnableAndroidPayIntegrationV1Description[] =
    "Enable integration with Android Pay using the first version of the API";

const char kEnableAndroidPayIntegrationV2Name[] = "Enable Android Pay v2";
const char kEnableAndroidPayIntegrationV2Description[] =
    "Enable integration with Android Pay using the second version of the API";

const char kEnableAndroidSpellcheckerName[] = "Enable spell checking";
const char kEnableAndroidSpellcheckerDescription[] =
    "Enables use of the Android spellchecker.";

const char kEnableCommandLineOnNonRootedName[] =
    "Enable command line on non-rooted devices";
const char kEnableCommandLineOnNoRootedDescription[] =
    "Enable reading command line file on non-rooted devices (DANGEROUS).";

const char kEnableContentSuggestionsNewFaviconServerName[] =
    "Get favicons for content suggestions from a new server.";
const char kEnableContentSuggestionsNewFaviconServerDescription[] =
    "If enabled, the content suggestions (on the NTP) will get favicons from a "
    "new favicon server.";

const char kEnableContentSuggestionsSettingsName[] =
    "Show content suggestions settings.";
const char kEnableContentSuggestionsSettingsDescription[] =
    "If enabled, the content suggestions settings will be available from the "
    "main settings menu.";

const char kEnableContentSuggestionsThumbnailDominantColorName[] =
    "Use content suggestions thumbnail dominant color.";
const char kEnableContentSuggestionsThumbnailDominantColorDescription[] =
    "Use content suggestions thumbnail dominant color as a placeholder before "
    "the real thumbnail is fetched (requires Chrome Home).";

const char kEnableCustomContextMenuName[] = "Enable custom context menu";
const char kEnableCustomContextMenuDescription[] =
    "Enables a new context menu when a link, image, or video is pressed within "
    "Chrome.";

const char kEnableCustomFeedbackUiName[] = "Enable Custom Feedback UI";
const char kEnableCustomFeedbackUiDescription[] =
    "Enables a custom feedback UI when submitting feedback through Google "
    "Feedback. Works with Google Play Services v10.2+";

const char kEnableDataReductionProxyMainMenuName[] =
    "Enable Data Saver main menu item";
const char kEnableDataReductionProxyMainMenuDescription[] =
    "Enables the Data Saver menu item in the main menu";

const char kEnableOmniboxClipboardProviderName[] =
    "Omnibox clipboard URL suggestions";
const char kEnableOmniboxClipboardProviderDescription[] =
    "Provide a suggestion of the URL stored in the clipboard (if any) upon "
    "focus in the omnibox.";

const char kEnableExpandedAutofillCreditCardPopupLayoutName[] =
    "Use expanded autofill credit card popup layout.";
const char kEnableExpandedAutofillCreditCardPopupLayoutDescription[] =
    "If enabled, displays autofill credit card popup using expanded layout.";

const char kEnableNtpArticleSuggestionsExpandableHeaderName[] =
    "Show article suggestions expandable header on New Tab Page";
const char kEnableNtpArticleSuggestionsExpandableHeaderDescription[] =
    "If enabled, the article suggestions content on New Tab Page can be "
    "toggled shown or hidden by clicking the expandable header.";

const char kEnableNtpAssetDownloadSuggestionsName[] =
    "Show asset downloads on the New Tab page";
const char kEnableNtpAssetDownloadSuggestionsDescription[] =
    "If enabled, the list of content suggestions on the New Tab page will "
    "contain assets (e.g. books, pictures, audio) that the user downloaded for "
    "later use.";

const char kEnableNtpBookmarkSuggestionsName[] =
    "Show recently visited bookmarks on the New Tab page";
const char kEnableNtpBookmarkSuggestionsDescription[] =
    "If enabled, the list of content suggestions on the New Tab page will "
    "contain recently visited bookmarks.";

const char kEnableNtpForeignSessionsSuggestionsName[] =
    "Show recent foreign tabs on the New Tab page";
const char kEnableNtpForeignSessionsSuggestionsDescription[] =
    "If enabled, the list of content suggestions on the New Tab page will "
    "contain recent foreign tabs.";

const char kEnableNtpOfflinePageDownloadSuggestionsName[] =
    "Show offline page downloads on the New Tab page";
const char kEnableNtpOfflinePageDownloadSuggestionsDescription[] =
    "If enabled, the list of content suggestions on the New Tab page will "
    "contain pages that the user downloaded for later use.";

const char kEnableNtpRemoteSuggestionsName[] =
    "Show server-side suggestions on the New Tab page";
const char kEnableNtpRemoteSuggestionsDescription[] =
    "If enabled, the list of content suggestions on the New Tab page will "
    "contain server-side suggestions (e.g., Articles for you). Furthermore, it "
    "allows to override the source used to retrieve these server-side "
    "suggestions.";

const char kEnableNtpSnippetsVisibilityName[] =
    "Make New Tab Page Snippets more visible.";
const char kEnableNtpSnippetsVisibilityDescription[] =
    "If enabled, the NTP snippets will become more discoverable with a larger "
    "portion of the first card above the fold.";

const char kEnableNtpSuggestionsNotificationsName[] =
    "Notify about new content suggestions available at the New Tab page";
const char kEnableNtpSuggestionsNotificationsDescription[] =
    "If enabled, notifications will inform about new content suggestions on "
    "the New Tab page.";

const char kEnableOfflinePreviewsName[] = "Offline Page Previews";
const char kEnableOfflinePreviewsDescription[] =
    "Enable showing offline page previews on slow networks.";

const char kEnableOskOverscrollName[] = "Enable OSK Overscroll";
const char kEnableOskOverscrollDescription[] =
    "Enable OSK overscroll support. With this flag on, the OSK will only "
    "resize the visual viewport.";

const char kEnableSpecialLocaleName[] =
    "Enable custom logic for special locales.";
const char kEnableSpecialLocaleDescription[] =
    "Enable custom logic for special locales. In this mode, Chrome might "
    "behave differently in some locales.";

const char kEnableWebNfcName[] = "WebNFC";
const char kEnableWebNfcDescription[] = "Enable WebNFC support.";

const char kEnableWebPaymentsMethodSectionOrderV2Name[] =
    "Enable Web Payments method section order V2.";
const char kEnableWebPaymentsMethodSectionOrderV2Description[] =
    "Enable this option to display payment method section above address "
    "section instead of below it.";

const char kGrantNotificationsToDSEName[] =
    "Grant notifications to the Default Search Engine";
const char kGrantNotificationsToDSENameDescription[] =
    "Automatically grant the notifications permission to the Default Search "
    "Engine";

const char kHomePageButtonName[] = "Force Enable Home Page Button";
const char kHomePageButtonDescription[] = "Displays a home button if enabled.";

const char kInterestFeedContentSuggestionsDescription[] =
    "Use the interest feed to render content suggestions. Currently content "
    "suggestions are shown on the New Tab Page.";
const char kInterestFeedContentSuggestionsName[] =
    "Interest Feed Content Suggestions";

const char kKeepPrefetchedContentSuggestionsName[] =
    "Keep prefetched content suggestions";
const char kKeepPrefetchedContentSuggestionsDescription[] =
    "If enabled, some of prefetched content suggestions are not replaced by "
    "the new fetched suggestions.";

const char kLanguagesPreferenceName[] = "Language Settings";
const char kLanguagesPreferenceDescription[] =
    "Enable this option for Language Settings feature on Android.";

const char kLongPressBackForHistoryName[] =
    "Long Press Back Button for History";
const char kLongPressBackForHistoryDescription[] =
    "Long press system back button to show navigation history if enabled";

const char kLsdPermissionPromptName[] =
    "Location Settings Dialog Permission Prompt";
const char kLsdPermissionPromptDescription[] =
    "Whether to use the Google Play Services Location Settings Dialog "
    "permission dialog.";

const char kModalPermissionDialogViewName[] = "Modal Permission Dialog";
const char kModalPermissionDialogViewDescription[] =
    "Enable this option to use ModalDialogManager for permission Dialogs.";

const char kMediaScreenCaptureName[] = "Experimental ScreenCapture.";
const char kMediaScreenCaptureDescription[] =
    "Enable this option for experimental ScreenCapture feature on Android.";

const char kModalPermissionPromptsName[] = "Modal Permission Prompts";
const char kModalPermissionPromptsDescription[] =
    "Whether to use permission dialogs in place of permission infobars.";

const char kNewPhotoPickerName[] = "Enable new Photopicker";
const char kNewPhotoPickerDescription[] =
    "Activates the new picker for selecting photos.";

const char kNoCreditCardAbort[] = "No Credit Card Abort";
const char kNoCreditCardAbortDescription[] =
    "Whether or not the No Credit Card Abort is enabled.";

const char kNtpButtonName[] = "Enable NTP Button";
const char kNtpButtonDescription[] =
    "Displays a New Tab Page button in the toolbar if enabled.";

const char kNtpModernLayoutName[] = "Modern NTP layout";
const char kNtpModernLayoutDescription[] =
    "Show a modern layout on the New Tab Page.";

const char kNtpGoogleGInOmniboxName[] = "Google G in New Tab Page omnibox";
const char kNtpGoogleGInOmniboxDescription[] =
    "Show a Google G in the omnibox on the New Tab Page.";

const char kOfflineBookmarksName[] = "Enable offline bookmarks";
const char kOfflineBookmarksDescription[] =
    "Enable saving bookmarked pages for offline viewing.";

const char kOfflinePagesCtName[] = "Enable Offline Pages CT features.";
const char kOfflinePagesCtDescription[] = "Enable Offline Pages CT features.";

const char kOfflinePagesCtV2Name[] = "Enable Offline Pages CT V2 features.";
const char kOfflinePagesCtV2Description[] =
    "V2 features include attributing pages to the app that initiated the "
    "custom tabs, and being able to query for pages by page attribution.";

const char kOfflinePagesCTSuppressNotificationsName[] =
    "Disable download complete notification for whitelisted CCT apps.";
const char kOfflinePagesCTSuppressNotificationsDescription[] =
    "Disable download complete notification for page downloads originating "
    "from a CCT app whitelisted to show their own download complete "
    "notification.";

const char kOfflinePagesDescriptiveFailStatusName[] =
    "Enables descriptive failed download status text.";
const char kOfflinePagesDescriptiveFailStatusDescription[] =
    "Enables failed download status text in notifications and Downloads Home "
    "to state the reason the request failed if the failure is actionable.";

const char kOfflinePagesDescriptivePendingStatusName[] =
    "Enables descriptive pending download status text.";
const char kOfflinePagesDescriptivePendingStatusDescription[] =
    "Enables pending download status text in notifications and Downloads Home "
    "to state the reason the request is pending.";

const char kOfflinePagesInDownloadHomeOpenInCctName[] =
    "Enables offline pages in the downloads home to be opened in CCT.";
const char kOfflinePagesInDownloadHomeOpenInCctDescription[] =
    "When enabled offline pages launched from the Downloads Home will be "
    "opened in Chrome Custom Tabs (CCT) instead of regular tabs.";

const char kOfflinePagesLimitlessPrefetchingName[] =
    "Removes resource usage limits for the prefetching of offline pages.";
const char kOfflinePagesLimitlessPrefetchingDescription[] =
    "Allows the prefetching of suggested offline pages to ignore resource "
    "usage limits. This allows it to completely ignore data usage limitations "
    "and allows downloads to happen with any kind of connection.";

const char kOfflinePagesLoadSignalCollectingName[] =
    "Enables collecting load timing data for offline page snapshots.";
const char kOfflinePagesLoadSignalCollectingDescription[] =
    "Enables loading completeness data collection while writing an offline "
    "page.  This data is collected in the snapshotted offline page to allow "
    "data analysis to improve deciding when to make the offline snapshot.";

const char kOfflinePagesPrefetchingName[] =
    "Enables suggested offline pages to be prefetched.";
const char kOfflinePagesPrefetchingDescription[] =
    "Enables suggested offline pages to be prefetched, so useful content is "
    "available while offline.";

const char kOfflinePagesPrefetchingUIName[] =
    "Enables prefetched offline pages to be shown in UI.";
const char kOfflinePagesPrefetchingUIDescription[] =
    "Enables prefetched offline pages to raise notifications and be shown in "
    "download home UI.";

const char kOfflinePagesResourceBasedSnapshotName[] =
    "Enables offline page snapshots to be based on percentage of page loaded.";
const char kOfflinePagesResourceBasedSnapshotDescription[] =
    "Enables offline page snapshots to use a resource percentage based "
    "approach for determining when the page is loaded as opposed to a time "
    "based approach";

const char kOfflinePagesRenovationsName[] = "Enables offline page renovations.";
const char kOfflinePagesRenovationsDescription[] =
    "Enables offline page renovations which correct issues with dynamic "
    "content that occur when offlining pages that use JavaScript.";

const char kOfflinePagesSharingName[] = "Enables offline pages to be shared.";
const char kOfflinePagesSharingDescription[] =
    "Enables the saved offline pages to be shared via other applications.";

const char kOfflinePagesShowAlternateDinoPageName[] =
    "Enable alternate dino page with more user capabilities.";
const char kOfflinePagesShowAlternateDinoPageDescription[] =
    "Enables the dino page to show more buttons and offer existing offline "
    "content.";

const char kOfflinePagesSvelteConcurrentLoadingName[] =
    "Enables concurrent background loading on svelte.";
const char kOfflinePagesSvelteConcurrentLoadingDescription[] =
    "Enables concurrent background loading (or downloading) of pages on "
    "Android svelte (512MB RAM) devices. Otherwise, background loading will "
    "happen when the svelte device is idle.";

const char kOffliningRecentPagesName[] =
    "Enable offlining of recently visited pages";
const char kOffliningRecentPagesDescription[] =
    "Enable storing recently visited pages locally for offline use. Requires "
    "Offline Pages to be enabled.";

const char kPayWithGoogleV1Name[] = "Pay with Google v1";
const char kPayWithGoogleV1Description[] =
    "Enable Pay with Google integration into Web Payments with API version "
    "'1'.";

const char kProgressBarThrottleName[] = "Android progress update throttling.";
const char kProgressBarThrottleDescription[] =
    "Limit the maximum progress update to make progress appear smoother.";

const char kPullToRefreshEffectName[] = "The pull-to-refresh effect";
const char kPullToRefreshEffectDescription[] =
    "Page reloads triggered by vertically overscrolling content.";

const char kPwaImprovedSplashScreenName[] =
    "Improved Splash Screen for standalone PWAs";
const char kPwaImprovedSplashScreenDescription[] =
    "Enables the Improved Splash Screen UX for standalone PWAs based on new "
    "Web App Manifest attributes";
const char kPwaPersistentNotificationName[] =
    "Persistent notification in standalone PWA";
const char kPwaPersistentNotificationDescription[] =
    "Enables a persistent Android notification for standalone PWAs";

const char kReaderModeHeuristicsName[] = "Reader Mode triggering";
const char kReaderModeHeuristicsDescription[] =
    "Determines what pages the Reader Mode infobar is shown on.";
const char kReaderModeHeuristicsMarkup[] = "With article structured markup";
const char kReaderModeHeuristicsAdaboost[] = "Non-mobile-friendly articles";
const char kReaderModeHeuristicsAllArticles[] = "All articles";
const char kReaderModeHeuristicsAlwaysOff[] = "Never";
const char kReaderModeHeuristicsAlwaysOn[] = "Always";

const char kReaderModeInCCTName[] = "Reader Mode in CCT";
const char kReaderModeInCCTDescription[] =
    "Open Reader Mode in Chrome Custom Tabs.";

const char kSetMarketUrlForTestingName[] = "Set market URL for testing";
const char kSetMarketUrlForTestingDescription[] =
    "When enabled, sets the market URL for use in testing the update menu "
    "item.";

const char kSpannableInlineAutocompleteName[] = "Spannable inline autocomplete";
const char kSpannableInlineAutocompleteDescription[] =
    "A new type of inline autocomplete for the omnibox that works with "
    "keyboards that compose text.";

const char kSimplifiedNtpName[] = "Simplified NTP";
const char kSimplifiedNtpDescription[] = "Show a simplified New Tab Page.";

const char kSiteExplorationUiName[] = "Site Exploration UI";
const char kSiteExplorationUiDescription[] =
    "Show site suggestions in the Exploration UI";

const char kUseClientCertName[] = "Use Client Certificates";
const char kUseClientCertDescription[] =
    "When enabled, the system's client certificate selection dialog will be "
	"shown when a https request requires client authentication";

const char kUpdateMenuBadgeName[] = "Force show update menu badge";
const char kUpdateMenuBadgeDescription[] =
    "When enabled, an update badge will be shown on the app menu button.";

const char kUpdateMenuItemCustomSummaryDescription[] =
    "When this flag and the force show update menu item flag are enabled, a "
    "custom summary string will be displayed below the update menu item.";
const char kUpdateMenuItemCustomSummaryName[] =
    "Update menu item custom summary";

const char kUpdateMenuItemName[] = "Force show update menu item";
const char kUpdateMenuItemDescription[] =
    R"*(When enabled, an "Update Chrome" item will be shown in the app )*"
    R"*(menu.)*";

const char kVrBrowsingNativeAndroidUiName[] = "VR browsing native android ui";
const char kVrBrowsingNativeAndroidUiDescription[] =
    "Enable Android UI elements in VR.";

const char kVrBrowsingTabsViewName[] = "VR browsing tabs view";
const char kVrBrowsingTabsViewDescription[] =
    "Enable tab overview (tab switcher) in VR.";

const char kThirdPartyDoodlesName[] =
    "Enable Doodles for third-party search engines";
const char kThirdPartyDoodlesDescription[] =
    "Enables fetching and displaying Doodles on the NTP for third-party search "
    "engines.";

const char kWebXrRenderPathName[] = "WebXR presentation render path";
const char kWebXrRenderPathDescription[] =
    "Render path to use for WebXR presentation (including WebVR)";
const char kWebXrRenderPathChoiceClientWaitDescription[] =
    "ClientWait (Baseline)";
const char kWebXrRenderPathChoiceGpuFenceDescription[] =
    "GpuFence (Android N+)";
const char kWebXrRenderPathChoiceSharedBufferDescription[] =
    "SharedBuffer (Android O+)";

const char kOneGoogleBarOnLocalNtpName[] =
    "Enable the OneGoogleBar on the local NTP";
const char kOneGoogleBarOnLocalNtpDescription[] =
    "Show a OneGoogleBar on the local New Tab page if Google is the default "
    "search engine.";

const char kUseGoogleLocalNtpName[] = "Enable using the Google local NTP";
const char kUseGoogleLocalNtpDescription[] =
    "Use the local New Tab page if Google is the default search engine.";

const char kDoodlesOnLocalNtpName[] = "Enable doodles on the local NTP";
const char kDoodlesOnLocalNtpDescription[] =
    "Show doodles on the local New Tab page if Google is the default search "
    "engine.";

const char kVoiceSearchOnLocalNtpName[] =
    "Enable Voice Search on the local NTP";
const char kVoiceSearchOnLocalNtpDescription[] =
    "Show a microphone for voice search on the local New Tab page "
    "if Google is the default search engine.";

// Non-Android -----------------------------------------------------------------

const char kAccountConsistencyName[] =
    "Identity consistency between browser and cookie jar";
const char kAccountConsistencyDescription[] =
    "When enabled, the browser manages signing in and out of Google accounts.";
const char kAccountConsistencyChoiceMirror[] = "Mirror";
const char kAccountConsistencyChoiceDice[] = "Dice";

const char kEnableAudioFocusName[] = "Manage audio focus across tabs";
const char kEnableAudioFocusDescription[] =
    "Manage audio focus across tabs to improve the audio mixing.";
const char kEnableAudioFocusDisabled[] = "Disabled";
const char kEnableAudioFocusEnabled[] = "Enabled";
const char kEnableAudioFocusEnabledDuckFlash[] =
    "Enabled (Flash lowers volume when interrupted by other sound, "
    "experimental)";

const char kEnableNewAppMenuIconName[] = "Enable the New App Menu Icon";
const char kEnableNewAppMenuIconDescription[] =
    "Use the new app menu icon with update notification animations.";

const char kOmniboxRichEntitySuggestionsName[] =
    "Omnibox rich entity suggestions";
const char kOmniboxRichEntitySuggestionsDescription[] =
    "Display entity suggestions using images and an enhanced layout; showing "
    "more context and descriptive text about the entity";

const char kOmniboxNewAnswerLayoutName[] = "Omnibox new answer layout";
const char kOmniboxNewAnswerLayoutDescription[] =
    "Display answers using an enhanced layout with larger images";

const char kOmniboxTabSwitchSuggestionsName[] =
    "Omnibox tab switch suggestions";
const char kOmniboxTabSwitchSuggestionsDescription[] =
    "Enable suggestions for switching to open tabs within the Omnibox.";

const char kOmniboxTailSuggestionsName[] = "Omnibox tail suggestions";
const char kOmniboxTailSuggestionsDescription[] =
    "Enable receiving tail suggestions, a type of search suggestion based on "
    "the last few words in the query, for the Omnibox.";

const char kEnableWebAuthenticationAPIName[] = "Web Authentication API";
const char kEnableWebAuthenticationAPIDescription[] =
    "Enable Web Authentication API support";

const char kEnableWebAuthenticationTestingAPIName[] =
    "Web Authentication Testing API";
const char kEnableWebAuthenticationTestingAPIDescription[] =
    "Enable Web Authentication Testing API support, which disconnects the API "
    "implementation from the real world, and allows configuring virtual "
    "authenticator devices for testing";

#if defined(GOOGLE_CHROME_BUILD)

const char kGoogleBrandedContextMenuName[] =
    "Google branding in the context menu";
const char kGoogleBrandedContextMenuDescription[] =
    "Shows a Google icon next to context menu items powered by Google "
    "services.";

#endif  // !defined(GOOGLE_CHROME_BUILD)

#endif  // !defined(OS_ANDROID)

// Windows ---------------------------------------------------------------------

#if defined(OS_WIN)

const char kCloudPrintXpsName[] = "XPS in Google Cloud Print";
const char kCloudPrintXpsDescription[] =
    "XPS enables advanced options for classic printers connected to the Cloud "
    "Print with Chrome. Printers must be re-connected after changing this "
    "flag.";

const char kDirectManipulationStylusName[] = "Direct Manipulation Stylus";
const char kDirectManipulationStylusDescription[] =
    "If enabled, Chrome will scroll web pages on stylus drag.";

const char kDisablePostscriptPrinting[] = "Disable PostScript Printing";
const char kDisablePostscriptPrintingDescription[] =
    "Disables PostScript generation when printing to PostScript capable "
    "printers, and uses EMF generation in its place.";

const char kEnableAppcontainerName[] = "Enable AppContainer Lockdown.";
const char kEnableAppcontainerDescription[] =
    "Enables the use of an AppContainer on sandboxed processes to improve "
    "security.";

const char kEnableD3DVsync[] = "D3D v-sync";
const char kEnableD3DVsyncDescription[] =
    "Produces v-sync signal by having D3D wait for vertical blanking interval "
    "to occur.";

const char kEnableDesktopIosPromotionsName[] = "Desktop to iOS promotions.";
const char kEnableDesktopIosPromotionsDescription[] =
    "Enable Desktop to iOS promotions, and allow users to see them if they are "
    "eligible.";

const char kEnableGpuAppcontainerName[] = "Enable GPU AppContainer Lockdown.";
const char kEnableGpuAppcontainerDescription[] =
    "Enables the use of an AppContainer for the GPU sandboxed processes to "
    "improve security.";

const char kGdiTextPrinting[] = "GDI Text Printing";
const char kGdiTextPrintingDescription[] =
    "Use GDI to print text as simply text";

const char kIncreaseInputAudioBufferSize[] = "Increase input audio buffer size";
const char kIncreaseInputAudioBufferSizeDescription[] =
    "Increases the input audio endpoint buffer to 100 ms.";

const char kTraceExportEventsToEtwName[] =
    "Enable exporting of tracing events to ETW.";
const char kTraceExportEventsToEtwDesription[] =
    "If enabled, trace events will be exported to the Event Tracing for "
    "Windows (ETW) and can then be captured by tools such as UIForETW or "
    "Xperf.";

const char kUseWinrtMidiApiName[] = "Use Windows Runtime MIDI API";
const char kUseWinrtMidiApiDescription[] =
    "Use Windows Runtime MIDI API for WebMIDI (effective only on Windows 10 or "
    "later).";

const char kWindows10CustomTitlebarName[] = "Custom-drawn Windows 10 Titlebar";
const char kWindows10CustomTitlebarDescription[] =
    "If enabled, Chrome will draw the titlebar and caption buttons instead of "
    "deferring to Windows.";

#endif  // defined(OS_WIN)

// Mac -------------------------------------------------------------------------

#if defined(OS_MACOSX)

const char kAppWindowCyclingName[] = "Custom Window Cycling for Chrome Apps.";
const char kAppWindowCyclingDescription[] =
    "Changes the behavior of Cmd+` when a Chrome App becomes active. When "
    "enabled, Chrome Apps will not be cycled when Cmd+` is pressed from a "
    "browser window, and browser windows will not be cycled when a Chrome App "
    "is active.";

const char kFullscreenToolbarRevealName[] =
    "Enables the toolbar in fullscreen to reveal itself.";
const char kFullscreenToolbarRevealDescription[] =
    "Reveal the toolbar in fullscreen for a short period when the tab strip "
    "has changed.";

const char kContentFullscreenName[] = "Improved Content Fullscreen";
const char kContentFullscreenDescription[] =
    "Fullscreen content window detaches from main browser window and goes to "
    "a new space without moving or changing the original browser window.";

const char kDialogTouchBarName[] = "Dialog Touch Bar";
const char kDialogTouchBarDescription[] =
    "Shows Dialog buttons on the Touch Bar.";

const char kHostedAppsInWindowsName[] =
    "Allow hosted apps to be opened in windows";
const char kHostedAppsInWindowsDescription[] =
    "Allows hosted apps to be opened in windows instead of being limited to "
    "tabs.";

const char kMacRTLName[] = "Enable RTL";
const char kMacRTLDescription[] = "Mirrors the UI for RTL language users";

const char kMacSystemShareMenuName[] = "Enable System Share Menu";
const char kMacSystemShareMenuDescription[] =
    "Enables sharing via macOS share extensions.";

const char kMacTouchBarName[] = "Hardware Touch Bar";
const char kMacTouchBarDescription[] = "Control the use of the Touch Bar.";

const char kMacV2SandboxName[] = "Mac V2 Sandbox";
const char kMacV2SandboxDescription[] =
    "Eliminates the unsandboxed warmup phase and sandboxes processes for their "
    "entire life cycle.";

const char kMacViewsNativeAppWindowsName[] = "Toolkit-Views App Windows.";
const char kMacViewsNativeAppWindowsDescription[] =
    "Controls whether to use Toolkit-Views based Chrome App windows.";

const char kMacViewsTaskManagerName[] = "Toolkit-Views Task Manager.";
const char kMacViewsTaskManagerDescription[] =
    "Controls whether to use the Toolkit-Views based Task Manager.";

const char kTabDetachingInFullscreenName[] =
    "Allow tab detaching in fullscreen";
const char kTabDetachingInFullscreenDescription[] =
    "Allow tabs to detach from the tabstrip when in fullscreen mode on Mac.";

const char kTabStripKeyboardFocusName[] = "Tab Strip Keyboard Focus";
const char kTabStripKeyboardFocusDescription[] =
    "Enable keyboard focus for the tabs in the tab strip.";

#endif

// Chrome OS -------------------------------------------------------------------

#if defined(OS_CHROMEOS)

const char kAcceleratedMjpegDecodeName[] =
    "Hardware-accelerated mjpeg decode for captured frame";
const char kAcceleratedMjpegDecodeDescription[] =
    "Enable hardware-accelerated mjpeg decode for captured frame where "
    "available.";

const char kAllowTouchpadThreeFingerClickName[] = "Touchpad three-finger-click";
const char kAllowTouchpadThreeFingerClickDescription[] =
    "Enables touchpad three-finger-click as middle button.";

const char kArcBootCompleted[] = "Load Android apps automatically";
const char kArcBootCompletedDescription[] =
    "Allow Android apps to start automatically after signing in.";

const char kArcNativeBridgeExperimentName[] =
    "Enable native bridge experiment for ARC";
const char kArcNativeBridgeExperimentDescription[] =
    "Enables experimental native bridge feature.";

const char kArcUsbHostName[] = "Enable ARC USB host integration";
const char kArcUsbHostDescription[] =
    "Allow Android apps to use USB host feature on ChromeOS devices.";

const char kArcVpnName[] = "Enable ARC VPN integration";
const char kArcVpnDescription[] =
    "Allow Android VPN clients to tunnel Chrome traffic.";

const char kAshDisableLoginDimAndBlurName[] =
    "Disable dimming and blur on login screen.";
const char kAshDisableLoginDimAndBlurDescription[] =
    "Disable dimming and blur on login screen.";

const char kAshDisableSmoothScreenRotationName[] =
    "Disable smooth rotation animations.";
const char kAshDisableSmoothScreenRotationDescription[] =
    "Disable smooth rotation animations.";

const char kAshEnableDisplayMoveWindowAccelsName[] =
    "Enable shortcuts for moving window between displays.";
const char kAshEnableDisplayMoveWindowAccelsDescription[] =
    "Enable shortcuts for moving window between displays.";

const char kAshEnableKeyboardShortcutViewerName[] =
    "Enable keyboard shortcut viewer.";
const char kAshEnableKeyboardShortcutViewerDescription[] =
    "Enable keyboard shortcut viewer.";

const char kAshEnableMirroredScreenName[] = "Enable mirrored screen mode.";
const char kAshEnableMirroredScreenDescription[] =
    "Enable the mirrored screen mode. This mode flips the screen image "
    "horizontally.";

const char kAshEnableNewOverviewAnimationsName[] =
    "Enable new overview animations.";
const char kAshEnableNewOverviewAnimationsDescription[] =
    "Enables the new overview mode animations.";

const char kAshEnablePersistentWindowBoundsName[] =
    "Enable persistent window bounds in multi-displays scenario.";
const char kAshEnablePersistentWindowBoundsDescription[] =
    "Enable persistent window bounds in multi-displays scenario.";

const char kAshEnableTrilinearFilteringName[] = "Enable trilinear filtering.";
const char kAshEnableTrilinearFilteringDescription[] =
    "Enable trilinear filtering.";

const char kAshEnableUnifiedDesktopName[] = "Unified desktop mode";
const char kAshEnableUnifiedDesktopDescription[] =
    "Enable unified desktop mode which allows a window to span multiple "
    "displays.";

const char kAshShelfColorName[] = "Shelf color in Chrome OS system UI";
const char kAshShelfColorDescription[] =
    "Enables/disables the shelf color to be a derived from the wallpaper. The "
    "--ash-shelf-color-scheme flag defines how that color is derived.";

const char kAshShelfColorScheme[] = "Shelf color scheme in Chrome OS System UI";
const char kAshShelfColorSchemeDescription[] =
    "Specify how the color is derived from the wallpaper. This flag is only "
    "used when the --ash-shelf-color flag is enabled. Defaults to Dark & Muted";
const char kAshShelfColorSchemeLightVibrant[] = "Light & Vibrant";
const char kAshShelfColorSchemeNormalVibrant[] = "Normal & Vibrant";
const char kAshShelfColorSchemeDarkVibrant[] = "Dark & Vibrant";
const char kAshShelfColorSchemeLightMuted[] = "Light & Muted";
const char kAshShelfColorSchemeNormalMuted[] = "Normal & Muted";
const char kAshShelfColorSchemeDarkMuted[] = "Dark & Muted";

const char kBulkPrintersName[] = "Bulk Printers Policy";
const char kBulkPrintersDescription[] = "Enables the new bulk printers policy";

const char kCaptivePortalBypassProxyName[] =
    "Bypass proxy for Captive Portal Authorization";
const char kCaptivePortalBypassProxyDescription[] =
    "If proxy is configured, it usually prevents from authorization on "
    "different captive portals. This enables opening captive portal "
    "authorization dialog in a separate window, which ignores proxy settings.";

const char kCrOSComponentName[] = "Chrome OS Component";
const char kCrOSComponentDescription[] =
    "Disable the use of componentized escpr CUPS filter.";

const char kCrOSContainerName[] = "Chrome OS Container";
const char kCrOSContainerDescription[] =
    "Enable the use of Chrome OS Container utility.";

const char kCrosRegionsModeName[] = "Cros-regions load mode";
const char kCrosRegionsModeDescription[] =
    "This flag controls cros-regions load mode";
const char kCrosRegionsModeDefault[] = "Default";
const char kCrosRegionsModeOverride[] = "Override VPD values.";
const char kCrosRegionsModeHide[] = "Hide VPD values.";

const char kDisableLockScreenAppsName[] = "Disable lock screen note taking";
const char kDisableLockScreenAppsDescription[] =
    "Disable new-note action handler apps on the lock screen. The user will "
    "not be able to launch the preferred note-taking action from the lock "
    "screen, provided that the app supports lock screen note taking.";

const char kDisableNetworkSettingsConfigName[] =
    "Disable Settings based Network Configuration";
const char kDisableNetworkSettingsConfigDescription[] =
    "Disables the Settings based network configuration UI and restores the "
    "Views based configuration dialog.";

const char kDisableSystemTimezoneAutomaticDetectionName[] =
    "SystemTimezoneAutomaticDetection policy support";
const char kDisableSystemTimezoneAutomaticDetectionDescription[] =
    "Disable system timezone automatic detection device policy.";

const char kDisableTabletAutohideTitlebarsName[] =
    "Disable autohide titlebars in tablet mode";
const char kDisableTabletAutohideTitlebarsDescription[] =
    "Disable tablet mode autohide titlebars functionality. The user will be "
    "able to see the titlebar in tablet mode.";

const char kDisableTabletSplitViewName[] = "Disable split view in Tablet mode";
const char kDisableTabletSplitViewDescription[] =
    "Disable split view for Chrome OS tablet mode.";

const char kEnableAppShortcutSearchName[] =
    "Enable app shortcut search in launcher";
const char kEnableAppShortcutSearchDescription[] =
    "Enables app shortcut search in launcher";

const char kEnableBackgroundBlurName[] = "Enable background blur.";
const char kEnableBackgroundBlurDescription[] =
    "Enables background blur for the Peeking Launcher and Tab Switcher.";

const char kEnableChromevoxArcSupportName[] = "ChromeVox ARC support";
const char kEnableChromevoxArcSupportDescription[] =
    "Enable ChromeVox screen reader features in ARC";

const char kEnableDisplayZoomSettingName[] = "Enable display zoom settings";
const char kEnableDisplayZoomSettingDescription[] =
    "Allows the user to modify the display size or zoom via the chrome display "
    "settings page.";

const char kEnableDragTabsInTabletModeName[] =
    "Enable dragging tabs in tablet mode";
const char kEnableDragTabsInTabletModeDescription[] =
    "Allows the user to drag the tabs out of a browser window in tablet mode.";

const char kEnableEhvInputName[] =
    "Emoji, handwriting and voice input on opt-in IME menu";
const char kEnableEhvInputDescription[] =
    "Enable access to emoji, handwriting and voice input form opt-in IME menu.";

const char kEnableEncryptionMigrationName[] =
    "Enable encryption migration of user data";
const char kEnableEncryptionMigrationDescription[] =
    "If enabled and the device supports ARC, the user will be asked to update "
    "the encryption of user data when the user signs in.";

const char kEnableFloatingVirtualKeyboardName[] =
    "Enable floating virtual keyboard";
const char kEnableFloatingVirtualKeyboardDescription[] =
    "If enabled, the keyboard will use floating behavior by default.";

const char kEnableFullscreenHandwritingVirtualKeyboardName[] =
    "Enable full screen handwriting virtual keyboard";
const char kEnableFullscreenHandwritingVirtualKeyboardDescription[] =
    "If enabled, the handwriting virtual keyboard will allow user to write "
    "anywhere on the screen";

const char kEnableHomeLauncherName[] = "Enable home launcher";
const char kEnableHomeLauncherDescription[] =
    "Enable home launcher in tablet mode.";

const char kEnableImeMenuName[] = "Enable opt-in IME menu";
const char kEnableImeMenuDescription[] =
    "Enable access to the new IME menu in the Language Settings page.";

const char kEnableNewWallpaperPickerName[] = "Enable new wallpaper picker";
const char kEnableNewWallpaperPickerDescription[] =
    "Enable the redesigned wallpaper picker with new wallpaper collections.";

const char kEnablePerUserTimezoneName[] = "Per-user time zone preferences.";
const char kEnablePerUserTimezoneDescription[] =
    "Chrome OS system timezone preference is stored and handled for each user "
    "individually.";

const char kEnableSettingsShortcutSearchName[] =
    "Enable Settings shortcut search";
const char kEnableSettingsShortcutSearchDescription[] =
    "Enable Settings shortcut search in launcher.";

const char kEnableStylusVirtualKeyboardName[] =
    "Enable stylus virtual keyboard";
const char kEnableStylusVirtualKeyboardDescription[] =
    "If enabled, tapping with a stylus will show the handwriting virtual "
    "keyboard.";

const char kEnableUnifiedMultiDeviceSettingsName[] =
    "Enable unified MultiDevice settings";
const char kEnableUnifiedMultiDeviceSettingsDescription[] =
    "If enabled, the Chrome OS Settings UI will include a menu for the unified "
    "MultiDevice settings.";

const char kEnableUnifiedMultiDeviceSetupName[] =
    "Enable unified MultiDevice setup";
const char kEnableUnifiedMultiDeviceSetupDescription[] =
    "Enable the device to setup all MultiDevice services in a single workflow.";

const char kEnableZipArchiverPackerName[] = "ZIP archiver - Packer";
const char kEnableZipArchiverPackerDescription[] =
    "Enable the ability to archive files on Drive in the Files app";

const char kEolNotificationName[] = "Disable Device End of Life notification.";
const char kEolNotificationDescription[] =
    "Disable Notifcation when Device is End of Life.";

const char kExperimentalAccessibilityFeaturesName[] =
    "Experimental accessibility features";
const char kExperimentalAccessibilityFeaturesDescription[] =
    "Enable additional accessibility features in the Settings page.";

const char kExperimentalInputViewFeaturesName[] =
    "Experimental input view features";
const char kExperimentalInputViewFeaturesDescription[] =
    "Enable experimental features for IME input views.";

const char kFileManagerTouchModeName[] = "Files App. touch mode";
const char kFileManagerTouchModeDescription[] =
    "Touchscreen-specific interactions of the Files app.";

const char kFirstRunUiTransitionsName[] =
    "Animated transitions in the first-run tutorial";
const char kFirstRunUiTransitionsDescription[] =
    "Transitions during first-run tutorial are animated.";

const char kForceEnableStylusToolsName[] = "Force enable stylus features";
const char kForceEnableStylusToolsDescription[] =
    "Forces display of the stylus tools menu in the shelf and the stylus "
    "section in settings, even if there is no attached stylus device.";

const char kGestureEditingName[] = "Gesture editing for the virtual keyboard.";
const char kGestureEditingDescription[] =
    "Enable/Disable gesture editing option in the settings page for the "
    "virtual keyboard.";

const char kGestureTypingName[] = "Gesture typing for the virtual keyboard.";
const char kGestureTypingDescription[] =
    "Enable/Disable gesture typing option in the settings page for the virtual "
    "keyboard.";

const char kInputViewName[] = "Input views";
const char kInputViewDescription[] =
    "Enable IME extensions to supply custom views for user input such as "
    "virtual keyboards.";

const char kLockScreenNotificationName[] = "Lock screen notification";
const char kLockScreenNotificationDescription[] =
    "Enable notifications on the lock screen.";

const char kMaterialDesignInkDropAnimationSpeedName[] =
    "Material design ink drop animation speed";
const char kMaterialDesignInkDropAnimationSpeedDescription[] =
    "Sets the speed of the experimental visual feedback animations for "
    "material design.";
const char kMaterialDesignInkDropAnimationFast[] = "Fast";
const char kMaterialDesignInkDropAnimationSlow[] = "Slow";

const char kMemoryPressureThresholdName[] =
    "Memory discard strategy for advanced pressure handling";
const char kMemoryPressureThresholdDescription[] =
    "Memory discarding strategy to use";
const char kConservativeThresholds[] =
    "Conservative memory pressure release strategy";
const char kAggressiveCacheDiscardThresholds[] =
    "Aggressive cache release strategy";
const char kAggressiveTabDiscardThresholds[] =
    "Aggressive tab release strategy";
const char kAggressiveThresholds[] =
    "Aggressive tab and cache release strategy";

const char kMtpWriteSupportName[] = "MTP write support";
const char kMtpWriteSupportDescription[] =
    "MTP write support in File System API (and file manager). In-place editing "
    "operations are not supported.";

const char kMultideviceName[] = "Enable multidevice features";
const char kMultideviceDescription[] =
    "Enables UI for controlling multidevice features.";

const char kMultiDeviceApiName[] = "Enables the MultiDevice API";
const char kMultiDeviceApiDescription[] =
    "Enable Mojo-based API which provides synced device metadata and the "
    "ability to connect to other devices associated the logged-in Google "
    "account.";

const char kNativeSmbName[] = "Native Smb Client";
const char kNativeSmbDescription[] =
    "If enabled, allows connections to an smb filesystem via Files app";

const char kNetworkPortalNotificationName[] =
    "Notifications about captive portals";
const char kNetworkPortalNotificationDescription[] =
    "If enabled, notification is displayed when device is connected to a "
    "network behind captive portal.";

const char kNewKoreanImeName[] = "New Korean IME";
const char kNewKoreanImeDescription[] =
    "New Korean IME, which is based on Google Input Tools' HMM engine.";

const char kNewZipUnpackerName[] = "New ZIP unpacker";
const char kNewZipUnpackerDescription[] =
    "New ZIP unpacker flow, based on the File System Provider API.";

const char kOfficeEditingComponentAppName[] =
    "Office Editing for Docs, Sheets & Slides";
const char kOfficeEditingComponentAppDescription[] =
    "Office Editing for Docs, Sheets & Slides for testing purposes.";

const char kPhysicalKeyboardAutocorrectName[] = "Physical keyboard autocorrect";
const char kPhysicalKeyboardAutocorrectDescription[] =
    "Enable physical keyboard autocorrect for US keyboard, which can provide "
    "suggestions as typing on physical keyboard.";

const char kPrinterProviderSearchAppName[] =
    "Chrome Web Store Gallery app for printer drivers";
const char kPrinterProviderSearchAppDescription[] =
    "Enables Chrome Web Store Gallery app for printer drivers. The app "
    "searches Chrome Web Store for extensions that support printing to a USB "
    "printer with specific USB ID.";

const char kQuickUnlockPinName[] = "Quick Unlock (PIN)";
const char kQuickUnlockPinDescription[] =
    "Enabling PIN quick unlock allows you to use a PIN to unlock your ChromeOS "
    "device on the lock screen after you have signed into your device.";
const char kQuickUnlockPinSignin[] = "Enable PIN when logging in.";
const char kQuickUnlockPinSigninDescription[] =
    "Enabling PIN allows you to use a PIN to sign in and unlock your ChromeOS "
    "device. After changing this flag PIN needs to be set up again.";
const char kQuickUnlockFingerprint[] = "Quick Unlock (Fingerprint)";
const char kQuickUnlockFingerprintDescription[] =
    "Enabling fingerprint quick unlock allows you to setup and use a "
    "fingerprint to unlock your Chromebook on the lock screen after you have "
    "signed into your device.";

const char kShowTapsName[] = "Show taps";
const char kShowTapsDescription[] =
    "Draws a circle at each touch point, which makes touch points more obvious "
    "when projecting or mirroring the display. Similar to the Android OS "
    "developer option.";

const char kShowTouchHudName[] = "Show HUD for touch points";
const char kShowTouchHudDescription[] =
    "Shows a trail of colored dots for the last few touch points. Pressing "
    "Ctrl-Alt-I shows a heads-up display view in the top-left corner. Helps "
    "debug hardware issues that generate spurious touch events.";

const char kSysInternalsName[] = "Enable Sys-Internals";
const char kSysInternalsDescription[] =
    "If enabled, user can monitor system information at "
    "chrome://sys-internals.";

const char kTapVisualizerAppName[] = "Show taps with mojo app";
const char kTapVisualizerAppDescription[] =
    "Use an out-of-process mojo app to show touch points.";

const char kTeamDrivesName[] = "Enable Team Drives Integration";
const char kTeamDrivesDescription[] =
    "If enabled, files under Team Drives will appear in the Files app.";

const char kTetherName[] = "Instant Tethering";
const char kTetherDescription[] =
    "Enables Instant Tethering. Instant Tethering allows your nearby Google "
    "phone to share its Internet connection with this device.";

const char kTouchscreenCalibrationName[] =
    "Enable/disable touchscreen calibration option in material design settings";
const char kTouchscreenCalibrationDescription[] =
    "If enabled, the user can calibrate the touch screen displays in "
    "chrome://settings/display.";

const char kUiDevToolsName[] = "Enable native UI inspection";
const char kUiDevToolsDescription[] =
    "Enables inspection of native UI elements. For local inspection use "
    "chrome://inspect#other";

const char kUiShowCompositedLayerBordersName[] =
    "Show UI composited layer borders";
const char kUiShowCompositedLayerBordersDescription[] =
    "Show border around composited layers created by UI.";
const char kUiShowCompositedLayerBordersRenderPass[] = "RenderPass";
const char kUiShowCompositedLayerBordersSurface[] = "Surface";
const char kUiShowCompositedLayerBordersLayer[] = "Layer";
const char kUiShowCompositedLayerBordersAll[] = "All";

const char kUiSlowAnimationsName[] = "Slow UI animations";
const char kUiSlowAnimationsDescription[] = "Makes all UI animations slow.";

// Force UI Mode
const char kUiModeName[] = "Force Ui Mode";
const char kUiModeDescription[] =
    R"*(This flag can be used to force a certain mode on to a chromebook, )*"
    R"*(despite its current orientation. "Tablet" means that the )*"
    R"*(chromebook will act as if it were in tablet mode. "Clamshell" )*"
    R"*(means that the chromebook will act as if it were in clamshell )*"
    R"*(mode . "Auto" means that the chromebook will alternate between )*"
    R"*(the two, based on its orientation.)*";
const char kUiModeTablet[] = "TouchView";
const char kUiModeClamshell[] = "Clamshell";
const char kUiModeAuto[] = "Auto (default)";

const char kUseMashName[] = "Out-of-process system UI (mash)";
const char kUseMashDescription[] =
    "Runs the mojo UI service (mus) and the ash window manager and system UI "
    "in a separate process.";

// TODO(mcasas): remove after https://crbug.com/771345.
const char kUseMonitorColorSpaceName[] = "Use monitor color space";
const char kUseMonitorColorSpaceDescription[] =
    "Enables Chrome to use the  color space information provided by the monitor"
    " instead of the default sRGB color space.";

const char kVideoPlayerChromecastSupportName[] =
    "Experimental Chromecast support for Video Player";
const char kVideoPlayerChromecastSupportDescription[] =
    "This option enables experimental Chromecast support for Video Player app "
    "on ChromeOS.";

const char kVirtualKeyboardName[] = "Virtual Keyboard";
const char kVirtualKeyboardDescription[] = "Enable virtual keyboard support.";

const char kVirtualKeyboardOverscrollName[] = "Virtual Keyboard Overscroll";
const char kVirtualKeyboardOverscrollDescription[] =
    "Enables virtual keyboard overscroll support.";

const char kVoiceInputName[] = "Voice input on virtual keyboard";
const char kVoiceInputDescription[] =
    "Enables voice input on virtual keyboard.";

const char kWakeOnPacketsName[] = "Wake On Packets";
const char kWakeOnPacketsDescription[] =
    "Enables waking the device based on the receipt of some network packets.";

const char kZipArchiverUnpackerName[] = "ZIP archiver - Unpacker";
const char kZipArchiverUnpackerDescription[] =
    "Enable or disable the ability to unpack archives in incognito mode";

#endif  // defined(OS_CHROMEOS)

// Random platform combinations -----------------------------------------------

#if defined(OS_WIN) || defined(OS_LINUX)

const char kEnableInputImeApiName[] = "Enable Input IME API";
const char kEnableInputImeApiDescription[] =
    "Enable the use of chrome.input.ime API.";

#if !defined(OS_CHROMEOS)

const char kWarnBeforeQuittingFlagName[] = "Warn Before Quitting";
const char kWarnBeforeQuittingFlagDescription[] =
    "Confirm to quit by either holding the quit shortcut or pressing it twice.";

#endif  // !defined(OS_CHROMEOS)

#endif  // defined(OS_WIN) || defined(OS_LINUX)

#if defined(OS_WIN) || defined(OS_MACOSX)

const char kAutomaticTabDiscardingName[] = "Automatic tab discarding";
const char kAutomaticTabDiscardingDescription[] =
    "If enabled, tabs get automatically discarded from memory when the system "
    "memory is low. Discarded tabs are still visible on the tab strip and get "
    "reloaded when clicked on. Info about discarded tabs can be found at "
    "chrome://discards.";

#endif  // defined(OS_WIN) || defined(OS_MACOSX)

// Feature flags --------------------------------------------------------------

#if BUILDFLAG(ENABLE_VR)

const char kWebVrVsyncAlignName[] = "WebVR VSync-aligned timing";
const char kWebVrVsyncAlignDescription[] =
    "Align WebVR application rendering with VSync for smoother animations.";

#if defined(OS_ANDROID)

const char kVrWebInputEditingName[] = "VR browsing web input editing";
const char kVrWebInputEditingDescription[] =
    "Allow editing web input fields while in VR mode.";

const char kVrBrowsingExperimentalFeaturesName[] =
    "VR browsing experimental features";
const char kVrBrowsingExperimentalFeaturesDescription[] =
    "Experimental VR browsing features that are under development.";

const char kVrBrowsingExperimentalRenderingName[] =
    "VR browsing experimental rendering features";
const char kVrBrowsingExperimentalRenderingDescription[] =
    "Experimental rendering features for VR browsing (e.g. power-saving "
    "rendering modes).";

const char kVrBrowsingInCustomTabName[] = "VR browsing in Custom Tabs";
const char kVrBrowsingInCustomTabDescription[] =
    "Allow browsing within a VR headset while in a Custom Tab.";

const char kWebVrAutopresentFromIntentName[] =
    "WebVR auto presentation from intents";
const char kWebVrAutopresentFromIntentDescription[] =
    "Allow auto presentation of WebVR content from trusted first-party apps.";

#endif  // OS_ANDROID

#if BUILDFLAG(ENABLE_OCULUS_VR)
const char kOculusVRName[] = "Oculus hardware support";
const char kOculusVRDescription[] =
    "If enabled, Chrome will use Oculus devices for VR (supported only on "
    "Windows 10 or later).";
#endif  // ENABLE_OCULUS_VR

#if BUILDFLAG(ENABLE_OPENVR)
const char kOpenVRName[] = "OpenVR hardware support";
const char kOpenVRDescription[] =
    "If enabled, Chrome will use OpenVR devices for VR (supported only on "
    "Windows 10 or later).";
#endif  // ENABLE_OPENVR

#endif  // ENABLE_VR

#if BUILDFLAG(ENABLE_NACL)

const char kNaclDebugMaskName[] =
    "Restrict Native Client GDB-based debugging by pattern";
const char kNaclDebugMaskDescription[] =
    "Restricts Native Client application GDB-based debugging by URL of "
    "manifest file. Native Client GDB-based debugging must be enabled for this "
    "option to work.";
const char kNaclDebugMaskChoiceDebugAll[] = "Debug everything.";
const char kNaclDebugMaskChoiceExcludeUtilsPnacl[] =
    "Debug everything except secure shell and the PNaCl translator.";
const char kNaclDebugMaskChoiceIncludeDebug[] =
    "Debug only if manifest URL ends with debug.nmf.";

const char kNaclDebugName[] = "Native Client GDB-based debugging";
const char kNaclDebugDescription[] =
    "Enable GDB debug stub. This will stop a Native Client application on "
    "startup and wait for nacl-gdb (from the NaCl SDK) to attach to it.";

const char kNaclName[] = "Native Client";
const char kNaclDescription[] =
    "Support Native Client for all web applications, even those that were not "
    "installed from the Chrome Web Store.";

const char kPnaclSubzeroName[] = "Force PNaCl Subzero";
const char kPnaclSubzeroDescription[] =
    "Force the use of PNaCl's fast Subzero translator for all pexe files.";

#endif  // BUILDFLAG(ENABLE_NACL)

const char kWebrtcH264WithOpenh264FfmpegName[] =
    "WebRTC H.264 software video encoder/decoder";
const char kWebrtcH264WithOpenh264FfmpegDescription[] =
    "When enabled, an H.264 software video encoder/decoder pair is included. "
    "If a hardware encoder/decoder is also available it may be used instead of "
    "this encoder/decoder.";

#if defined(TOOLKIT_VIEWS) || defined(OS_ANDROID)

const char kAutofillCreditCardUploadName[] =
    "Enable offering upload of Autofilled credit cards";
const char kAutofillCreditCardUploadDescription[] =
    "Enables a new option to upload credit cards to Google Payments for sync "
    "to all Chrome devices.";

#endif  // defined(TOOLKIT_VIEWS) || defined(OS_ANDROID)

#if defined(OS_ANDROID)

const char kDisplayCutoutAPIName[] =
    "Enable support for the Display Cutout API";
const char kDisplayCutoutAPIDescription[] =
    "Enables developers to support devices that have a display cutout.";

#endif  // defined(OS_ANDROID)

// ============================================================================
// Don't just add flags to the end, put them in the right section in
// alphabetical order just like the header file.
// ============================================================================

}  // namespace flag_descriptions
