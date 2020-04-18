// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/oculus/oculus_device.h"

#include <math.h>

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/numerics/math_constants.h"
#include "build/build_config.h"
#include "device/vr/oculus/oculus_render_loop.h"
#include "device/vr/oculus/oculus_type_converters.h"
#include "third_party/libovr/src/Include/OVR_CAPI.h"
#include "third_party/libovr/src/Include/OVR_CAPI_D3D.h"
#include "ui/gfx/geometry/angle_conversions.h"

namespace device {

namespace {

mojom::VREyeParametersPtr GetEyeDetails(ovrSession session,
                                        const ovrHmdDesc& hmd_desc,
                                        ovrEyeType eye) {
  auto eye_parameters = mojom::VREyeParameters::New();
  auto render_desc =
      ovr_GetRenderDesc(session, eye, hmd_desc.DefaultEyeFov[eye]);
  eye_parameters->fieldOfView = mojom::VRFieldOfView::New();
  eye_parameters->fieldOfView->upDegrees =
      gfx::RadToDeg(atanf(render_desc.Fov.UpTan));
  eye_parameters->fieldOfView->downDegrees =
      gfx::RadToDeg(atanf(render_desc.Fov.DownTan));
  eye_parameters->fieldOfView->leftDegrees =
      gfx::RadToDeg(atanf(render_desc.Fov.LeftTan));
  eye_parameters->fieldOfView->rightDegrees =
      gfx::RadToDeg(atanf(render_desc.Fov.RightTan));

  auto offset = render_desc.HmdToEyeOffset;
  eye_parameters->offset = std::vector<float>({offset.x, offset.y, offset.z});

  auto texture_size =
      ovr_GetFovTextureSize(session, eye, render_desc.Fov, 1.0f);
  eye_parameters->renderWidth = texture_size.w;
  eye_parameters->renderHeight = texture_size.h;

  return eye_parameters;
}

mojom::VRDisplayInfoPtr CreateVRDisplayInfo(unsigned int id,
                                            ovrSession session) {
  mojom::VRDisplayInfoPtr display_info = mojom::VRDisplayInfo::New();
  display_info->index = id;
  display_info->displayName = std::string("Oculus");
  display_info->capabilities = mojom::VRDisplayCapabilities::New();
  display_info->capabilities->hasPosition = true;
  display_info->capabilities->hasExternalDisplay = true;
  display_info->capabilities->canPresent = true;
  display_info->webvr_default_framebuffer_scale = 1.0;
  display_info->webxr_default_framebuffer_scale = 1.0;

  ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
  display_info->leftEye = GetEyeDetails(session, hmdDesc, ovrEye_Left);
  display_info->rightEye = GetEyeDetails(session, hmdDesc, ovrEye_Right);

  display_info->stageParameters = mojom::VRStageParameters::New();
  ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
  ovrTrackingState ovr_state = ovr_GetTrackingState(session, 0, true);
  float floor_height = ovr_state.HeadPose.ThePose.Position.y;
  ovr_SetTrackingOriginType(session, ovrTrackingOrigin_EyeLevel);

  display_info->stageParameters->standingTransform = {
      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, floor_height, 0, 1};

  ovrVector3f boundary_size;
  ovr_GetBoundaryDimensions(session, ovrBoundary_PlayArea, &boundary_size);
  display_info->stageParameters->sizeX = boundary_size.x;
  display_info->stageParameters->sizeZ = boundary_size.z;

  return display_info;
}

}  // namespace

OculusDevice::OculusDevice(ovrSession session, ovrGraphicsLuid luid)
    : session_(session),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {
  SetVRDisplayInfo(CreateVRDisplayInfo(GetId(), session_));

  render_loop_ = std::make_unique<OculusRenderLoop>(session_, luid);
}

OculusDevice::~OculusDevice() {}

void OculusDevice::RequestPresent(
    mojom::VRSubmitFrameClientPtr submit_client,
    mojom::VRPresentationProviderRequest request,
    mojom::VRRequestPresentOptionsPtr present_options,
    mojom::VRDisplayHost::RequestPresentCallback callback) {
  if (!render_loop_->IsRunning())
    render_loop_->Start();

  if (!render_loop_->IsRunning()) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  auto on_request_present_result =
      base::BindOnce(&OculusDevice::OnRequestPresentResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  render_loop_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&OculusRenderLoop::RequestPresent,
                     render_loop_->GetWeakPtr(), submit_client.PassInterface(),
                     std::move(request), std::move(present_options),
                     std::move(on_request_present_result)));
}

void OculusDevice::OnRequestPresentResult(
    mojom::VRDisplayHost::RequestPresentCallback callback,
    bool result,
    mojom::VRDisplayFrameTransportOptionsPtr transport_options) {
  std::move(callback).Run(result, std::move(transport_options));
}

void OculusDevice::ExitPresent() {
  render_loop_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&OculusRenderLoop::ExitPresent,
                                render_loop_->GetWeakPtr()));
  OnExitPresent();
}

void OculusDevice::OnMagicWindowPoseRequest(
    mojom::VRMagicWindowProvider::GetPoseCallback callback) {
  ovrTrackingState state = ovr_GetTrackingState(session_, 0, false);
  std::move(callback).Run(
      mojo::ConvertTo<mojom::VRPosePtr>(state.HeadPose.ThePose));
}

}  // namespace device
