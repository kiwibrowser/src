// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/openvr/openvr_device.h"

#include <math.h>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/numerics/math_constants.h"
#include "build/build_config.h"
#include "device/vr/openvr/openvr_render_loop.h"
#include "device/vr/openvr/openvr_type_converters.h"
#include "third_party/openvr/src/headers/openvr.h"
#include "ui/gfx/geometry/angle_conversions.h"

namespace device {

namespace {

constexpr float kDefaultIPD = 0.06f;  // Default average IPD.
constexpr double kTimeBetweenPollingEventsSeconds = 0.25;

mojom::VRFieldOfViewPtr OpenVRFovToWebVRFov(vr::IVRSystem* vr_system,
                                            vr::Hmd_Eye eye) {
  auto out = mojom::VRFieldOfView::New();
  float up_tan, down_tan, left_tan, right_tan;
  vr_system->GetProjectionRaw(eye, &left_tan, &right_tan, &up_tan, &down_tan);

  // TODO(billorr): Plumb the expected projection matrix over mojo instead of
  // using angles. Up and down are intentionally swapped to account for
  // differences in expected projection matrix format for GVR and OpenVR.
  out->upDegrees = gfx::RadToDeg(atanf(down_tan));
  out->downDegrees = -gfx::RadToDeg(atanf(up_tan));
  out->leftDegrees = -gfx::RadToDeg(atanf(left_tan));
  out->rightDegrees = gfx::RadToDeg(atanf(right_tan));
  return out;
}

std::string GetOpenVRString(vr::IVRSystem* vr_system,
                            vr::TrackedDeviceProperty prop) {
  std::string out;

  vr::TrackedPropertyError error = vr::TrackedProp_Success;
  char openvr_string[vr::k_unMaxPropertyStringSize];
  vr_system->GetStringTrackedDeviceProperty(
      vr::k_unTrackedDeviceIndex_Hmd, prop, openvr_string,
      vr::k_unMaxPropertyStringSize, &error);

  if (error == vr::TrackedProp_Success)
    out = openvr_string;

  return out;
}

std::vector<float> HmdMatrix34ToWebVRTransformMatrix(
    const vr::HmdMatrix34_t& mat) {
  std::vector<float> transform;
  transform.resize(16);
  transform[0] = mat.m[0][0];
  transform[1] = mat.m[1][0];
  transform[2] = mat.m[2][0];
  transform[3] = 0.0f;
  transform[4] = mat.m[0][1];
  transform[5] = mat.m[1][1];
  transform[6] = mat.m[2][1];
  transform[7] = 0.0f;
  transform[8] = mat.m[0][2];
  transform[9] = mat.m[1][2];
  transform[10] = mat.m[2][2];
  transform[11] = 0.0f;
  transform[12] = mat.m[0][3];
  transform[13] = mat.m[1][3];
  transform[14] = mat.m[2][3];
  transform[15] = 1.0f;
  return transform;
}

mojom::VRDisplayInfoPtr CreateVRDisplayInfo(vr::IVRSystem* vr_system,
                                            unsigned int id) {
  mojom::VRDisplayInfoPtr display_info = mojom::VRDisplayInfo::New();
  display_info->index = id;
  display_info->displayName =
      GetOpenVRString(vr_system, vr::Prop_ManufacturerName_String) + " " +
      GetOpenVRString(vr_system, vr::Prop_ModelNumber_String);
  display_info->capabilities = mojom::VRDisplayCapabilities::New();
  display_info->capabilities->hasPosition = true;
  display_info->capabilities->hasExternalDisplay = true;
  display_info->capabilities->canPresent = true;
  display_info->webvr_default_framebuffer_scale = 1.0;
  display_info->webxr_default_framebuffer_scale = 1.0;

  display_info->leftEye = mojom::VREyeParameters::New();
  display_info->rightEye = mojom::VREyeParameters::New();
  mojom::VREyeParametersPtr& left_eye = display_info->leftEye;
  mojom::VREyeParametersPtr& right_eye = display_info->rightEye;

  left_eye->fieldOfView = OpenVRFovToWebVRFov(vr_system, vr::Eye_Left);
  right_eye->fieldOfView = OpenVRFovToWebVRFov(vr_system, vr::Eye_Right);

  vr::TrackedPropertyError error = vr::TrackedProp_Success;
  float ipd = vr_system->GetFloatTrackedDeviceProperty(
      vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_UserIpdMeters_Float, &error);

  if (error != vr::TrackedProp_Success)
    ipd = kDefaultIPD;

  left_eye->offset.resize(3);
  left_eye->offset[0] = -ipd * 0.5;
  left_eye->offset[1] = 0.0f;
  left_eye->offset[2] = 0.0f;
  right_eye->offset.resize(3);
  right_eye->offset[0] = ipd * 0.5;
  right_eye->offset[1] = 0.0;
  right_eye->offset[2] = 0.0;

  uint32_t width, height;
  vr_system->GetRecommendedRenderTargetSize(&width, &height);
  left_eye->renderWidth = width;
  left_eye->renderHeight = height;
  right_eye->renderWidth = left_eye->renderWidth;
  right_eye->renderHeight = left_eye->renderHeight;

  display_info->stageParameters = mojom::VRStageParameters::New();
  vr::HmdMatrix34_t mat =
      vr_system->GetSeatedZeroPoseToStandingAbsoluteTrackingPose();
  display_info->stageParameters->standingTransform =
      HmdMatrix34ToWebVRTransformMatrix(mat);

  vr::IVRChaperone* chaperone = vr::VRChaperone();
  if (chaperone) {
    chaperone->GetPlayAreaSize(&display_info->stageParameters->sizeX,
                               &display_info->stageParameters->sizeZ);
  } else {
    display_info->stageParameters->sizeX = 0.0f;
    display_info->stageParameters->sizeZ = 0.0f;
  }

  return display_info;
}


}  // namespace

OpenVRDevice::OpenVRDevice(vr::IVRSystem* vr)
    : vr_system_(vr),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {
  DCHECK(vr_system_);
  SetVRDisplayInfo(CreateVRDisplayInfo(vr_system_, GetId()));

  render_loop_ = std::make_unique<OpenVRRenderLoop>(vr);

  OnPollingEvents();
}

OpenVRDevice::~OpenVRDevice() {
  Shutdown();
}

void OpenVRDevice::Shutdown() {
  // Wait for the render loop to stop before completing destruction. This will
  // ensure that the IVRSystem doesn't get shutdown until the render loop is no
  // longer referencing it.
  if (render_loop_->IsRunning())
    render_loop_->Stop();
}

void OpenVRDevice::RequestPresent(
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

  auto my_callback =
      base::BindOnce(&OpenVRDevice::OnRequestPresentResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  render_loop_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&OpenVRRenderLoop::RequestPresent,
                     render_loop_->GetWeakPtr(), submit_client.PassInterface(),
                     std::move(request), std::move(present_options),
                     std::move(my_callback)));
}

void OpenVRDevice::OnRequestPresentResult(
    mojom::VRDisplayHost::RequestPresentCallback callback,
    bool result,
    mojom::VRDisplayFrameTransportOptionsPtr transport_options) {
  std::move(callback).Run(result, std::move(transport_options));

  if (result) {
    using ViewerMap = std::map<std::string, VrViewerType>;
    CR_DEFINE_STATIC_LOCAL(
        ViewerMap, viewer_types,
        ({
            {"Oculus Rift CV1", VrViewerType::OPENVR_RIFT_CV1},
            {"Vive MV", VrViewerType::OPENVR_VIVE},
        }));

    VrViewerType type = VrViewerType::OPENVR_UNKNOWN;
    std::string model =
        GetOpenVRString(vr_system_, vr::Prop_ModelNumber_String);
    auto it = viewer_types.find(model);
    if (it != viewer_types.end())
      type = it->second;

    base::UmaHistogramSparse("VRViewerType", static_cast<int>(type));
  }
}

void OpenVRDevice::ExitPresent() {
  render_loop_->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&OpenVRRenderLoop::ExitPresent, render_loop_->GetWeakPtr()));
  render_loop_->Stop();
  OnExitPresent();
}

void OpenVRDevice::OnMagicWindowPoseRequest(
    mojom::VRMagicWindowProvider::GetPoseCallback callback) {
  vr::TrackedDevicePose_t rendering_poses[vr::k_unMaxTrackedDeviceCount];
  vr_system_->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseSeated, 0.03f,
                                              rendering_poses,
                                              vr::k_unMaxTrackedDeviceCount);
  std::move(callback).Run(mojo::ConvertTo<mojom::VRPosePtr>(
      rendering_poses[vr::k_unTrackedDeviceIndex_Hmd]));
}

// Only deal with events that will cause displayInfo changes for now.
void OpenVRDevice::OnPollingEvents() {
  if (!vr_system_)
    return;

  vr::VREvent_t event;
  bool is_changed = false;
  while (vr_system_->PollNextEvent(&event, sizeof(event))) {
    if (event.trackedDeviceIndex != vr::k_unTrackedDeviceIndex_Hmd &&
        event.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
      continue;
    }

    switch (event.eventType) {
      case vr::VREvent_TrackedDeviceUpdated:
      case vr::VREvent_IpdChanged:
      case vr::VREvent_ChaperoneDataHasChanged:
      case vr::VREvent_ChaperoneSettingsHaveChanged:
      case vr::VREvent_ChaperoneUniverseHasChanged:
        is_changed = true;
        break;

      default:
        break;
    }
  }

  if (is_changed)
    SetVRDisplayInfo(CreateVRDisplayInfo(vr_system_, GetId()));

  main_thread_task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&OpenVRDevice::OnPollingEvents,
                 weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSecondsD(kTimeBetweenPollingEventsSeconds));
}

}  // namespace device
