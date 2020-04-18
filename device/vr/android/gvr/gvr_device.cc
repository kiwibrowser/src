// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/android/gvr/gvr_device.h"

#include <math.h>
#include <algorithm>
#include <utility>

#include "base/android/android_hardware_buffer_compat.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "device/vr/android/gvr/gvr_delegate.h"
#include "device/vr/android/gvr/gvr_delegate_provider.h"
#include "device/vr/android/gvr/gvr_delegate_provider_factory.h"
#include "device/vr/android/gvr/gvr_device_provider.h"
#include "device/vr/vr_display_impl.h"
#include "jni/NonPresentingGvrContext_jni.h"
#include "third_party/gvr-android-sdk/src/libraries/headers/vr/gvr/capi/include/gvr.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/transform.h"
#include "ui/gfx/transform_util.h"

using base::android::JavaRef;

namespace device {

namespace {

// Default downscale factor for computing the recommended WebVR/WebXR
// renderWidth/Height from the 1:1 pixel mapped size. Using a rather
// aggressive downscale due to the high overhead of copying pixels
// twice before handing off to GVR. For comparison, the polyfill
// uses approximately 0.55 on a Pixel XL.
static constexpr float kWebVrRecommendedResolutionScale = 0.5;
static constexpr float kWebXrRecommendedResolutionScale = 0.7;

// The scale factor for WebXR on devices that don't have shared buffer
// support. (Android N and earlier.)
static constexpr float kWebXrNoSharedBufferResolutionScale = 0.5;

gfx::Size GetMaximumWebVrSize(gvr::GvrApi* gvr_api) {
  // Get the default, unscaled size for the WebVR transfer surface
  // based on the optimal 1:1 render resolution. A scalar will be applied to
  // this value in the renderer to reduce the render load. This size will also
  // be reported to the client via CreateVRDisplayInfo as the
  // client-recommended renderWidth/renderHeight and for the GVR
  // framebuffer. If the client chooses a different size or resizes it
  // while presenting, we'll resize the transfer surface and GVR
  // framebuffer to match.
  gvr::Sizei render_target_size =
      gvr_api->GetMaximumEffectiveRenderTargetSize();

  gfx::Size webvr_size(render_target_size.width, render_target_size.height);

  // Ensure that the width is an even number so that the eyes each
  // get the same size, the recommended renderWidth is per eye
  // and the client will use the sum of the left and right width.
  //
  // TODO(klausw,crbug.com/699350): should we round the recommended
  // size to a multiple of 2^N pixels to be friendlier to the GPU? The
  // exact size doesn't matter, and it might be more efficient.
  webvr_size.set_width(webvr_size.width() & ~1);
  return webvr_size;
}

mojom::VREyeParametersPtr CreateEyeParamater(
    gvr::GvrApi* gvr_api,
    gvr::Eye eye,
    const gvr::BufferViewportList& buffers,
    const gfx::Size& maximum_size) {
  mojom::VREyeParametersPtr eye_params = mojom::VREyeParameters::New();
  eye_params->fieldOfView = mojom::VRFieldOfView::New();
  eye_params->offset.resize(3);
  eye_params->renderWidth = maximum_size.width() / 2;
  eye_params->renderHeight = maximum_size.height();

  gvr::BufferViewport eye_viewport = gvr_api->CreateBufferViewport();
  buffers.GetBufferViewport(eye, &eye_viewport);
  gvr::Rectf eye_fov = eye_viewport.GetSourceFov();
  eye_params->fieldOfView->upDegrees = eye_fov.top;
  eye_params->fieldOfView->downDegrees = eye_fov.bottom;
  eye_params->fieldOfView->leftDegrees = eye_fov.left;
  eye_params->fieldOfView->rightDegrees = eye_fov.right;

  gvr::Mat4f eye_mat = gvr_api->GetEyeFromHeadMatrix(eye);
  eye_params->offset[0] = -eye_mat.m[0][3];
  eye_params->offset[1] = -eye_mat.m[1][3];
  eye_params->offset[2] = -eye_mat.m[2][3];
  return eye_params;
}

mojom::VRDisplayInfoPtr CreateVRDisplayInfo(gvr::GvrApi* gvr_api,
                                            uint32_t device_id) {
  TRACE_EVENT0("input", "GvrDelegate::CreateVRDisplayInfo");

  mojom::VRDisplayInfoPtr device = mojom::VRDisplayInfo::New();

  device->index = device_id;

  device->capabilities = mojom::VRDisplayCapabilities::New();
  device->capabilities->hasPosition = false;
  device->capabilities->hasExternalDisplay = false;
  device->capabilities->canPresent = true;

  std::string vendor = gvr_api->GetViewerVendor();
  std::string model = gvr_api->GetViewerModel();
  device->displayName = vendor + " " + model;

  gvr::BufferViewportList gvr_buffer_viewports =
      gvr_api->CreateEmptyBufferViewportList();
  gvr_buffer_viewports.SetToRecommendedBufferViewports();

  gfx::Size maximum_size = GetMaximumWebVrSize(gvr_api);
  device->leftEye = CreateEyeParamater(gvr_api, GVR_LEFT_EYE,
                                       gvr_buffer_viewports, maximum_size);
  device->rightEye = CreateEyeParamater(gvr_api, GVR_RIGHT_EYE,
                                        gvr_buffer_viewports, maximum_size);

  // This scalar will be applied in the renderer to the recommended render
  // target sizes. For WebVR it will always be applied, for WebXR it can be
  // overridden.
  if (base::AndroidHardwareBufferCompat::IsSupportAvailable()) {
    device->webxr_default_framebuffer_scale = kWebXrRecommendedResolutionScale;
  } else {
    device->webxr_default_framebuffer_scale =
        kWebXrNoSharedBufferResolutionScale;
  }
  device->webvr_default_framebuffer_scale = kWebVrRecommendedResolutionScale;

  return device;
}

}  // namespace

std::unique_ptr<GvrDevice> GvrDevice::Create() {
  std::unique_ptr<GvrDevice> device = base::WrapUnique(new GvrDevice());
  if (!device->gvr_api_)
    return nullptr;
  return device;
}

GvrDevice::GvrDevice() : weak_ptr_factory_(this) {
  GvrDelegateProvider* delegate_provider = GetGvrDelegateProvider();
  if (!delegate_provider || delegate_provider->ShouldDisableGvrDevice())
    return;
  JNIEnv* env = base::android::AttachCurrentThread();
  non_presenting_context_.Reset(
      Java_NonPresentingGvrContext_create(env, reinterpret_cast<jlong>(this)));
  if (!non_presenting_context_.obj())
    return;
  jlong context = Java_NonPresentingGvrContext_getNativeGvrContext(
      env, non_presenting_context_);
  gvr_api_ = gvr::GvrApi::WrapNonOwned(reinterpret_cast<gvr_context*>(context));
  SetVRDisplayInfo(CreateVRDisplayInfo(gvr_api_.get(), GetId()));
  GvrDelegateProviderFactory::SetDevice(this);
}

GvrDevice::~GvrDevice() {
  GvrDelegateProviderFactory::SetDevice(nullptr);
  if (!non_presenting_context_.obj())
    return;
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_NonPresentingGvrContext_shutdown(env, non_presenting_context_);
}

void GvrDevice::RequestPresent(
    mojom::VRSubmitFrameClientPtr submit_client,
    mojom::VRPresentationProviderRequest request,
    mojom::VRRequestPresentOptionsPtr present_options,
    mojom::VRDisplayHost::RequestPresentCallback callback) {
  GvrDelegateProvider* delegate_provider = GetGvrDelegateProvider();
  if (!delegate_provider) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  // RequestWebVRPresent is async as we may trigger a DON (Device ON) flow that
  // pauses Chrome.
  delegate_provider->RequestWebVRPresent(
      std::move(submit_client), std::move(request), GetVRDisplayInfo(),
      std::move(present_options),
      base::BindOnce(&GvrDevice::OnRequestPresentResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void GvrDevice::OnRequestPresentResult(
    mojom::VRDisplayHost::RequestPresentCallback callback,
    bool result,
    mojom::VRDisplayFrameTransportOptionsPtr transport_options) {
  if (result)
    SetIsPresenting();
  std::move(callback).Run(result, std::move(transport_options));
}

void GvrDevice::ExitPresent() {
  GvrDelegateProvider* delegate_provider = GetGvrDelegateProvider();
  if (delegate_provider)
    delegate_provider->ExitWebVRPresent();
  OnExitPresent();
}

void GvrDevice::OnMagicWindowPoseRequest(
    mojom::VRMagicWindowProvider::GetPoseCallback callback) {
  std::move(callback).Run(
      GvrDelegate::GetVRPosePtrWithNeckModel(gvr_api_.get(), nullptr));
}

void GvrDevice::OnListeningForActivate(bool listening) {
  GvrDelegateProvider* delegate_provider = GetGvrDelegateProvider();
  if (!delegate_provider)
    return;
  delegate_provider->OnListeningForActivateChanged(listening);
}

void GvrDevice::PauseTracking() {
  gvr_api_->PauseTracking();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_NonPresentingGvrContext_pause(env, non_presenting_context_);
}

void GvrDevice::ResumeTracking() {
  gvr_api_->ResumeTracking();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_NonPresentingGvrContext_resume(env, non_presenting_context_);
}

GvrDelegateProvider* GvrDevice::GetGvrDelegateProvider() {
  // GvrDelegateProviderFactory::Create() may fail transiently, so every time we
  // try to get it, set the device ID.
  GvrDelegateProvider* delegate_provider = GvrDelegateProviderFactory::Create();
  if (delegate_provider)
    delegate_provider->SetDeviceId(GetId());
  return delegate_provider;
}

void GvrDevice::OnDisplayConfigurationChanged(JNIEnv* env,
                                              const JavaRef<jobject>& obj) {
  SetVRDisplayInfo(CreateVRDisplayInfo(gvr_api_.get(), GetId()));
}

void GvrDevice::Activate(mojom::VRDisplayEventReason reason,
                         base::Callback<void(bool)> on_handled) {
  OnActivate(reason, std::move(on_handled));
}

}  // namespace device
