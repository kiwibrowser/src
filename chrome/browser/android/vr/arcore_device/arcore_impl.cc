// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr/arcore_device/arcore_impl.h"

#include "base/android/jni_android.h"
#include "base/bind.h"
#include "base/numerics/math_constants.h"
#include "base/optional.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/android/vr/arcore_device/arcore_java_utils.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "ui/display/display.h"

using base::android::JavaRef;

namespace device {

ARCoreImpl::ARCoreImpl()
    : gl_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {}

ARCoreImpl::~ARCoreImpl() = default;

bool ARCoreImpl::Initialize() {
  DCHECK(IsOnGlThread());
  DCHECK(!arcore_session_.is_valid());

  // TODO(https://crbug.com/837944): Notify error earlier if this will fail.

  JNIEnv* env = base::android::AttachCurrentThread();
  if (!env) {
    DLOG(ERROR) << "Unable to get JNIEnv for ARCore";
    return false;
  }

  // Get the activity context.
  base::android::ScopedJavaLocalRef<jobject> context =
      vr::ArCoreJavaUtils::GetApplicationContext();
  if (!context.obj()) {
    DLOG(ERROR) << "Unable to retrieve the Java context/activity!";
    return false;
  }

  if (!vr::ArCoreJavaUtils::EnsureLoaded()) {
    DLOG(ERROR) << "ARCore could not be loaded.";
    return false;
  }

  // Use a local scoped ArSession for the next steps, we want the
  // arcore_session_ member to remain null until we complete successful
  // initialization.
  internal::ScopedArCoreObject<ArSession*> session;

  ArStatus status = ArSession_create(env, context.obj(), session.receive());
  if (status != AR_SUCCESS) {
    DLOG(ERROR) << "ArSession_create failed: " << status;
    return false;
  }

  internal::ScopedArCoreObject<ArConfig*> arcore_config;
  ArConfig_create(session.get(), arcore_config.receive());
  if (!arcore_config.is_valid()) {
    DLOG(ERROR) << "ArConfig_create failed";
    return false;
  }

  // We just use the default config.
  status = ArSession_checkSupported(session.get(), arcore_config.get());
  if (status != AR_SUCCESS) {
    DLOG(ERROR) << "ArSession_checkSupported failed: " << status;
    return false;
  }

  status = ArSession_configure(session.get(), arcore_config.get());
  if (status != AR_SUCCESS) {
    DLOG(ERROR) << "ArSession_configure failed: " << status;
    return false;
  }

  ArFrame_create(session.get(), arcore_frame_.receive());
  if (!arcore_frame_.is_valid()) {
    DLOG(ERROR) << "ArFrame_create failed";
    return false;
  }

  // Success, we now have a valid session.
  arcore_session_ = std::move(session);
  return true;
}

void ARCoreImpl::SetCameraTexture(GLuint camera_texture_id) {
  DCHECK(IsOnGlThread());
  DCHECK(arcore_session_.is_valid());
  ArSession_setCameraTextureName(arcore_session_.get(), camera_texture_id);
}

void ARCoreImpl::SetDisplayGeometry(
    const gfx::Size& frame_size,
    display::Display::Rotation display_rotation) {
  DCHECK(IsOnGlThread());
  DCHECK(arcore_session_.is_valid());
  // Display::Rotation is the same as Android's rotation and is compatible with
  // what ARCore is expecting.
  ArSession_setDisplayGeometry(arcore_session_.get(), display_rotation,
                               frame_size.width(), frame_size.height());
}

std::vector<float> ARCoreImpl::TransformDisplayUvCoords(
    const base::span<const float> uvs) {
  DCHECK(IsOnGlThread());
  DCHECK(arcore_session_.is_valid());
  DCHECK(arcore_frame_.is_valid());

  size_t num_elements = uvs.size();
  DCHECK(num_elements % 2 == 0);
  std::vector<float> uvs_out(num_elements);
  ArFrame_transformDisplayUvCoords(arcore_session_.get(), arcore_frame_.get(),
                                   num_elements, &uvs[0], &uvs_out[0]);
  return uvs_out;
}

mojom::VRPosePtr ARCoreImpl::Update() {
  DCHECK(IsOnGlThread());
  DCHECK(arcore_session_.is_valid());
  DCHECK(arcore_frame_.is_valid());

  ArStatus status;
  if (!is_tracking_) {
    status = ArSession_resume(arcore_session_.get());
    if (status != AR_SUCCESS) {
      DLOG(ERROR) << "ArSession_resume failed: " << status;
      return nullptr;
    }
    is_tracking_ = true;
  }

  status = ArSession_update(arcore_session_.get(), arcore_frame_.get());
  if (status != AR_SUCCESS) {
    DLOG(ERROR) << "ArSession_update failed: " << status;
    return nullptr;
  }

  internal::ScopedArCoreObject<ArCamera*> arcore_camera;
  ArFrame_acquireCamera(arcore_session_.get(), arcore_frame_.get(),
                        arcore_camera.receive());
  if (!arcore_camera.is_valid()) {
    DLOG(ERROR) << "ArFrame_acquireCamera failed!";
    return nullptr;
  }

  ArTrackingState tracking_state;
  ArCamera_getTrackingState(arcore_session_.get(), arcore_camera.get(),
                            &tracking_state);
  if (tracking_state != AR_TRACKING_STATE_TRACKING) {
    DLOG(ERROR) << "Tracking state is not AR_TRACKING_STATE_TRACKING: "
                << tracking_state;
    return nullptr;
  }

  internal::ScopedArCoreObject<ArPose*> arcore_pose;
  ArPose_create(arcore_session_.get(), nullptr, arcore_pose.receive());
  if (!arcore_pose.is_valid()) {
    DLOG(ERROR) << "ArPose_create failed!";
    return nullptr;
  }

  ArCamera_getDisplayOrientedPose(arcore_session_.get(), arcore_camera.get(),
                                  arcore_pose.get());
  float pose_raw[7];  // 7 = orientation(4) + position(3).
  ArPose_getPoseRaw(arcore_session_.get(), arcore_pose.get(), pose_raw);

  mojom::VRPosePtr pose = mojom::VRPose::New();
  pose->orientation.emplace(pose_raw, pose_raw + 4);
  pose->position.emplace(pose_raw + 4, pose_raw + 7);

  return pose;
}

gfx::Transform ARCoreImpl::GetProjectionMatrix(float near, float far) {
  DCHECK(IsOnGlThread());
  DCHECK(arcore_session_.is_valid());
  DCHECK(arcore_frame_.is_valid());

  internal::ScopedArCoreObject<ArCamera*> arcore_camera;
  ArFrame_acquireCamera(arcore_session_.get(), arcore_frame_.get(),
                        arcore_camera.receive());
  DCHECK(arcore_camera.is_valid())
      << "ArFrame_acquireCamera failed despite documentation saying it cannot";

  // ARCore's projection matrix is 16 floats in column-major order.
  float matrix_4x4[16];
  ArCamera_getProjectionMatrix(arcore_session_.get(), arcore_camera.get(), near,
                               far, matrix_4x4);
  gfx::Transform result;
  result.matrix().setColMajorf(matrix_4x4);
  return result;
}

bool ARCoreImpl::IsOnGlThread() {
  return gl_thread_task_runner_->BelongsToCurrentThread();
}

}  // namespace device
