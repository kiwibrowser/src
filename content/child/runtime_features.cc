// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/runtime_features.h"

#include <vector>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "content/common/content_switches_internal.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "media/base/media_switches.h"
#include "services/device/public/cpp/device_features.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "ui/gfx/switches.h"
#include "ui/gl/gl_switches.h"
#include "ui/native_theme/native_theme_features.h"

using blink::WebRuntimeFeatures;

namespace content {

static void SetRuntimeFeatureDefaultsForPlatform() {
#if defined(OS_ANDROID)
  // Android does not have support for PagePopup
  WebRuntimeFeatures::EnablePagePopup(false);
  // No plan to support complex UI for date/time INPUT types.
  WebRuntimeFeatures::EnableInputMultipleFieldsUI(false);
  // Android does not yet support SharedWorker. crbug.com/154571
  WebRuntimeFeatures::EnableSharedWorker(false);
  // Android does not yet support NavigatorContentUtils.
  WebRuntimeFeatures::EnableNavigatorContentUtils(false);
  WebRuntimeFeatures::EnableOrientationEvent(true);
  WebRuntimeFeatures::EnableFastMobileScrolling(true);
  WebRuntimeFeatures::EnableMediaCapture(true);
  // Android won't be able to reliably support non-persistent notifications, the
  // intended behavior for which is in flux by itself.
  WebRuntimeFeatures::EnableNotificationConstructor(false);
  // Android does not yet support switching of audio output devices
  WebRuntimeFeatures::EnableAudioOutputDevices(false);
  WebRuntimeFeatures::EnableAutoplayMutedVideos(true);
  // Android does not yet support SystemMonitor.
  WebRuntimeFeatures::EnableOnDeviceChange(false);
  WebRuntimeFeatures::EnableMediaSession(true);
  WebRuntimeFeatures::EnableMediaControlsOverlayPlayButton(true);
  WebRuntimeFeatures::EnableRemotePlaybackBackend(true);
#else  // defined(OS_ANDROID)
  WebRuntimeFeatures::EnableNavigatorContentUtils(true);
  // Tracking bug for the implementation: https://crbug.com/728609
  WebRuntimeFeatures::EnableRemotePlaybackBackend(false);
#endif  // defined(OS_ANDROID)

#if defined(OS_ANDROID) || defined(USE_AURA)
  WebRuntimeFeatures::EnableCompositedSelectionUpdate(true);
#endif

#if !(defined OS_ANDROID || defined OS_CHROMEOS)
  // Only Android, ChromeOS support NetInfo downlinkMax, type and ontypechange
  // now.
  WebRuntimeFeatures::EnableNetInfoDownlinkMax(false);
#endif

// Web Bluetooth is shipped on Android, ChromeOS & MacOS, experimental
// otherwise.
#if defined(OS_CHROMEOS) || defined(OS_ANDROID) || defined(OS_MACOSX)
  WebRuntimeFeatures::EnableWebBluetooth(true);
#endif

#if defined(OS_CHROMEOS)
  WebRuntimeFeatures::EnableForceTallerSelectPopup(true);
#endif

// The Notification Center on Mac OS X does not support content images.
#if !defined(OS_MACOSX)
  WebRuntimeFeatures::EnableNotificationContentImage(true);
#endif
}

void SetRuntimeFeaturesDefaultsAndUpdateFromArgs(
    const base::CommandLine& command_line) {
  bool enableExperimentalWebPlatformFeatures = command_line.HasSwitch(
      switches::kEnableExperimentalWebPlatformFeatures);
  if (enableExperimentalWebPlatformFeatures)
    WebRuntimeFeatures::EnableExperimentalFeatures(true);

  SetRuntimeFeatureDefaultsForPlatform();

  // Begin individual features.
  // Do not add individual features above this line.

  WebRuntimeFeatures::EnableOriginTrials(
      base::FeatureList::IsEnabled(features::kOriginTrials));

  if (!base::FeatureList::IsEnabled(features::kWebUsb))
    WebRuntimeFeatures::EnableWebUsb(false);

  if (command_line.HasSwitch(switches::kDisableDatabases))
    WebRuntimeFeatures::EnableDatabase(false);

  if (command_line.HasSwitch(switches::kDisableNotifications)) {
    WebRuntimeFeatures::EnableNotifications(false);

    // Chrome's Push Messaging implementation relies on Web Notifications.
    WebRuntimeFeatures::EnablePushMessaging(false);
  }

  if (!base::FeatureList::IsEnabled(features::kNotificationContentImage))
    WebRuntimeFeatures::EnableNotificationContentImage(false);

  WebRuntimeFeatures::EnableWebAssemblyStreaming(
      base::FeatureList::IsEnabled(features::kWebAssemblyStreaming));

  WebRuntimeFeatures::EnableSharedArrayBuffer(
      base::FeatureList::IsEnabled(features::kSharedArrayBuffer));

  if (command_line.HasSwitch(switches::kDisableSharedWorkers))
    WebRuntimeFeatures::EnableSharedWorker(false);

  if (command_line.HasSwitch(switches::kDisableSpeechAPI))
    WebRuntimeFeatures::EnableScriptedSpeech(false);

  if (command_line.HasSwitch(switches::kDisableFileSystem))
    WebRuntimeFeatures::EnableFileSystem(false);

  if (!command_line.HasSwitch(switches::kDisableAcceleratedJpegDecoding))
    WebRuntimeFeatures::EnableDecodeToYUV(true);

  if (command_line.HasSwitch(switches::kEnableWebGLDraftExtensions))
    WebRuntimeFeatures::EnableWebGLDraftExtensions(true);

  if (command_line.HasSwitch(switches::kEnableAutomation) ||
      command_line.HasSwitch(switches::kHeadless)) {
    WebRuntimeFeatures::EnableAutomationControlled(true);
  }

#if defined(OS_MACOSX)
  const bool enable_canvas_2d_image_chromium =
      command_line.HasSwitch(
          switches::kEnableGpuMemoryBufferCompositorResources) &&
      !command_line.HasSwitch(switches::kDisable2dCanvasImageChromium) &&
      !command_line.HasSwitch(switches::kDisableGpu) &&
      base::FeatureList::IsEnabled(features::kCanvas2DImageChromium);
#else
  bool enable_canvas_2d_image_chromium = false;
#endif
  WebRuntimeFeatures::EnableCanvas2dImageChromium(
      enable_canvas_2d_image_chromium);

#if defined(OS_MACOSX)
  bool enable_web_gl_image_chromium = command_line.HasSwitch(
      switches::kEnableGpuMemoryBufferCompositorResources) &&
      !command_line.HasSwitch(switches::kDisableWebGLImageChromium) &&
      !command_line.HasSwitch(switches::kDisableGpu);

  if (enable_web_gl_image_chromium) {
    enable_web_gl_image_chromium =
        base::FeatureList::IsEnabled(features::kWebGLImageChromium);
  }
#else
  bool enable_web_gl_image_chromium =
      command_line.HasSwitch(switches::kEnableWebGLImageChromium);
#endif
  WebRuntimeFeatures::EnableWebGLImageChromium(enable_web_gl_image_chromium);

  if (command_line.HasSwitch(switches::kForceOverlayFullscreenVideo))
    WebRuntimeFeatures::ForceOverlayFullscreenVideo(true);

  if (ui::IsOverlayScrollbarEnabled())
    WebRuntimeFeatures::EnableOverlayScrollbars(true);

  if (command_line.HasSwitch(switches::kEnablePreciseMemoryInfo))
    WebRuntimeFeatures::EnablePreciseMemoryInfo(true);

  if (command_line.HasSwitch(switches::kEnablePrintBrowser))
    WebRuntimeFeatures::EnablePrintBrowser(true);

  if (command_line.HasSwitch(switches::kEnableNetworkInformationDownlinkMax) ||
      enableExperimentalWebPlatformFeatures) {
    WebRuntimeFeatures::EnableNetInfoDownlinkMax(true);
  }

  if (command_line.HasSwitch(switches::kReducedReferrerGranularity))
    WebRuntimeFeatures::EnableReducedReferrerGranularity(true);

  WebRuntimeFeatures::EnableIntersectionObserverGeometryMapper(
      base::FeatureList::IsEnabled(
          features::kIntersectionObserverGeometryMapper));

  if (command_line.HasSwitch(switches::kDisablePermissionsAPI))
    WebRuntimeFeatures::EnablePermissionsAPI(false);

  if (command_line.HasSwitch(switches::kDisableV8IdleTasks))
    WebRuntimeFeatures::EnableV8IdleTasks(false);
  else
    WebRuntimeFeatures::EnableV8IdleTasks(true);

  if (command_line.HasSwitch(switches::kEnableWebVR))
    WebRuntimeFeatures::EnableWebVR(true);

  if (base::FeatureList::IsEnabled(features::kWebXr))
    WebRuntimeFeatures::EnableWebXR(true);

  if (base::FeatureList::IsEnabled(features::kWebXrGamepadSupport))
    WebRuntimeFeatures::EnableWebXRGamepadSupport(true);

  if (base::FeatureList::IsEnabled(features::kWebXrHitTest))
    WebRuntimeFeatures::EnableWebXRHitTest(true);

  if (command_line.HasSwitch(switches::kDisablePresentationAPI))
    WebRuntimeFeatures::EnablePresentationAPI(false);

  if (command_line.HasSwitch(switches::kDisableRemotePlaybackAPI))
    WebRuntimeFeatures::EnableRemotePlaybackAPI(false);

  WebRuntimeFeatures::EnableSecMetadata(
      base::FeatureList::IsEnabled(features::kSecMetadata) ||
      enableExperimentalWebPlatformFeatures);

  WebRuntimeFeatures::EnableUserActivationV2(
      base::FeatureList::IsEnabled(features::kUserActivationV2));

  if (base::FeatureList::IsEnabled(features::kScrollAnchorSerialization))
    WebRuntimeFeatures::EnableScrollAnchorSerialization(true);

  WebRuntimeFeatures::EnableFeatureFromString(
      "SlimmingPaintV175",
      base::FeatureList::IsEnabled(features::kSlimmingPaintV175) ||
          command_line.HasSwitch(switches::kEnableSlimmingPaintV175) ||
          enableExperimentalWebPlatformFeatures);

  WebRuntimeFeatures::EnableFeatureFromString(
      "BlinkGenPropertyTrees",
      command_line.HasSwitch(switches::kEnableBlinkGenPropertyTrees));

  if (command_line.HasSwitch(switches::kEnableSlimmingPaintV2))
    WebRuntimeFeatures::EnableSlimmingPaintV2(true);

  if (base::FeatureList::IsEnabled(features::kLazyParseCSS))
    WebRuntimeFeatures::EnableLazyParseCSS(true);

  WebRuntimeFeatures::EnablePassiveDocumentEventListeners(
      base::FeatureList::IsEnabled(features::kPassiveDocumentEventListeners));

  WebRuntimeFeatures::EnableFeatureFromString(
      "FontCacheScaling",
      base::FeatureList::IsEnabled(features::kFontCacheScaling));

  WebRuntimeFeatures::EnableFeatureFromString(
      "FramebustingNeedsSameOriginOrUserGesture",
      base::FeatureList::IsEnabled(
          features::kFramebustingNeedsSameOriginOrUserGesture));

  if (command_line.HasSwitch(switches::kDisableBackgroundTimerThrottling))
    WebRuntimeFeatures::EnableTimerThrottlingForBackgroundTabs(false);

  WebRuntimeFeatures::EnableExpensiveBackgroundTimerThrottling(
      base::FeatureList::IsEnabled(
          features::kExpensiveBackgroundTimerThrottling));

  if (base::FeatureList::IsEnabled(features::kHeapCompaction))
    WebRuntimeFeatures::EnableHeapCompaction(true);

  WebRuntimeFeatures::EnableRenderingPipelineThrottling(
      base::FeatureList::IsEnabled(features::kRenderingPipelineThrottling));

  WebRuntimeFeatures::EnableTimerThrottlingForHiddenFrames(
      base::FeatureList::IsEnabled(features::kTimerThrottlingForHiddenFrames));

  WebRuntimeFeatures::EnableTouchpadAndWheelScrollLatching(
      base::FeatureList::IsEnabled(features::kTouchpadAndWheelScrollLatching));

  if (base::FeatureList::IsEnabled(
          features::kSendBeaconThrowForBlobWithNonSimpleType))
    WebRuntimeFeatures::EnableSendBeaconThrowForBlobWithNonSimpleType(true);

#if defined(OS_ANDROID)
  if (command_line.HasSwitch(switches::kDisableMediaSessionAPI))
    WebRuntimeFeatures::EnableMediaSession(false);
#endif

  WebRuntimeFeatures::EnablePaymentRequest(
      base::FeatureList::IsEnabled(features::kWebPayments));

  if (base::FeatureList::IsEnabled(features::kServiceWorkerPaymentApps))
    WebRuntimeFeatures::EnablePaymentApp(true);

  WebRuntimeFeatures::EnableServiceWorkerScriptFullCodeCache(
      base::FeatureList::IsEnabled(
          features::kServiceWorkerScriptFullCodeCache));

  WebRuntimeFeatures::EnableNetworkService(
      base::FeatureList::IsEnabled(network::features::kNetworkService));

  WebRuntimeFeatures::EnableMojoBlobURLs(
      base::FeatureList::IsEnabled(network::features::kNetworkService));

  if (base::FeatureList::IsEnabled(features::kGamepadExtensions))
    WebRuntimeFeatures::EnableGamepadExtensions(true);

  if (base::FeatureList::IsEnabled(features::kGamepadVibration))
    WebRuntimeFeatures::EnableGamepadVibration(true);

  if (base::FeatureList::IsEnabled(features::kCompositeOpaqueFixedPosition))
    WebRuntimeFeatures::EnableFeatureFromString("CompositeOpaqueFixedPosition",
                                                true);

  if (!base::FeatureList::IsEnabled(features::kCompositeOpaqueScrollers))
    WebRuntimeFeatures::EnableFeatureFromString("CompositeOpaqueScrollers",
                                                false);
  if (base::FeatureList::IsEnabled(features::kCompositorTouchAction))
    WebRuntimeFeatures::EnableCompositorTouchAction(true);

  if (base::FeatureList::IsEnabled(features::kGenericSensor)) {
    WebRuntimeFeatures::EnableGenericSensor(true);
    if (base::FeatureList::IsEnabled(features::kGenericSensorExtraClasses))
      WebRuntimeFeatures::EnableGenericSensorExtraClasses(true);
  }

  if (base::FeatureList::IsEnabled(network::features::kOutOfBlinkCORS))
    WebRuntimeFeatures::EnableOutOfBlinkCORS(true);

  if (base::FeatureList::IsEnabled(features::kOriginManifest))
    WebRuntimeFeatures::EnableOriginManifest(true);

  WebRuntimeFeatures::EnableMediaCastOverlayButton(
      base::FeatureList::IsEnabled(media::kMediaCastOverlayButton));

  if (!base::FeatureList::IsEnabled(features::kBlockCredentialedSubresources)) {
    WebRuntimeFeatures::EnableFeatureFromString("BlockCredentialedSubresources",
                                                false);
  }

  if (base::FeatureList::IsEnabled(features::kRasterInducingScroll))
    WebRuntimeFeatures::EnableRasterInducingScroll(true);

  WebRuntimeFeatures::EnableFeatureFromString(
      "AllowContentInitiatedDataUrlNavigations",
      base::FeatureList::IsEnabled(
          features::kAllowContentInitiatedDataUrlNavigations));

#if defined(OS_ANDROID)
  if (base::FeatureList::IsEnabled(features::kWebNfc))
    WebRuntimeFeatures::EnableWebNfc(true);
#endif

  if (media::GetEffectiveAutoplayPolicy(command_line) !=
      switches::autoplay::kNoUserGestureRequiredPolicy) {
    WebRuntimeFeatures::EnableAutoplayMutedVideos(true);
  }

  WebRuntimeFeatures::EnableWebAuth(
      base::FeatureList::IsEnabled(features::kWebAuth));

  WebRuntimeFeatures::EnableClientPlaceholdersForServerLoFi(
      base::GetFieldTrialParamValue("PreviewsClientLoFi",
                                    "replace_server_placeholders") != "false");

  WebRuntimeFeatures::EnableResourceLoadScheduler(
      base::FeatureList::IsEnabled(features::kResourceLoadScheduler));

  if (base::FeatureList::IsEnabled(features::kLayeredAPI))
    WebRuntimeFeatures::EnableLayeredAPI(true);

  WebRuntimeFeatures::EnableLazyInitializeMediaControls(
      base::FeatureList::IsEnabled(features::kLazyInitializeMediaControls));

  WebRuntimeFeatures::EnableMediaEngagementBypassAutoplayPolicies(
      base::FeatureList::IsEnabled(
          media::kMediaEngagementBypassAutoplayPolicies));

  WebRuntimeFeatures::EnableModuleScriptsDynamicImport(
      base::FeatureList::IsEnabled(features::kModuleScriptsDynamicImport));

  WebRuntimeFeatures::EnableModuleScriptsImportMetaUrl(
      base::FeatureList::IsEnabled(features::kModuleScriptsImportMetaUrl));

  WebRuntimeFeatures::EnableOverflowIconsForMediaControls(
      base::FeatureList::IsEnabled(media::kOverflowIconsForMediaControls));

  WebRuntimeFeatures::EnableAllowActivationDelegationAttr(
      base::FeatureList::IsEnabled(features::kAllowActivationDelegationAttr));

  WebRuntimeFeatures::EnableModernMediaControls(
      base::FeatureList::IsEnabled(media::kUseModernMediaControls));

  WebRuntimeFeatures::EnableWorkStealingInScriptRunner(
      base::FeatureList::IsEnabled(features::kWorkStealingInScriptRunner));

  WebRuntimeFeatures::EnableFeatureFromString(
      "FeaturePolicyForPermissions",
      base::FeatureList::IsEnabled(features::kUseFeaturePolicyForPermissions));

  if (base::FeatureList::IsEnabled(features::kKeyboardLockAPI))
    WebRuntimeFeatures::EnableFeatureFromString("KeyboardLock", true);

  if (base::FeatureList::IsEnabled(features::kLazyFrameLoading))
    WebRuntimeFeatures::EnableLazyFrameLoading(true);

  WebRuntimeFeatures::EnableV8ContextSnapshot(
      base::FeatureList::IsEnabled(features::kV8ContextSnapshot));

  if (base::FeatureList::IsEnabled(features::kStopInBackground))
    WebRuntimeFeatures::EnableStopInBackground(true);

  if (base::FeatureList::IsEnabled(features::kStopLoadingInBackground))
    WebRuntimeFeatures::EnableStopLoadingInBackground(true);

  if (base::FeatureList::IsEnabled(features::kStopNonTimersInBackground))
    WebRuntimeFeatures::EnableStopNonTimersInBackground(true);

  WebRuntimeFeatures::EnablePWAFullCodeCache(
      base::FeatureList::IsEnabled(features::kPWAFullCodeCache));

  WebRuntimeFeatures::EnableRequireCSSExtensionForFile(
      base::FeatureList::IsEnabled(features::kRequireCSSExtensionForFile));

  WebRuntimeFeatures::EnablePictureInPicture(
      base::FeatureList::IsEnabled(media::kPictureInPicture));

  WebRuntimeFeatures::EnableCacheInlineScriptCode(
      base::FeatureList::IsEnabled(features::kCacheInlineScriptCode));

  // Make srcset on link rel=preload work with SignedHTTPExchange flag too.
  if (base::FeatureList::IsEnabled(features::kSignedHTTPExchange))
    WebRuntimeFeatures::EnablePreloadImageSrcSetEnabled(true);

  WebRuntimeFeatures::EnableOffMainThreadWebSocket(
      base::FeatureList::IsEnabled(features::kOffMainThreadWebSocket));

  if (base::FeatureList::IsEnabled(
          features::kExperimentalProductivityFeatures)) {
    WebRuntimeFeatures::EnableExperimentalProductivityFeatures(true);
  }

#if defined(OS_ANDROID)
  if (base::FeatureList::IsEnabled(features::kDisplayCutoutAPI))
    WebRuntimeFeatures::EnableDisplayCutoutViewportFit(true);
#endif

  // End individual features.
  // Do not add individual features below this line.

  if (command_line.HasSwitch(
          switches::kDisableOriginTrialControlledBlinkFeatures)) {
    WebRuntimeFeatures::EnableOriginTrialControlledFeatures(false);
  }

  WebRuntimeFeatures::EnableAutoplayIgnoresWebAudio(
      base::FeatureList::IsEnabled(media::kAutoplayIgnoreWebAudio));

  // Enable explicitly enabled features, and then disable explicitly disabled
  // ones.
  for (const std::string& feature :
       FeaturesFromSwitch(command_line, switches::kEnableBlinkFeatures)) {
    WebRuntimeFeatures::EnableFeatureFromString(feature, true);
  }
  for (const std::string& feature :
       FeaturesFromSwitch(command_line, switches::kDisableBlinkFeatures)) {
    WebRuntimeFeatures::EnableFeatureFromString(feature, false);
  }
};

}  // namespace content
