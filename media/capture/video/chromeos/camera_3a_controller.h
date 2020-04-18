// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_CHROMEOS_CAMERA_3A_CONTROLLER_H_
#define MEDIA_CAPTURE_VIDEO_CHROMEOS_CAMERA_3A_CONTROLLER_H_

#include <unordered_set>

#include "media/base/media_export.h"
#include "media/capture/video/chromeos/mojo/camera3.mojom.h"
#include "media/capture/video/chromeos/stream_buffer_manager.h"

namespace media {

// A class to control the auto-exposure, auto-focus, and auto-white-balancing
// operations and modes of the camera.  For the detailed state transitions for
// auto-exposure, auto-focus, and auto-white-balancing, see
// https://source.android.com/devices/camera/camera3_3Amodes
class CAPTURE_EXPORT Camera3AController
    : public CaptureMetadataDispatcher::ResultMetadataObserver {
 public:
  Camera3AController(const cros::mojom::CameraMetadataPtr& static_metadata,
                     CaptureMetadataDispatcher* capture_metadata_dispatcher,
                     scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~Camera3AController() final;

  // Trigger the camera to start exposure, focus, and white-balance metering and
  // lock them for still capture.
  void Stabilize3AForStillCapture(base::OnceClosure on_3a_stabilized_callback);

  // CaptureMetadataDispatcher::ResultMetadataObserver implementation.
  void OnResultMetadataAvailable(
      const cros::mojom::CameraMetadataPtr& result_metadata) final;

  // Enable the auto-focus mode suitable for still capture.
  void SetAutoFocusModeForStillCapture();

  // Enable the auto-focus mode suitable for video recording.
  void SetAutoFocusModeForVideoRecording();

  base::WeakPtr<Camera3AController> GetWeakPtr();

 private:
  void Set3AMode(cros::mojom::CameraMetadataTag tag, uint8_t target_mode);
  bool Is3AStabilized();

  CaptureMetadataDispatcher* capture_metadata_dispatcher_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  std::unordered_set<cros::mojom::AndroidControlAfMode> available_af_modes_;
  cros::mojom::AndroidControlAfMode af_mode_;
  cros::mojom::AndroidControlAfState af_state_;
  // |af_mode_set_| is set to true when the AF mode is synchronized between the
  // HAL and the Camera3AController.
  bool af_mode_set_;

  std::unordered_set<cros::mojom::AndroidControlAeMode> available_ae_modes_;
  cros::mojom::AndroidControlAeMode ae_mode_;
  cros::mojom::AndroidControlAeState ae_state_;
  // |ae_mode_set_| is set to true when the AE mode is synchronized between the
  // HAL and the Camera3AController.
  bool ae_mode_set_;

  std::unordered_set<cros::mojom::AndroidControlAwbMode> available_awb_modes_;
  cros::mojom::AndroidControlAwbMode awb_mode_;
  cros::mojom::AndroidControlAwbState awb_state_;
  // |awb_mode_set_| is set to true when the AWB mode is synchronized between
  // the HAL and the Camera3AController.
  bool awb_mode_set_;

  base::OnceClosure on_3a_mode_set_callback_;

  base::OnceClosure on_3a_stabilized_callback_;

  base::WeakPtrFactory<Camera3AController> weak_ptr_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(Camera3AController);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CHROMEOS_CAMERA_3A_CONTROLLER_H_
