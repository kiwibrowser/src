// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/chromeos/camera_3a_controller.h"

#include "media/capture/video/chromeos/camera_metadata_utils.h"

namespace media {

namespace {

template <typename EntryType>
bool Get3AEntry(const cros::mojom::CameraMetadataPtr& metadata,
                cros::mojom::CameraMetadataTag control,
                EntryType* result) {
  const auto* entry = GetMetadataEntry(metadata, control);
  if (entry) {
    *result = static_cast<EntryType>((*entry)->data[0]);
    return true;
  } else {
    return false;
  }
}

}  // namespace

Camera3AController::Camera3AController(
    const cros::mojom::CameraMetadataPtr& static_metadata,
    CaptureMetadataDispatcher* capture_metadata_dispatcher,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : capture_metadata_dispatcher_(capture_metadata_dispatcher),
      task_runner_(std::move(task_runner)),
      af_mode_(cros::mojom::AndroidControlAfMode::ANDROID_CONTROL_AF_MODE_OFF),
      af_state_(cros::mojom::AndroidControlAfState::
                    ANDROID_CONTROL_AF_STATE_INACTIVE),
      af_mode_set_(false),
      ae_mode_(cros::mojom::AndroidControlAeMode::ANDROID_CONTROL_AE_MODE_ON),
      ae_state_(cros::mojom::AndroidControlAeState::
                    ANDROID_CONTROL_AE_STATE_INACTIVE),
      ae_mode_set_(false),
      awb_mode_(
          cros::mojom::AndroidControlAwbMode::ANDROID_CONTROL_AWB_MODE_AUTO),
      awb_state_(cros::mojom::AndroidControlAwbState::
                     ANDROID_CONTROL_AWB_STATE_INACTIVE),
      awb_mode_set_(false),
      weak_ptr_factory_(this) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  capture_metadata_dispatcher_->AddResultMetadataObserver(this);

  auto* af_modes = GetMetadataEntry(
      static_metadata,
      cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_AVAILABLE_MODES);
  if (af_modes) {
    for (const auto& m : (*af_modes)->data) {
      available_af_modes_.insert(
          static_cast<cros::mojom::AndroidControlAfMode>(m));
    }
  }
  auto* ae_modes = GetMetadataEntry(
      static_metadata,
      cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AE_AVAILABLE_MODES);
  if (ae_modes) {
    for (const auto& m : (*ae_modes)->data) {
      available_ae_modes_.insert(
          static_cast<cros::mojom::AndroidControlAeMode>(m));
    }
  }
  auto* awb_modes = GetMetadataEntry(
      static_metadata,
      cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AWB_AVAILABLE_MODES);
  if (awb_modes) {
    for (const auto& m : (*awb_modes)->data) {
      available_awb_modes_.insert(
          static_cast<cros::mojom::AndroidControlAwbMode>(m));
    }
  }

  // Enable AF if supported.  MODE_AUTO is always supported on auto-focus camera
  // modules; fixed focus camera modules always has MODE_OFF.
  if (available_af_modes_.count(
          cros::mojom::AndroidControlAfMode::ANDROID_CONTROL_AF_MODE_AUTO)) {
    af_mode_ = cros::mojom::AndroidControlAfMode::ANDROID_CONTROL_AF_MODE_AUTO;
  }
  // AE should always be MODE_ON unless we enable manual sensor control.  Since
  // we don't have flash on any of our devices we don't care about the
  // flash-related AE modes.
  //
  // AWB should always be MODE_AUTO unless we enable manual sensor control.
  Set3AMode(cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_MODE,
            base::checked_cast<uint8_t>(af_mode_));
  Set3AMode(cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AE_MODE,
            base::checked_cast<uint8_t>(ae_mode_));
  Set3AMode(cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AWB_MODE,
            base::checked_cast<uint8_t>(awb_mode_));
}

Camera3AController::~Camera3AController() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  capture_metadata_dispatcher_->RemoveResultMetadataObserver(this);
}

void Camera3AController::Stabilize3AForStillCapture(
    base::OnceClosure on_3a_stabilized_callback) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (on_3a_stabilized_callback_ || on_3a_mode_set_callback_) {
    // Already stabilizing 3A.
    return;
  }

  if (Is3AStabilized()) {
    std::move(on_3a_stabilized_callback).Run();
    return;
  }

  // Wait until all the 3A modes are set in the HAL; otherwise the AF trigger
  // and AE precapture trigger may be invalidated during mode transition.
  if (!af_mode_set_ || !ae_mode_set_ || !awb_mode_set_) {
    on_3a_mode_set_callback_ =
        base::BindOnce(&Camera3AController::Stabilize3AForStillCapture,
                       GetWeakPtr(), base::Passed(&on_3a_stabilized_callback));
    return;
  }

  on_3a_stabilized_callback_ = std::move(on_3a_stabilized_callback);

  if (af_mode_ !=
      cros::mojom::AndroidControlAfMode::ANDROID_CONTROL_AF_MODE_OFF) {
    DVLOG(1) << "Start AF trigger to lock focus";
    std::vector<uint8_t> af_trigger = {
        base::checked_cast<uint8_t>(cros::mojom::AndroidControlAfTrigger::
                                        ANDROID_CONTROL_AF_TRIGGER_START)};
    capture_metadata_dispatcher_->SetCaptureMetadata(
        cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_TRIGGER,
        cros::mojom::EntryType::TYPE_BYTE, 1, std::move(af_trigger));
  }

  if (ae_mode_ !=
      cros::mojom::AndroidControlAeMode::ANDROID_CONTROL_AE_MODE_OFF) {
    DVLOG(1) << "Start AE precapture trigger to converge exposure";
    std::vector<uint8_t> ae_precapture_trigger = {base::checked_cast<uint8_t>(
        cros::mojom::AndroidControlAePrecaptureTrigger::
            ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_START)};
    capture_metadata_dispatcher_->SetCaptureMetadata(
        cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER,
        cros::mojom::EntryType::TYPE_BYTE, 1, std::move(ae_precapture_trigger));
  }
}

void Camera3AController::OnResultMetadataAvailable(
    const cros::mojom::CameraMetadataPtr& result_metadata) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (af_mode_set_ && ae_mode_set_ && awb_mode_set_ &&
      !on_3a_stabilized_callback_) {
    // Process the result metadata only when we need to check if 3A modes are
    // synchronized, or when there's a pending 3A stabilization request.
    return;
  }

  cros::mojom::AndroidControlAfMode af_mode;
  if (Get3AEntry(result_metadata,
                 cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_MODE,
                 &af_mode)) {
    af_mode_set_ = (af_mode == af_mode_);
  } else {
    DVLOG(2) << "AF mode is not available in the metadata";
  }
  if (!Get3AEntry(result_metadata,
                  cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_STATE,
                  &af_state_)) {
    DVLOG(2) << "AF state is not available in the metadata";
  }

  cros::mojom::AndroidControlAeMode ae_mode;
  if (Get3AEntry(result_metadata,
                 cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AE_MODE,
                 &ae_mode)) {
    ae_mode_set_ = (ae_mode == ae_mode_);
  } else {
    DVLOG(2) << "AE mode is not available in the metadata";
  }
  if (!Get3AEntry(result_metadata,
                  cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AE_STATE,
                  &ae_state_)) {
    DVLOG(2) << "AE state is not available in the metadata";
  }

  cros::mojom::AndroidControlAwbMode awb_mode;
  if (Get3AEntry(result_metadata,
                 cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AWB_MODE,
                 &awb_mode)) {
    awb_mode_set_ = (awb_mode == awb_mode_);
  } else {
    DVLOG(2) << "AWB mode is not available in the metadata";
  }
  if (!Get3AEntry(result_metadata,
                  cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AWB_STATE,
                  &awb_state_)) {
    DVLOG(2) << "AWB state is not available in the metadata";
  }

  DVLOG(2) << "AF mode: " << af_mode_;
  DVLOG(2) << "AF state: " << af_state_;
  DVLOG(2) << "AE mode: " << ae_mode_;
  DVLOG(2) << "AE state: " << ae_state_;
  DVLOG(2) << "AWB mode: " << awb_mode_;
  DVLOG(2) << "AWB state: " << awb_state_;

  if (on_3a_mode_set_callback_ && af_mode_set_ && ae_mode_set_ &&
      awb_mode_set_) {
    task_runner_->PostTask(FROM_HERE, std::move(on_3a_mode_set_callback_));
  }

  if (on_3a_stabilized_callback_ && Is3AStabilized()) {
    std::move(on_3a_stabilized_callback_).Run();
  }
}

void Camera3AController::SetAutoFocusModeForStillCapture() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  std::vector<uint8_t> af_trigger = {base::checked_cast<uint8_t>(
      cros::mojom::AndroidControlAfTrigger::ANDROID_CONTROL_AF_TRIGGER_CANCEL)};
  capture_metadata_dispatcher_->SetCaptureMetadata(
      cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_TRIGGER,
      cros::mojom::EntryType::TYPE_BYTE, 1, std::move(af_trigger));

  if (available_af_modes_.count(
          cros::mojom::AndroidControlAfMode::
              ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE)) {
    af_mode_ = cros::mojom::AndroidControlAfMode::
        ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
  }
  std::vector<uint8_t> af_mode = {base::checked_cast<uint8_t>(af_mode_)};
  Set3AMode(cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_MODE,
            base::checked_cast<uint8_t>(af_mode_));
  DVLOG(1) << "Setting AF mode to: " << af_mode_;
}

void Camera3AController::SetAutoFocusModeForVideoRecording() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  std::vector<uint8_t> af_trigger = {base::checked_cast<uint8_t>(
      cros::mojom::AndroidControlAfTrigger::ANDROID_CONTROL_AF_TRIGGER_CANCEL)};
  capture_metadata_dispatcher_->SetCaptureMetadata(
      cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_TRIGGER,
      cros::mojom::EntryType::TYPE_BYTE, 1, std::move(af_trigger));

  if (available_af_modes_.count(cros::mojom::AndroidControlAfMode::
                                    ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO)) {
    af_mode_ = cros::mojom::AndroidControlAfMode::
        ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
  }
  Set3AMode(cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_MODE,
            base::checked_cast<uint8_t>(af_mode_));
  DVLOG(1) << "Setting AF mode to: " << af_mode_;
}

base::WeakPtr<Camera3AController> Camera3AController::GetWeakPtr() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  return weak_ptr_factory_.GetWeakPtr();
}

void Camera3AController::Set3AMode(cros::mojom::CameraMetadataTag tag,
                                   uint8_t target_mode) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(tag == cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_MODE ||
         tag == cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AE_MODE ||
         tag == cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AWB_MODE);

  std::vector<uint8_t> mode = {base::checked_cast<uint8_t>(target_mode)};
  capture_metadata_dispatcher_->SetCaptureMetadata(
      tag, cros::mojom::EntryType::TYPE_BYTE, 1, std::move(mode));

  switch (tag) {
    case cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AF_MODE:
      af_mode_set_ = false;
      break;
    case cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AE_MODE:
      ae_mode_set_ = false;
      break;
    case cros::mojom::CameraMetadataTag::ANDROID_CONTROL_AWB_MODE:
      awb_mode_set_ = false;
      break;
    default:
      NOTREACHED() << "Invalid 3A mode: " << tag;
  }
}

bool Camera3AController::Is3AStabilized() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (af_mode_ !=
      cros::mojom::AndroidControlAfMode::ANDROID_CONTROL_AF_MODE_OFF) {
    if (af_state_ != cros::mojom::AndroidControlAfState::
                         ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED &&
        af_state_ != cros::mojom::AndroidControlAfState::
                         ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED) {
      return false;
    }
  }
  if (ae_mode_ !=
      cros::mojom::AndroidControlAeMode::ANDROID_CONTROL_AE_MODE_OFF) {
    if (ae_state_ != cros::mojom::AndroidControlAeState::
                         ANDROID_CONTROL_AE_STATE_CONVERGED &&
        ae_state_ != cros::mojom::AndroidControlAeState::
                         ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED) {
      return false;
    }
  }
  if (awb_mode_ ==
      cros::mojom::AndroidControlAwbMode::ANDROID_CONTROL_AWB_MODE_AUTO) {
    if (awb_state_ != cros::mojom::AndroidControlAwbState::
                          ANDROID_CONTROL_AWB_STATE_CONVERGED) {
      return false;
    }
  }
  DVLOG(1) << "3A stabilized";
  return true;
}

}  // namespace media
