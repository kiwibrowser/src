// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/content_features.h"
#include "build/build_config.h"

namespace features {

// All features in alphabetical order.

// Enables the allowActivationDelegation attribute on iframes.
// https://www.chromestatus.com/features/6025124331388928
const base::Feature kAllowActivationDelegationAttr{
    "AllowActivationDelegationAttr", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables content-initiated, main frame navigations to data URLs.
// TODO(meacer): Remove when the deprecation is complete.
//               https://www.chromestatus.com/feature/5669602927312896
const base::Feature kAllowContentInitiatedDataUrlNavigations{
    "AllowContentInitiatedDataUrlNavigations",
    base::FEATURE_DISABLED_BY_DEFAULT};

// Enables asm.js to WebAssembly V8 backend.
// http://asmjs.org/spec/latest/
const base::Feature kAsmJsToWebAssembly{"AsmJsToWebAssembly",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

// Enables async wheel events. Note that this feature depends on
// TouchpadAndWheelScrollLatching and enabling it when latching is disabled
// won't have any impacts.
const base::Feature kAsyncWheelEvents{"AsyncWheelEvents",
                                      base::FEATURE_ENABLED_BY_DEFAULT};

// Creates audio output and input streams using the audio service.
const base::Feature kAudioServiceAudioStreams{
    "AudioServiceAudioStreams", base::FEATURE_DISABLED_BY_DEFAULT};

// Runs the audio service in a separate process.
const base::Feature kAudioServiceOutOfProcess{
    "AudioServiceOutOfProcess", base::FEATURE_DISABLED_BY_DEFAULT};

// Allows swipe left/right from touchpad change browser navigation. Currently
// only enabled by default on CrOS.
const base::Feature kTouchpadOverscrollHistoryNavigation {
  "TouchpadOverscrollHistoryNavigation",
#if defined(OS_CHROMEOS)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

// Block subresource requests whose URLs contain embedded credentials (e.g.
// `https://user:pass@example.com/resource`).
const base::Feature kBlockCredentialedSubresources{
    "BlockCredentialedSubresources", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables brotli "Accept-Encoding" advertising and "Content-Encoding" support.
// Brotli format specification: http://www.ietf.org/id/draft-alakuijala-brotli
const base::Feature kBrotliEncoding{"brotli-encoding",
                                    base::FEATURE_ENABLED_BY_DEFAULT};

// Enables code caching for inline scripts.
const base::Feature kCacheInlineScriptCode{"CacheInlineScriptCode",
                                           base::FEATURE_DISABLED_BY_DEFAULT};

// If Canvas2D Image Chromium is allowed, this feature controls whether it is
// enabled.
const base::Feature kCanvas2DImageChromium {
  "Canvas2DImageChromium",
#if defined(OS_MACOSX) || defined(OS_CHROMEOS)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

// Enables the compositing of fixed position content that is opaque and can
// preserve LCD text.
const base::Feature kCompositeOpaqueFixedPosition{
    "CompositeOpaqueFixedPosition", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables the compositing of scrolling content that supports painting the
// background with the foreground, such that LCD text will still be enabled.
const base::Feature kCompositeOpaqueScrollers{"CompositeOpaqueScrollers",
                                              base::FEATURE_ENABLED_BY_DEFAULT};

// Enables handling touch events in compositor using impl side touch action
// knowledge.
const base::Feature kCompositorTouchAction{"CompositorTouchAction",
                                           base::FEATURE_DISABLED_BY_DEFAULT};

// Enables blocking cross-site document responses (not paying attention to
// whether a site isolation mode is also enabled).
const base::Feature kCrossSiteDocumentBlockingAlways{
    "CrossSiteDocumentBlockingAlways", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables blocking cross-site document responses if one of site isolation modes
// is (e.g. site-per-process or isolate-origins) is enabled.
const base::Feature kCrossSiteDocumentBlockingIfIsolating{
    "CrossSiteDocumentBlockingIfIsolating", base::FEATURE_ENABLED_BY_DEFAULT};

// Puts save-data header in the holdback mode. This disables sending of
// save-data header to origins, and to the renderer processes within Chrome.
const base::Feature kDataSaverHoldback{"DataSaverHoldback",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

// Throttle tasks in Blink background timer queues based on CPU budgets
// for the background tab. Bug: https://crbug.com/639852.
const base::Feature kExpensiveBackgroundTimerThrottling{
    "ExpensiveBackgroundTimerThrottling", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables exposing back/forward mouse buttons to the renderer and the web.
const base::Feature kExtendedMouseButtons{"ExtendedMouseButtons",
                                          base::FEATURE_ENABLED_BY_DEFAULT};

// Enables a blink::FontCache optimization that reuses a font to serve different
// size of font.
const base::Feature kFontCacheScaling{"FontCacheScaling",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

// Enables a security restriction on iframes navigating their top frame.
// When enabled, the navigation will only be permitted if the iframe is
// same-origin to the top frame, or if a user gesture is being processed.
const base::Feature kFramebustingNeedsSameOriginOrUserGesture{
    "FramebustingNeedsSameOriginOrUserGesture",
    base::FEATURE_ENABLED_BY_DEFAULT};

// Enables extended Gamepad API features like motion tracking and haptics.
const base::Feature kGamepadExtensions{"GamepadExtensions",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

// Enables haptic vibration effects on supported gamepads.
const base::Feature kGamepadVibration{"GamepadVibration",
                                      base::FEATURE_ENABLED_BY_DEFAULT};

// When WebXR Device API is enabled, exposes VR controllers as Gamepads and
// enables additional Gamepad attributes for use with WebXR Device API. Each
// XRInputSource will have a corresponding Gamepad instance.
const base::Feature kWebXrGamepadSupport{"WebXRGamepadSupport",
                                         base::FEATURE_DISABLED_BY_DEFAULT};

// Causes the implementations of guests (inner WebContents) to use
// out-of-process iframes.
const base::Feature kGuestViewCrossProcessFrames{
    "GuestViewCrossProcessFrames", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables BlinkGC heap compaction.
const base::Feature kHeapCompaction{"HeapCompaction",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

// Enables/disables the Image Capture API.
const base::Feature kImageCaptureAPI{"ImageCaptureAPI",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

// Whether to use GeometryMapper to optimize IntersectionObserver.
const base::Feature kIntersectionObserverGeometryMapper{
    "IntersectionObserverGeometryMapper", base::FEATURE_DISABLED_BY_DEFAULT};

// Alternative to switches::kIsolateOrigins, for turning on origin isolation.
// List of origins to isolate has to be specified via
// kIsolateOriginsFieldTrialParamName.
const base::Feature kIsolateOrigins{"IsolateOrigins",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const char kIsolateOriginsFieldTrialParamName[] = "OriginsList";

// Enables an API which allows websites to capture reserved keys in fullscreen.
// Defined by w3c here: https://w3c.github.io/keyboard-lock/
const base::Feature kKeyboardLockAPI{"KeyboardLockAPI",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kLayeredAPI{"LayeredAPI",
                                base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kLazyFrameLoading{"LazyFrameLoading",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

// Enable lazy initialization of the media controls.
const base::Feature kLazyInitializeMediaControls{
    "LazyInitializeMediaControls", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables lazily parsing css properties for performance.
const base::Feature kLazyParseCSS{"LazyParseCSS",
                                  base::FEATURE_ENABLED_BY_DEFAULT};

// Enables lowering the priority of the resources in iframes.
const base::Feature kLowPriorityIframes{"LowPriorityIframes",
                                        base::FEATURE_DISABLED_BY_DEFAULT};

// If this feature is enabled, media-device enumerations use a cache that is
// invalidated upon notifications sent by base::SystemMonitor. If disabled, the
// cache is considered invalid on every enumeration request.
const base::Feature kMediaDevicesSystemMonitorCache {
  "MediaDevicesSystemMonitorCaching",
#if defined(OS_MACOSX) || defined(OS_WIN)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

// Enables the memory coordinator.
// WARNING:
// The memory coordinator is not ready for use and enabling this may cause
// unexpected memory regression at this point. Please do not enable this.
const base::Feature kMemoryCoordinator{"MemoryCoordinator",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

// ES6 Modules dynamic imports.
const base::Feature kModuleScriptsDynamicImport{
    "ModuleScriptsDynamicImport", base::FEATURE_ENABLED_BY_DEFAULT};

// ES6 Modules import.meta.url.
const base::Feature kModuleScriptsImportMetaUrl{
    "ModuleScriptsImportMetaUrl", base::FEATURE_ENABLED_BY_DEFAULT};

// Mojo-based Session Storage.
const base::Feature kMojoSessionStorage{"MojoSessionStorage",
                                        base::FEATURE_DISABLED_BY_DEFAULT};

// Enables/disables the video capture service.
const base::Feature kMojoVideoCapture {
  "MojoVideoCapture",
#if defined(OS_MACOSX) || defined(OS_WIN)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

// If the network service is enabled, runs it in process.
const base::Feature kNetworkServiceInProcess{"NetworkServiceInProcess",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

// Kill switch for Web Notification content images.
const base::Feature kNotificationContentImage{"NotificationContentImage",
                                              base::FEATURE_ENABLED_BY_DEFAULT};

// Off-main-thread WebSocket. See https://crbug.com/825740
const base::Feature kOffMainThreadWebSocket{"OffMainThreadWebSocket",
                                            base::FEATURE_ENABLED_BY_DEFAULT};

// Origin Manifest. See crbug.com/751996
const base::Feature kOriginManifest{"OriginManifest",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

// Origin Trials for controlling access to feature/API experiments.
const base::Feature kOriginTrials{"OriginTrials",
                                  base::FEATURE_ENABLED_BY_DEFAULT};

// Whether document level event listeners should default 'passive' to true.
const base::Feature kPassiveDocumentEventListeners{
    "PassiveDocumentEventListeners", base::FEATURE_ENABLED_BY_DEFAULT};

// Whether we should force a touchstart and first touchmove per scroll event
// listeners to be non-blocking during fling.
const base::Feature kPassiveEventListenersDueToFling{
    "PassiveEventListenersDueToFling", base::FEATURE_ENABLED_BY_DEFAULT};

// Whether PDF files should be rendered in diffent processes based on origin.
const base::Feature kPdfIsolation = {"PdfIsolation",
                                     base::FEATURE_DISABLED_BY_DEFAULT};

// Whether we should use the navigation_client mojo interface for navigations.
const base::Feature kPerNavigationMojoInterface = {
    "PerNavigationMojoInterface", base::FEATURE_DISABLED_BY_DEFAULT};

// If Pepper 3D Image Chromium is allowed, this feature controls whether it is
// enabled.
const base::Feature kPepper3DImageChromium {
  "Pepper3DImageChromium",
#if defined(OS_MACOSX)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

// Enables Purge+Throttle on platforms except Android and MacOS.
// (Android) Purge+Throttle depends on TabManager, but TabManager doesn't
// support Android. Enable after Android is supported.
// (MacOS X) Enable after Purge+Throttle handles memory pressure signals
// send by OS correctly.
const base::Feature kPurgeAndSuspend {
  "PurgeAndSuspend",
#if defined(OS_MACOSX) || defined(OS_ANDROID)
      base::FEATURE_DISABLED_BY_DEFAULT
#else
      base::FEATURE_ENABLED_BY_DEFAULT
#endif
};

// Generate V8 full code cache for PWAs.
const base::Feature kPWAFullCodeCache{"PWAFullCodeCache",
                                      base::FEATURE_ENABLED_BY_DEFAULT};

// Enable raster-inducing scroll.
const base::Feature kRasterInducingScroll{"RasterInducingScroll",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

// Throttle Blink's rendering pipeline based on frame visibility.
const base::Feature kRenderingPipelineThrottling{
    "RenderingPipelineThrottling", base::FEATURE_ENABLED_BY_DEFAULT};

// When loading CSS from a 'file:' URL, require a CSS-like file extension.
const base::Feature kRequireCSSExtensionForFile{
    "RequireCSSExtensionForFile", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables resampling input events on main thread.
const base::Feature kResamplingInputEvents{"ResamplingInputEvents",
                                           base::FEATURE_DISABLED_BY_DEFAULT};

// Loading Dispatcher v0 support with ResourceLoadScheduler (crbug.com/729954).
const base::Feature kResourceLoadScheduler{"ResourceLoadScheduler",
                                           base::FEATURE_DISABLED_BY_DEFAULT};

// Run video capture service in the Browser process as opposed to a dedicated
// utility process
const base::Feature kRunVideoCaptureServiceInBrowserProcess{
    "RunVideoCaptureServiceInBrowserProcess",
    base::FEATURE_DISABLED_BY_DEFAULT};

// Save the scroll anchor and use it to restore scroll position.
const base::Feature kScrollAnchorSerialization{
    "ScrollAnchorSerialization", base::FEATURE_DISABLED_BY_DEFAULT};

// Make sendBeacon throw for a Blob with a non simple type.
const base::Feature kSendBeaconThrowForBlobWithNonSimpleType{
    "SendBeaconThrowForBlobWithNonSimpleType",
    base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kSecMetadata{"SecMetadata",
                                 base::FEATURE_DISABLED_BY_DEFAULT};

// Service worker based payment apps as defined by w3c here:
// https://w3c.github.io/webpayments-payment-apps-api/
const base::Feature kServiceWorkerPaymentApps{
    "ServiceWorkerPaymentApps", base::FEATURE_DISABLED_BY_DEFAULT};

// Generate V8 full code cache of service worker scripts.
const base::Feature kServiceWorkerScriptFullCodeCache{
    "ServiceWorkerScriptFullCodeCache", base::FEATURE_ENABLED_BY_DEFAULT};

// Establish direct connection from clients to the service worker.
const base::Feature kServiceWorkerServicification{
    "ServiceWorkerServicification", base::FEATURE_DISABLED_BY_DEFAULT};

// http://tc39.github.io/ecmascript_sharedmem/shmem.html
const base::Feature kSharedArrayBuffer{"SharedArrayBuffer",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

// Origin-Signed HTTP Exchanges (for WebPackage Loading)
// https://www.chromestatus.com/features/5745285984681984
const base::Feature kSignedHTTPExchange{"SignedHTTPExchange",
                                        base::FEATURE_DISABLED_BY_DEFAULT};

// Origin Trial of Origin-Signed HTTP Exchanges (for WebPackage Loading)
const base::Feature kSignedHTTPExchangeOriginTrial{
    "SignedHTTPExchangeOriginTrial", base::FEATURE_DISABLED_BY_DEFAULT};

// An experiment to require process isolation for the sign-in origin,
// https://accounts.google.com.  Launch bug: https://crbug.com/739418.
const base::Feature kSignInProcessIsolation{"sign-in-process-isolation",
                                            base::FEATURE_ENABLED_BY_DEFAULT};

// Slimming Paint V1.75. See http://crbug.com/771643.
const base::Feature kSlimmingPaintV175{"SlimmingPaintV175",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether SpareRenderProcessHostManager tries to always have a warm
// spare renderer process around for the most recently requested BrowserContext.
// This feature is only consulted in site-per-process mode.
const base::Feature kSpareRendererForSitePerProcess{
    "SpareRendererForSitePerProcess", base::FEATURE_ENABLED_BY_DEFAULT};

// Stop scheduler task queues in background after allowed grace time.
const base::Feature kStopInBackground {
  "stop-in-background",
#if defined(OS_ANDROID)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

// Stop loading tasks and loading of resources in background, on Android,
// after allowed grace time. Launch bug: https://crbug.com/775761.
const base::Feature kStopLoadingInBackground{"stop-loading-in-background",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

// Stop non-timer task queues in background, on Android,
// after allowed grace time. Launch bug: https://crbug.com/822954.
const base::Feature kStopNonTimersInBackground{
    "stop-non-timers-in-background", base::FEATURE_DISABLED_BY_DEFAULT};

// Throttle Blink timers in out-of-view cross origin frames.
const base::Feature kTimerThrottlingForHiddenFrames{
    "TimerThrottlingForHiddenFrames", base::FEATURE_ENABLED_BY_DEFAULT};

// Groups all out-of-process iframes to a different process from the process of
// the top document. This is a performance isolation mode.  Launch bug:
// https://crbug.com/595987.
const base::Feature kTopDocumentIsolation{"top-document-isolation",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

// Enables touchpad and wheel scroll latching.
const base::Feature kTouchpadAndWheelScrollLatching{
    "TouchpadAndWheelScrollLatching", base::FEATURE_ENABLED_BY_DEFAULT};

// Use Feature Policy to gate the use of permission features like midi,
// geolocation, camera, microphone, etc.
const base::Feature kUseFeaturePolicyForPermissions{
    "UseFeaturePolicyForPermissions", base::FEATURE_ENABLED_BY_DEFAULT};

// An experimental simple user-activation model where the user gesture state is
// tracked through a frame-based state instead of the gesture tokens we use
// today.
const base::Feature kUserActivationV2{"UserActivationV2",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

// Use RenderWidgetHostView::CreateVideoCapturer instead of
// RenderWidgetHostView::CopyFromSurface to obtain a stream of snapshots
// captured from the renderer for DevTools performance timeline and eyedropper
// tool.
const base::Feature kUseVideoCaptureApiForDevToolsSnapshots{
    "UseVideoCaptureApiForDevToolsSnapshots", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables to use a snapshot file in creating V8 contexts.
const base::Feature kV8ContextSnapshot{"V8ContextSnapshot",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

// Enables future V8 VM features
const base::Feature kV8VmFuture{"V8VmFuture",
                                base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether editing web input fields is enabled in VR.
const base::Feature kVrWebInputEditing{"VrWebInputEditing",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

// Enable WebAssembly structured cloning.
// http://webassembly.org/
const base::Feature kWebAssembly{"WebAssembly",
                                 base::FEATURE_DISABLED_BY_DEFAULT};

// Enable WebAssembly streamed compilation.
const base::Feature kWebAssemblyStreaming{"WebAssemblyStreaming",
                                          base::FEATURE_ENABLED_BY_DEFAULT};

// Enable WebAssembly baseline compilation and tier up.
const base::Feature kWebAssemblyBaseline{"WebAssemblyBaseline",
                                         base::FEATURE_DISABLED_BY_DEFAULT};

// Enable WebAssembly trap handler.
const base::Feature kWebAssemblyTrapHandler{"WebAssemblyTrapHandler",
                                            base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether the visibility of a WebContents can be OCCLUDED. When
// disabled, an occluded WebContents behaves exactly like a VISIBLE WebContents.
const base::Feature kWebContentsOcclusion {
  "WebContentsOcclusion",
#if defined(OS_MACOSX) || defined(OS_CHROMEOS)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

// Controls whether the WebAuthentication API is enabled:
// https://w3c.github.io/webauthn
const base::Feature kWebAuth {
  "WebAuthentication",
#if defined(OS_ANDROID)
      base::FEATURE_DISABLED_BY_DEFAULT
#else
      base::FEATURE_ENABLED_BY_DEFAULT
#endif
};

// Controls whether BLE authenticators can be used via the WebAuthentication
// API. https://w3c.github.io/webauthn
const base::Feature kWebAuthBle{"WebAuthenticationBle",
                                base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether CTAP2 authenticators can be used via the WebAuthentication
// API. https://w3c.github.io/webauthn
const base::Feature kWebAuthCtap2{"WebAuthenticationCtap2",
                                  base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether CTAP2 devices can communicate via the WebAuthentication API
// using pairingless BLE protocol.
// https://w3c.github.io/webauthn
const base::Feature kWebAuthCable{"WebAuthenticationCable",
                                  base::FEATURE_DISABLED_BY_DEFAULT};

// If WebGL Image Chromium is allowed, this feature controls whether it is
// enabled.
const base::Feature kWebGLImageChromium{"WebGLImageChromium",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

// Enable experimental policy-controlled features and LAPIs
const base::Feature kExperimentalProductivityFeatures{
    "ExperimentalProductivityFeatures", base::FEATURE_DISABLED_BY_DEFAULT};

// The JavaScript API for payments on the web.
const base::Feature kWebPayments{"WebPayments",
                                 base::FEATURE_ENABLED_BY_DEFAULT};

// Informs the WebRTC Acoustic Echo Canceler (AEC) that echo path loss is
// bounded.
const base::Feature kWebRtcAecBoundedErlSetup{
    "WebRtcAecBoundedErlSetup", base::FEATURE_DISABLED_BY_DEFAULT};

// Informs the WebRTC Acoustic Echo Canceler (AEC) that the playout and
// capture is done using different clocks.
const base::Feature kWebRtcAecClockDriftSetup{
    "WebRtcAecClockDriftSetup", base::FEATURE_DISABLED_BY_DEFAULT};

// Informs the WebRTC Acoustic Echo Canceler (AEC) that the feature providing
// echo canceller transparency to render noise should be used.
const base::Feature kWebRtcAecNoiseTransparency{
    "WebRtcAecNoiseTransparency", base::FEATURE_DISABLED_BY_DEFAULT};

// Makes WebRTC use ECDSA certs by default (i.e., when no cert type was
// specified in JS).
const base::Feature kWebRtcEcdsaDefault{"WebRTC-EnableWebRtcEcdsa",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

// Enables HW H264 encoding on Android.
const base::Feature kWebRtcHWH264Encoding{"WebRtcHWH264Encoding",
                                          base::FEATURE_ENABLED_BY_DEFAULT};

// Enables HW VP8 encoding on Android.
const base::Feature kWebRtcHWVP8Encoding {
  "WebRtcHWVP8Encoding",
#if defined(OS_ANDROID)
      base::FEATURE_DISABLED_BY_DEFAULT
#else
      base::FEATURE_ENABLED_BY_DEFAULT
#endif
};

// Enables HW VP8 encoding using Intel Vaapi, if available.
// TODO(crbug.com/794608): Enable by default when we trust quality.
const base::Feature kWebRtcVaapiHWVP8Encoding{
    "WebRtcVaapiHWVP8Encoding", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables negotiation of experimental multiplex codec in SDP.
const base::Feature kWebRtcMultiplexCodec{"WebRTC-MultiplexCodec",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

// Fallback from hardware encoder (if available) to software, for WebRTC
// screensharing that uses temporal scalability.
const base::Feature kWebRtcScreenshareSwEncoding{
    "WebRtcScreenshareSwEncoding", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables the WebRTC Echo Canceller version 3 (AEC3). Feature for
// http://crbug.com/688388. This value is sent to WebRTC's echo canceller to
// toggle which echo canceller should be used.
const base::Feature kWebRtcUseEchoCanceller3{"WebRtcUseEchoCanceller3",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

// Use GpuMemoryBuffer backed VideoFrames in media streams.
const base::Feature kWebRtcUseGpuMemoryBufferVideoFrames{
    "WebRTC-UseGpuMemoryBufferVideoFrames", base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether the WebUSB API is enabled:
// https://wicg.github.io/webusb
const base::Feature kWebUsb{"WebUSB", base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether WebVR VSync-aligned render loop timing is enabled.
const base::Feature kWebVrVsyncAlign{"WebVrVsyncAlign",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether the WebXR Device API is enabled.
const base::Feature kWebXr{"WebXR", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables access to raycasting against estimated XR scene geometry.
const base::Feature kWebXrHitTest{"WebXRHitTest",
                                  base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether the orientation sensor based device is enabled.
const base::Feature kWebXrOrientationSensorDevice{
    "WebXROrientationSensorDevice", base::FEATURE_DISABLED_BY_DEFAULT};

// Wipe corrupt v2 IndexedDB databases.
const base::Feature kWipeCorruptV2IDBDatabases{
    "WipeCorruptV2IDBDatabases", base::FEATURE_ENABLED_BY_DEFAULT};

// Enabled "work stealing" in the script runner.
const base::Feature kWorkStealingInScriptRunner{
    "WorkStealingInScriptRunner", base::FEATURE_DISABLED_BY_DEFAULT};

#if defined(OS_ANDROID)
// Autofill Accessibility in Android.
// crbug.com/627860
const base::Feature kAndroidAutofillAccessibility{
    "AndroidAutofillAccessibility", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables developers to use the CSS safe-area-* and viewport-fit APIs which
// allow them to support devices with a display cutout.
const base::Feature kDisplayCutoutAPI{"DisplayCutoutAPI",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

// Enables hiding incorrectly-sized frames while in fullscreen.
const base::Feature kHideIncorrectlySizedFullscreenFrames{
    "HideIncorrectlySizedFullscreenFrames", base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether the WebNFC API is enabled:
// https://w3c.github.io/web-nfc/
const base::Feature kWebNfc{"WebNFC", base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether an override for the WebXR presentation render path is
// enabled. The param value specifies the requested specific render path. This
// is combined with a runtime capability check, the option is ignored if the
// requested render path is unsupported.
const base::Feature kWebXrRenderPath{"WebXrRenderPath",
                                     base::FEATURE_DISABLED_BY_DEFAULT};
const char kWebXrRenderPathParamName[] = "RenderPath";
const char kWebXrRenderPathParamValueClientWait[] = "ClientWait";
const char kWebXrRenderPathParamValueGpuFence[] = "GpuFence";
const char kWebXrRenderPathParamValueSharedBuffer[] = "SharedBuffer";
#endif  // defined(OS_ANDROID)

#if defined(OS_MACOSX)
// Enables caching of media devices for the purpose of enumerating them.
const base::Feature kDeviceMonitorMac{"DeviceMonitorMac",
                                      base::FEATURE_ENABLED_BY_DEFAULT};

// Enable IOSurface based screen capturer.
const base::Feature kIOSurfaceCapturer{"IOSurfaceCapturer",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

// The V2 sandbox on MacOS removes the unsandboed warmup phase and sandboxes the
// entire life of the process.
const base::Feature kMacV2Sandbox{"MacV2Sandbox",
                                  base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether the CTAP2 implementation should use a built-in platform
// authenticator, where available.
const base::Feature kWebAuthTouchId{"WebAuthenticationTouchId",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
#endif  // defined(OS_MACOSX)

bool IsVideoCaptureServiceEnabledForOutOfProcess() {
#if defined(OS_ANDROID)
  return false;
#else
  return base::FeatureList::IsEnabled(features::kMojoVideoCapture) &&
         !base::FeatureList::IsEnabled(
             features::kRunVideoCaptureServiceInBrowserProcess);
#endif
}

bool IsVideoCaptureServiceEnabledForBrowserProcess() {
#if defined(OS_ANDROID)
  return base::FeatureList::IsEnabled(features::kMojoVideoCapture);
#else
  return base::FeatureList::IsEnabled(features::kMojoVideoCapture) &&
         base::FeatureList::IsEnabled(
             features::kRunVideoCaptureServiceInBrowserProcess);
#endif
}

}  // namespace features
