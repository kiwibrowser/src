// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/oculus/oculus_render_loop.h"

#include "device/vr/oculus/oculus_type_converters.h"
#include "third_party/libovr/src/Include/Extras/OVR_Math.h"
#include "third_party/libovr/src/Include/OVR_CAPI.h"
#include "third_party/libovr/src/Include/OVR_CAPI_D3D.h"
#include "ui/gfx/geometry/angle_conversions.h"

#if defined(OS_WIN)
#include "device/vr/windows/d3d11_texture_helper.h"
#endif

namespace device {

namespace {

// How far the index trigger needs to be pushed to be considered "pressed".
const float kTriggerPressedThreshold = 0.6f;

// WebXR reports a pointer pose separate from the grip pose, which represents a
// pointer ray emerging from the tip of the controller. Oculus does not report
// anything like that, but the pose they report matches WebXR's idea of the
// pointer pose more than grip. For consistency with other WebXR backends we
// apply a rotation and slight translation to the reported pose to get the grip
// pose. Experimentally determined, should roughly place the grip pose origin at
// the center of the Oculus Touch handle.
const float kGripRotationXDelta = 45.0f;
const float kGripOffsetZMeters = 0.025f;

gfx::Transform PoseToTransform(const ovrPosef& pose) {
  OVR::Matrix4f mat(pose);
  return gfx::Transform(mat.M[0][0], mat.M[0][1], mat.M[0][2], mat.M[0][3],
                        mat.M[1][0], mat.M[1][1], mat.M[1][2], mat.M[1][3],
                        mat.M[2][0], mat.M[2][1], mat.M[2][2], mat.M[2][3],
                        mat.M[3][0], mat.M[3][1], mat.M[3][2], mat.M[3][3]);
}

}  // namespace

OculusRenderLoop::OculusRenderLoop(ovrSession session, ovrGraphicsLuid luid)
    : base::Thread("OculusRenderLoop"),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      session_(session),
      luid_(luid),
      binding_(this),
      weak_ptr_factory_(this) {
  DCHECK(main_thread_task_runner_);
}

OculusRenderLoop::~OculusRenderLoop() {
  Stop();
}

void OculusRenderLoop::CleanUp() {
  submit_client_ = nullptr;
  binding_.Close();
}

void OculusRenderLoop::SubmitFrameMissing(int16_t frame_index,
                                          const gpu::SyncToken& sync_token) {
  // Nothing to do. It's OK to start the next frame even if the current
  // one didn't get sent to the ovrSession.
}

void OculusRenderLoop::SubmitFrame(int16_t frame_index,
                                   const gpu::MailboxHolder& mailbox,
                                   base::TimeDelta time_waited) {
  // There are two submit paths - one for Android, and one for Windows. This
  // method is called on Android, but this class isn't used on Android.
  NOTREACHED();
}

void OculusRenderLoop::SubmitFrameDrawnIntoTexture(
    int16_t frame_index,
    const gpu::SyncToken& sync_token,
    base::TimeDelta time_waited) {
  // Not currently implemented for Windows.
  NOTREACHED();
}

void OculusRenderLoop::SubmitFrameWithTextureHandle(
    int16_t frame_index,
    mojo::ScopedHandle texture_handle) {
  TRACE_EVENT1("gpu", "SubmitFrameWithTextureHandle", "frameIndex",
               frame_index);

#if defined(OS_WIN)
  MojoPlatformHandle platform_handle;
  platform_handle.struct_size = sizeof(platform_handle);
  MojoResult result = MojoUnwrapPlatformHandle(texture_handle.release().value(),
                                               nullptr, &platform_handle);
  if (result != MOJO_RESULT_OK)
    return;

  texture_helper_.SetSourceTexture(
      base::win::ScopedHandle(reinterpret_cast<HANDLE>(platform_handle.value)));

  // Create swap chain on demand.
  if (!texture_swap_chain_) {
    ovrTextureSwapChainDesc desc = {};
    desc.Type = ovrTexture_2D;
    desc.ArraySize = 1;
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.Width = source_size_.width();
    desc.Height = source_size_.height();
    desc.MipLevels = 1;
    desc.SampleCount = 1;
    desc.StaticImage = ovrFalse;
    desc.MiscFlags = ovrTextureMisc_DX_Typeless;
    desc.BindFlags = ovrTextureBind_DX_RenderTarget;
    ovr_CreateTextureSwapChainDX(session_, texture_helper_.GetDevice().Get(),
                                 &desc, &texture_swap_chain_);
  }

  bool copy_succeeded = false;
  if (texture_swap_chain_) {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    ovr_GetTextureSwapChainBufferDX(
        session_, texture_swap_chain_, -1,
        IID_PPV_ARGS(texture.ReleaseAndGetAddressOf()));
    texture_helper_.SetBackbuffer(texture);

    if (texture_helper_.CopyTextureToBackBuffer(true)) {
      copy_succeeded = true;
      ovrLayerEyeFov layer = {};
      layer.Header.Type = ovrLayerType_EyeFov;
      layer.Header.Flags = 0;
      layer.ColorTexture[0] = texture_swap_chain_;
      DCHECK(source_size_.width() % 2 == 0);
      layer.Viewport[0] = {
          // Left viewport.
          {static_cast<int>(source_size_.width() * left_bounds_.x()),
           static_cast<int>(source_size_.height() * left_bounds_.y())},
          {static_cast<int>(source_size_.width() * left_bounds_.width()),
           static_cast<int>(source_size_.height() * left_bounds_.height())}};

      layer.Viewport[1] = {
          // Right viewport.
          {static_cast<int>(source_size_.width() * right_bounds_.x()),
           static_cast<int>(source_size_.height() * right_bounds_.y())},
          {static_cast<int>(source_size_.width() * right_bounds_.width()),
           static_cast<int>(source_size_.height() * right_bounds_.height())}};
      ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session_);
      layer.Fov[0] = hmdDesc.DefaultEyeFov[0];
      layer.Fov[1] = hmdDesc.DefaultEyeFov[1];

      auto render_desc_left = ovr_GetRenderDesc(
          session_, ovrEye_Left, hmdDesc.DefaultEyeFov[ovrEye_Left]);
      auto render_desc_right = ovr_GetRenderDesc(
          session_, ovrEye_Right, hmdDesc.DefaultEyeFov[ovrEye_Right]);
      ovrVector3f eye_offsets[2] = {render_desc_left.HmdToEyeOffset,
                                    render_desc_right.HmdToEyeOffset};
      ovr_CalcEyePoses(last_render_pose_, eye_offsets, layer.RenderPose);

      layer.SensorSampleTime = sensor_time_;

      ovrViewScaleDesc view_scale_desc;
      view_scale_desc.HmdToEyeOffset[ovrEye_Left] = eye_offsets[ovrEye_Left];
      view_scale_desc.HmdToEyeOffset[ovrEye_Right] = eye_offsets[ovrEye_Right];
      view_scale_desc.HmdSpaceToWorldScaleInMeters = 1;

      constexpr unsigned int layer_count = 1;
      ovrLayerHeader* layer_headers[layer_count] = {&layer.Header};
      ovr_CommitTextureSwapChain(session_, texture_swap_chain_);
      ovrResult result =
          ovr_SubmitFrame(session_, ovr_frame_index_, &view_scale_desc,
                          layer_headers, layer_count);
      DCHECK(OVR_SUCCESS(result));
      ovr_frame_index_++;
    }
  }
  // Tell WebVR that we are done with the texture.
  submit_client_->OnSubmitFrameTransferred(copy_succeeded);
  submit_client_->OnSubmitFrameRendered();
#endif
}

void OculusRenderLoop::UpdateLayerBounds(int16_t frame_id,
                                         const gfx::RectF& left_bounds,
                                         const gfx::RectF& right_bounds,
                                         const gfx::Size& source_size) {
  // There is no parallelism, so we can ignore frame_id and assume the bounds
  // are updating for the next frame.
  left_bounds_ = left_bounds;
  right_bounds_ = right_bounds;
  source_size_ = source_size;

  // TODO(billorr): Recreate texture_swap_chain_ when source_size_ has changed.
};

void OculusRenderLoop::RequestPresent(
    mojom::VRSubmitFrameClientPtrInfo submit_client_info,
    mojom::VRPresentationProviderRequest request,
    device::mojom::VRRequestPresentOptionsPtr present_options,
    device::mojom::VRDisplayHost::RequestPresentCallback callback) {
#if defined(OS_WIN)
  if (!texture_helper_.SetAdapterLUID(*reinterpret_cast<LUID*>(&luid_)) ||
      !texture_helper_.EnsureInitialized()) {
    main_thread_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), false, nullptr));
    return;
  }
#endif
  submit_client_.Bind(std::move(submit_client_info));

  binding_.Close();
  binding_.Bind(std::move(request));

  device::mojom::VRDisplayFrameTransportOptionsPtr transport_options =
      device::mojom::VRDisplayFrameTransportOptions::New();
  transport_options->transport_method =
      device::mojom::VRDisplayFrameTransportMethod::SUBMIT_AS_TEXTURE_HANDLE;
  // Only set boolean options that we need. Default is false, and we should be
  // able to safely ignore ones that our implementation doesn't care about.
  transport_options->wait_for_transfer_notification = true;

  report_webxr_input_ = present_options->webxr_input;

  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), true, std::move(transport_options)));
  is_presenting_ = true;
}

void OculusRenderLoop::ExitPresent() {
  is_presenting_ = false;
  binding_.Close();
  submit_client_ = nullptr;
}

void OculusRenderLoop::Init() {}

base::WeakPtr<OculusRenderLoop> OculusRenderLoop::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void OculusRenderLoop::GetVSync(
    mojom::VRPresentationProvider::GetVSyncCallback callback) {
  DCHECK(is_presenting_);
  int16_t frame = next_frame_id_;
  next_frame_id_ += 1;
  if (next_frame_id_ < 0) {
    next_frame_id_ = 0;
  }

  auto predicted_time =
      ovr_GetPredictedDisplayTime(session_, ovr_frame_index_ + 1);
  ovrTrackingState state = ovr_GetTrackingState(session_, predicted_time, true);
  sensor_time_ = ovr_GetTimeInSeconds();
  mojom::VRPosePtr pose =
      mojo::ConvertTo<mojom::VRPosePtr>(state.HeadPose.ThePose);
  last_render_pose_ = state.HeadPose.ThePose;

  if (pose && report_webxr_input_) {
    pose->input_state = GetInputState(state);
  }

  base::TimeDelta time = base::TimeDelta::FromSecondsD(predicted_time);

  std::move(callback).Run(std::move(pose), time, frame,
                          mojom::VRPresentationProvider::VSyncStatus::SUCCESS,
                          base::nullopt);
}

std::vector<mojom::XRInputSourceStatePtr> OculusRenderLoop::GetInputState(
    const ovrTrackingState& tracking_state) {
  std::vector<mojom::XRInputSourceStatePtr> input_states;

  ovrInputState input_state;

  // Get the state of the active controllers.
  if ((OVR_SUCCESS(ovr_GetInputState(session_, ovrControllerType_Active,
                                     &input_state)))) {
    // Report whichever touch controllers are currently active.
    if (input_state.ControllerType & ovrControllerType_LTouch) {
      input_states.push_back(GetTouchData(
          ovrControllerType_LTouch, tracking_state.HandPoses[ovrHand_Left],
          input_state, ovrHand_Left));
    } else {
      primary_input_pressed[ovrControllerType_LTouch] = false;
    }

    if (input_state.ControllerType & ovrControllerType_RTouch) {
      input_states.push_back(GetTouchData(
          ovrControllerType_RTouch, tracking_state.HandPoses[ovrHand_Right],
          input_state, ovrHand_Right));
    } else {
      primary_input_pressed[ovrControllerType_RTouch] = false;
    }

    // If an oculus remote is active, report a gaze controller.
    if (input_state.ControllerType & ovrControllerType_Remote) {
      device::mojom::XRInputSourceStatePtr state =
          device::mojom::XRInputSourceState::New();

      state->source_id = ovrControllerType_Remote;
      state->primary_input_pressed =
          (input_state.Buttons & ovrButton_Enter) != 0;

      if (!state->primary_input_pressed &&
          primary_input_pressed[ovrControllerType_Remote]) {
        state->primary_input_clicked = true;
      }

      primary_input_pressed[ovrControllerType_Remote] =
          state->primary_input_pressed;

      input_states.push_back(std::move(state));
    } else {
      primary_input_pressed[ovrControllerType_Remote] = false;
    }
  }

  return input_states;
}

device::mojom::XRInputSourceStatePtr OculusRenderLoop::GetTouchData(
    ovrControllerType type,
    const ovrPoseStatef& pose,
    const ovrInputState& input_state,
    ovrHandType hand) {
  device::mojom::XRInputSourceStatePtr state =
      device::mojom::XRInputSourceState::New();

  state->source_id = type;
  state->primary_input_pressed =
      (input_state.IndexTrigger[hand] > kTriggerPressedThreshold);

  // If the input has gone from pressed to not pressed since the last poll
  // report it as clicked.
  if (!state->primary_input_pressed && primary_input_pressed[type])
    state->primary_input_clicked = true;

  primary_input_pressed[type] = state->primary_input_pressed;

  device::mojom::XRInputSourceDescriptionPtr desc =
      device::mojom::XRInputSourceDescription::New();

  // It's a handheld pointing device.
  desc->pointer_origin = device::mojom::XRPointerOrigin::HAND;

  // Set handedness.
  switch (hand) {
    case ovrHand_Left:
      desc->handedness = device::mojom::XRHandedness::LEFT;
      break;
    case ovrHand_Right:
      desc->handedness = device::mojom::XRHandedness::RIGHT;
      break;
    default:
      desc->handedness = device::mojom::XRHandedness::NONE;
      break;
  }

  // Touch controller are fully 6DoF.
  desc->emulated_position = false;

  // The grip pose will be rotated and translated back a bit from the pointer
  // pose, which is what the Oculus API returns.
  state->grip = PoseToTransform(pose.ThePose);
  state->grip->RotateAboutXAxis(kGripRotationXDelta);
  state->grip->Translate3d(0, 0, kGripOffsetZMeters);

  // Need to apply the inverse transform from above to put the pointer back in
  // the right orientation relative to the grip.
  desc->pointer_offset = gfx::Transform();
  desc->pointer_offset->Translate3d(0, 0, -kGripOffsetZMeters);
  desc->pointer_offset->RotateAboutXAxis(-kGripRotationXDelta);

  state->description = std::move(desc);

  return state;
}

}  // namespace device
