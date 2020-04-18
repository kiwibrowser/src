/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/public/platform/web_runtime_features.h"

#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

void WebRuntimeFeatures::EnableExperimentalFeatures(bool enable) {
  RuntimeEnabledFeatures::SetExperimentalFeaturesEnabled(enable);
}

void WebRuntimeFeatures::EnableWebBluetooth(bool enable) {
  RuntimeEnabledFeatures::SetWebBluetoothEnabled(enable);
}

void WebRuntimeFeatures::EnableWebAssemblyStreaming(bool enable) {
  RuntimeEnabledFeatures::SetWebAssemblyStreamingEnabled(enable);
}

void WebRuntimeFeatures::EnableWebNfc(bool enable) {
  RuntimeEnabledFeatures::SetWebNFCEnabled(enable);
}

void WebRuntimeFeatures::EnableWebUsb(bool enable) {
  RuntimeEnabledFeatures::SetWebUSBEnabled(enable);
}

void WebRuntimeFeatures::EnableFeatureFromString(const std::string& name,
                                                 bool enable) {
  RuntimeEnabledFeatures::SetFeatureEnabledFromString(name, enable);
}

void WebRuntimeFeatures::EnableTestOnlyFeatures(bool enable) {
  RuntimeEnabledFeatures::SetTestFeaturesEnabled(enable);
}

void WebRuntimeFeatures::EnableOriginTrialControlledFeatures(bool enable) {
  RuntimeEnabledFeatures::SetOriginTrialControlledFeaturesEnabled(enable);
}

void WebRuntimeFeatures::EnableOriginManifest(bool enable) {
  RuntimeEnabledFeatures::SetOriginManifestEnabled(enable);
}

void WebRuntimeFeatures::EnableOutOfBlinkCORS(bool enable) {
  RuntimeEnabledFeatures::SetOutOfBlinkCORSEnabled(enable);
}

void WebRuntimeFeatures::EnableAccelerated2dCanvas(bool enable) {
  RuntimeEnabledFeatures::SetAccelerated2dCanvasEnabled(enable);
}

void WebRuntimeFeatures::EnableAllowActivationDelegationAttr(bool enable) {
  RuntimeEnabledFeatures::SetAllowActivationDelegationAttrEnabled(enable);
}

void WebRuntimeFeatures::EnableAudioOutputDevices(bool enable) {
  RuntimeEnabledFeatures::SetAudioOutputDevicesEnabled(enable);
}

void WebRuntimeFeatures::EnableCacheInlineScriptCode(bool enable) {
  RuntimeEnabledFeatures::SetCacheInlineScriptCodeEnabled(enable);
}

void WebRuntimeFeatures::EnableCanvas2dImageChromium(bool enable) {
  RuntimeEnabledFeatures::SetCanvas2dImageChromiumEnabled(enable);
}

void WebRuntimeFeatures::EnableCompositedSelectionUpdate(bool enable) {
  RuntimeEnabledFeatures::SetCompositedSelectionUpdateEnabled(enable);
}

bool WebRuntimeFeatures::IsCompositedSelectionUpdateEnabled() {
  return RuntimeEnabledFeatures::CompositedSelectionUpdateEnabled();
}

void WebRuntimeFeatures::EnableCompositorTouchAction(bool enable) {
  RuntimeEnabledFeatures::SetCompositorTouchActionEnabled(enable);
}

void WebRuntimeFeatures::EnableCSSHexAlphaColor(bool enable) {
  RuntimeEnabledFeatures::SetCSSHexAlphaColorEnabled(enable);
}

void WebRuntimeFeatures::EnableScrollTopLeftInterop(bool enable) {
  RuntimeEnabledFeatures::SetScrollTopLeftInteropEnabled(enable);
}

void WebRuntimeFeatures::EnableDatabase(bool enable) {
  RuntimeEnabledFeatures::SetDatabaseEnabled(enable);
}

void WebRuntimeFeatures::EnableDecodeToYUV(bool enable) {
  RuntimeEnabledFeatures::SetDecodeToYUVEnabled(enable);
}

void WebRuntimeFeatures::EnableFastMobileScrolling(bool enable) {
  RuntimeEnabledFeatures::SetFastMobileScrollingEnabled(enable);
}

void WebRuntimeFeatures::EnableFileSystem(bool enable) {
  RuntimeEnabledFeatures::SetFileSystemEnabled(enable);
}

void WebRuntimeFeatures::EnableForceTallerSelectPopup(bool enable) {
  RuntimeEnabledFeatures::SetForceTallerSelectPopupEnabled(enable);
}

void WebRuntimeFeatures::EnableGamepadExtensions(bool enable) {
  RuntimeEnabledFeatures::SetGamepadExtensionsEnabled(enable);
}

void WebRuntimeFeatures::EnableGamepadVibration(bool enable) {
  RuntimeEnabledFeatures::SetGamepadVibrationEnabled(enable);
}

void WebRuntimeFeatures::EnableGenericSensor(bool enable) {
  RuntimeEnabledFeatures::SetSensorEnabled(enable);
}

void WebRuntimeFeatures::EnableGenericSensorExtraClasses(bool enable) {
  RuntimeEnabledFeatures::SetSensorExtraClassesEnabled(enable);
}

void WebRuntimeFeatures::EnableHeapCompaction(bool enable) {
  RuntimeEnabledFeatures::SetHeapCompactionEnabled(enable);
}

void WebRuntimeFeatures::EnableInputMultipleFieldsUI(bool enable) {
  RuntimeEnabledFeatures::SetInputMultipleFieldsUIEnabled(enable);
}

void WebRuntimeFeatures::EnableIntersectionObserverGeometryMapper(bool enable) {
  RuntimeEnabledFeatures::SetIntersectionObserverGeometryMapperEnabled(enable);
}

void WebRuntimeFeatures::EnableLayeredAPI(bool enable) {
  RuntimeEnabledFeatures::SetLayeredAPIEnabled(enable);
}

void WebRuntimeFeatures::EnableLazyFrameLoading(bool enable) {
  RuntimeEnabledFeatures::SetLazyFrameLoadingEnabled(enable);
}

void WebRuntimeFeatures::EnableLazyParseCSS(bool enable) {
  RuntimeEnabledFeatures::SetLazyParseCSSEnabled(enable);
}

void WebRuntimeFeatures::EnableMediaCapture(bool enable) {
  RuntimeEnabledFeatures::SetMediaCaptureEnabled(enable);
}

void WebRuntimeFeatures::EnableMediaSession(bool enable) {
  RuntimeEnabledFeatures::SetMediaSessionEnabled(enable);
}

void WebRuntimeFeatures::EnableModernMediaControls(bool enable) {
  RuntimeEnabledFeatures::SetModernMediaControlsEnabled(enable);
}

void WebRuntimeFeatures::EnableModuleScriptsDynamicImport(bool enable) {
  RuntimeEnabledFeatures::SetModuleScriptsDynamicImportEnabled(enable);
}

void WebRuntimeFeatures::EnableModuleScriptsImportMetaUrl(bool enable) {
  RuntimeEnabledFeatures::SetModuleScriptsImportMetaUrlEnabled(enable);
}

void WebRuntimeFeatures::EnableNotificationConstructor(bool enable) {
  RuntimeEnabledFeatures::SetNotificationConstructorEnabled(enable);
}

void WebRuntimeFeatures::EnableNotificationContentImage(bool enable) {
  RuntimeEnabledFeatures::SetNotificationContentImageEnabled(enable);
}

void WebRuntimeFeatures::EnableNotifications(bool enable) {
  RuntimeEnabledFeatures::SetNotificationsEnabled(enable);
}

void WebRuntimeFeatures::EnableNavigatorContentUtils(bool enable) {
  RuntimeEnabledFeatures::SetNavigatorContentUtilsEnabled(enable);
}

void WebRuntimeFeatures::EnableNetInfoDownlinkMax(bool enable) {
  RuntimeEnabledFeatures::SetNetInfoDownlinkMaxEnabled(enable);
}

void WebRuntimeFeatures::EnableNetworkService(bool enable) {
  RuntimeEnabledFeatures::SetNetworkServiceEnabled(enable);
}

void WebRuntimeFeatures::EnableOnDeviceChange(bool enable) {
  RuntimeEnabledFeatures::SetOnDeviceChangeEnabled(enable);
}

void WebRuntimeFeatures::EnableOrientationEvent(bool enable) {
  RuntimeEnabledFeatures::SetOrientationEventEnabled(enable);
}

void WebRuntimeFeatures::EnableOriginTrials(bool enable) {
  RuntimeEnabledFeatures::SetOriginTrialsEnabled(enable);
}

bool WebRuntimeFeatures::IsOriginTrialsEnabled() {
  return RuntimeEnabledFeatures::OriginTrialsEnabled();
}

void WebRuntimeFeatures::EnableOverflowIconsForMediaControls(bool enable) {
  RuntimeEnabledFeatures::SetOverflowIconsForMediaControlsEnabled(enable);
}

void WebRuntimeFeatures::EnablePagePopup(bool enable) {
  RuntimeEnabledFeatures::SetPagePopupEnabled(enable);
}

void WebRuntimeFeatures::EnableMiddleClickAutoscroll(bool enable) {
  RuntimeEnabledFeatures::SetMiddleClickAutoscrollEnabled(enable);
}

void WebRuntimeFeatures::EnablePassiveDocumentEventListeners(bool enable) {
  RuntimeEnabledFeatures::SetPassiveDocumentEventListenersEnabled(enable);
}

void WebRuntimeFeatures::EnablePaymentApp(bool enable) {
  RuntimeEnabledFeatures::SetPaymentAppEnabled(enable);
}

void WebRuntimeFeatures::EnablePaymentRequest(bool enable) {
  RuntimeEnabledFeatures::SetPaymentRequestEnabled(enable);
}

void WebRuntimeFeatures::EnablePermissionsAPI(bool enable) {
  RuntimeEnabledFeatures::SetPermissionsEnabled(enable);
}

void WebRuntimeFeatures::EnablePictureInPicture(bool enable) {
  RuntimeEnabledFeatures::SetPictureInPictureEnabled(enable);
}

void WebRuntimeFeatures::EnablePreloadDefaultIsMetadata(bool enable) {
  RuntimeEnabledFeatures::SetPreloadDefaultIsMetadataEnabled(enable);
}

void WebRuntimeFeatures::EnablePreloadImageSrcSetEnabled(bool enable) {
  RuntimeEnabledFeatures::SetPreloadImageSrcSetEnabled(enable);
}

void WebRuntimeFeatures::EnableRasterInducingScroll(bool enable) {
  RuntimeEnabledFeatures::SetRasterInducingScrollEnabled(enable);
}

void WebRuntimeFeatures::EnableScriptedSpeech(bool enable) {
  RuntimeEnabledFeatures::SetScriptedSpeechEnabled(enable);
}

void WebRuntimeFeatures::EnableSlimmingPaintV2(bool enable) {
  RuntimeEnabledFeatures::SetSlimmingPaintV2Enabled(enable);
}

void WebRuntimeFeatures::EnableUserActivationV2(bool enable) {
  RuntimeEnabledFeatures::SetUserActivationV2Enabled(enable);
}

void WebRuntimeFeatures::EnableTouchEventFeatureDetection(bool enable) {
  RuntimeEnabledFeatures::SetTouchEventFeatureDetectionEnabled(enable);
}

void WebRuntimeFeatures::EnableTouchpadAndWheelScrollLatching(bool enable) {
  RuntimeEnabledFeatures::SetTouchpadAndWheelScrollLatchingEnabled(enable);
}

void WebRuntimeFeatures::EnableWebGLDraftExtensions(bool enable) {
  RuntimeEnabledFeatures::SetWebGLDraftExtensionsEnabled(enable);
}

void WebRuntimeFeatures::EnableWebGLImageChromium(bool enable) {
  RuntimeEnabledFeatures::SetWebGLImageChromiumEnabled(enable);
}

void WebRuntimeFeatures::EnableXSLT(bool enable) {
  RuntimeEnabledFeatures::SetXSLTEnabled(enable);
}

void WebRuntimeFeatures::EnableOverlayScrollbars(bool enable) {
  RuntimeEnabledFeatures::SetOverlayScrollbarsEnabled(enable);
}

void WebRuntimeFeatures::ForceOverlayFullscreenVideo(bool enable) {
  RuntimeEnabledFeatures::SetForceOverlayFullscreenVideoEnabled(enable);
}

void WebRuntimeFeatures::EnableSharedArrayBuffer(bool enable) {
  RuntimeEnabledFeatures::SetSharedArrayBufferEnabled(enable);
}

void WebRuntimeFeatures::EnableSharedWorker(bool enable) {
  RuntimeEnabledFeatures::SetSharedWorkerEnabled(enable);
}

void WebRuntimeFeatures::EnablePreciseMemoryInfo(bool enable) {
  RuntimeEnabledFeatures::SetPreciseMemoryInfoEnabled(enable);
}

void WebRuntimeFeatures::EnablePrintBrowser(bool enable) {
  RuntimeEnabledFeatures::SetPrintBrowserEnabled(enable);
}

void WebRuntimeFeatures::EnableV8IdleTasks(bool enable) {
  RuntimeEnabledFeatures::SetV8IdleTasksEnabled(enable);
}

void WebRuntimeFeatures::EnableReducedReferrerGranularity(bool enable) {
  RuntimeEnabledFeatures::SetReducedReferrerGranularityEnabled(enable);
}

void WebRuntimeFeatures::EnablePushMessaging(bool enable) {
  RuntimeEnabledFeatures::SetPushMessagingEnabled(enable);
}

void WebRuntimeFeatures::EnableWebShare(bool enable) {
  RuntimeEnabledFeatures::SetWebShareEnabled(enable);
}

void WebRuntimeFeatures::EnableWebVR(bool enable) {
  RuntimeEnabledFeatures::SetWebVREnabled(enable);
}

void WebRuntimeFeatures::EnableWebXR(bool enable) {
  RuntimeEnabledFeatures::SetWebXREnabled(enable);
}

void WebRuntimeFeatures::EnableWebXRGamepadSupport(bool enable) {
  RuntimeEnabledFeatures::SetWebXRGamepadSupportEnabled(enable);
}

void WebRuntimeFeatures::EnableWebXRHitTest(bool enable) {
  RuntimeEnabledFeatures::SetWebXRHitTestEnabled(enable);
}

void WebRuntimeFeatures::EnablePresentationAPI(bool enable) {
  RuntimeEnabledFeatures::SetPresentationEnabled(enable);
}

void WebRuntimeFeatures::EnableRenderingPipelineThrottling(bool enable) {
  RuntimeEnabledFeatures::SetRenderingPipelineThrottlingEnabled(enable);
}

void WebRuntimeFeatures::EnableRequireCSSExtensionForFile(bool enable) {
  RuntimeEnabledFeatures::SetRequireCSSExtensionForFileEnabled(enable);
}

void WebRuntimeFeatures::EnableResourceLoadScheduler(bool enable) {
  RuntimeEnabledFeatures::SetResourceLoadSchedulerEnabled(enable);
}

void WebRuntimeFeatures::EnableExpensiveBackgroundTimerThrottling(bool enable) {
  RuntimeEnabledFeatures::SetExpensiveBackgroundTimerThrottlingEnabled(enable);
}

void WebRuntimeFeatures::EnableScrollAnchorSerialization(bool enable) {
  RuntimeEnabledFeatures::SetScrollAnchorSerializationEnabled(enable);
}

void WebRuntimeFeatures::EnableSecMetadata(bool enable) {
  RuntimeEnabledFeatures::SetSecMetadataEnabled(enable);
}

void WebRuntimeFeatures::EnableServiceWorkerScriptFullCodeCache(bool enable) {
  RuntimeEnabledFeatures::SetServiceWorkerScriptFullCodeCacheEnabled(enable);
}

void WebRuntimeFeatures::EnableAutoplayMutedVideos(bool enable) {
  RuntimeEnabledFeatures::SetAutoplayMutedVideosEnabled(enable);
}

void WebRuntimeFeatures::EnableTimerThrottlingForBackgroundTabs(bool enable) {
  RuntimeEnabledFeatures::SetTimerThrottlingForBackgroundTabsEnabled(enable);
}

void WebRuntimeFeatures::EnableTimerThrottlingForHiddenFrames(bool enable) {
  RuntimeEnabledFeatures::SetTimerThrottlingForHiddenFramesEnabled(enable);
}

void WebRuntimeFeatures::EnableSendBeaconThrowForBlobWithNonSimpleType(
    bool enable) {
  RuntimeEnabledFeatures::SetSendBeaconThrowForBlobWithNonSimpleTypeEnabled(
      enable);
}

void WebRuntimeFeatures::EnableBackgroundVideoTrackOptimization(bool enable) {
  RuntimeEnabledFeatures::SetBackgroundVideoTrackOptimizationEnabled(enable);
}

void WebRuntimeFeatures::EnableNewRemotePlaybackPipeline(bool enable) {
  RuntimeEnabledFeatures::SetNewRemotePlaybackPipelineEnabled(enable);
}

void WebRuntimeFeatures::EnableRemotePlaybackAPI(bool enable) {
  RuntimeEnabledFeatures::SetRemotePlaybackEnabled(enable);
}

void WebRuntimeFeatures::EnableVideoFullscreenOrientationLock(bool enable) {
  RuntimeEnabledFeatures::SetVideoFullscreenOrientationLockEnabled(enable);
}

void WebRuntimeFeatures::EnableVideoRotateToFullscreen(bool enable) {
  RuntimeEnabledFeatures::SetVideoRotateToFullscreenEnabled(enable);
}

void WebRuntimeFeatures::EnableVideoFullscreenDetection(bool enable) {
  RuntimeEnabledFeatures::SetVideoFullscreenDetectionEnabled(enable);
}

void WebRuntimeFeatures::EnableMediaControlsOverlayPlayButton(bool enable) {
  RuntimeEnabledFeatures::SetMediaControlsOverlayPlayButtonEnabled(enable);
}

void WebRuntimeFeatures::EnableRemotePlaybackBackend(bool enable) {
  RuntimeEnabledFeatures::SetRemotePlaybackBackendEnabled(enable);
}

void WebRuntimeFeatures::EnableMediaCastOverlayButton(bool enable) {
  RuntimeEnabledFeatures::SetMediaCastOverlayButtonEnabled(enable);
}

void WebRuntimeFeatures::EnableWebAuth(bool enable) {
  RuntimeEnabledFeatures::SetWebAuthEnabled(enable);
}

void WebRuntimeFeatures::EnableClientPlaceholdersForServerLoFi(bool enable) {
  RuntimeEnabledFeatures::SetClientPlaceholdersForServerLoFiEnabled(enable);
}

void WebRuntimeFeatures::EnableLazyInitializeMediaControls(bool enable) {
  RuntimeEnabledFeatures::SetLazyInitializeMediaControlsEnabled(enable);
}

void WebRuntimeFeatures::EnableClientHintsPersistent(bool enable) {
  RuntimeEnabledFeatures::SetClientHintsPersistentEnabled(enable);
}

void WebRuntimeFeatures::EnableMediaEngagementBypassAutoplayPolicies(
    bool enable) {
  RuntimeEnabledFeatures::SetMediaEngagementBypassAutoplayPoliciesEnabled(
      enable);
}

void WebRuntimeFeatures::EnableV8ContextSnapshot(bool enable) {
  RuntimeEnabledFeatures::SetV8ContextSnapshotEnabled(enable);
}

void WebRuntimeFeatures::EnableAutomationControlled(bool enable) {
  RuntimeEnabledFeatures::SetAutomationControlledEnabled(enable);
}

void WebRuntimeFeatures::EnableWorkStealingInScriptRunner(bool enable) {
  RuntimeEnabledFeatures::SetWorkStealingInScriptRunnerEnabled(enable);
}

void WebRuntimeFeatures::EnableStopInBackground(bool enable) {
  RuntimeEnabledFeatures::SetStopInBackgroundEnabled(enable);
}

void WebRuntimeFeatures::EnableStopLoadingInBackground(bool enable) {
  RuntimeEnabledFeatures::SetStopLoadingInBackgroundEnabled(enable);
}

void WebRuntimeFeatures::EnableStopNonTimersInBackground(bool enable) {
  RuntimeEnabledFeatures::SetStopNonTimersInBackgroundEnabled(enable);
}

void WebRuntimeFeatures::EnablePWAFullCodeCache(bool enable) {
  RuntimeEnabledFeatures::SetPWAFullCodeCacheEnabled(enable);
}

void WebRuntimeFeatures::EnableMojoBlobURLs(bool enable) {
  RuntimeEnabledFeatures::SetMojoBlobURLsEnabled(enable);
}

void WebRuntimeFeatures::EnableOffMainThreadWebSocket(bool enable) {
  RuntimeEnabledFeatures::SetOffMainThreadWebSocketEnabled(enable);
}

void WebRuntimeFeatures::EnableExperimentalProductivityFeatures(bool enable) {
  RuntimeEnabledFeatures::SetExperimentalProductivityFeaturesEnabled(enable);
}

void WebRuntimeFeatures::EnableDisplayCutoutViewportFit(bool enable) {
  RuntimeEnabledFeatures::SetDisplayCutoutViewportFitEnabled(enable);
}

void WebRuntimeFeatures::EnableAutoplayIgnoresWebAudio(bool enable) {
  RuntimeEnabledFeatures::SetAutoplayIgnoresWebAudioEnabled(enable);
}

}  // namespace blink
