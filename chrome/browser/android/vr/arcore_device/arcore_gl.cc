// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr/arcore_device/arcore_gl.h"

#include <algorithm>
#include <limits>
#include <utility>
#include "base/android/android_hardware_buffer_compat.h"
#include "base/android/jni_android.h"
#include "base/callback_helpers.h"
#include "base/containers/queue.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event_argument.h"
#include "chrome/browser/android/vr/arcore_device/ar_image_transport.h"
#include "chrome/browser/android/vr/arcore_device/arcore_impl.h"
#include "chrome/browser/android/vr/mailbox_to_surface_bridge.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl_android_hardware_buffer.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/angle_conversions.h"
#include "ui/gfx/gpu_fence.h"
#include "ui/gl/android/scoped_java_surface.h"
#include "ui/gl/android/surface_texture.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_fence_egl.h"
#include "ui/gl/gl_image_ahardwarebuffer.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/init/gl_factory.h"

namespace device {

namespace {
// Input display coordinates (range 0..1) used with ARCore's
// transformDisplayUvCoords to calculate the output matrix.
constexpr std::array<float, 6> kDisplayCoordinatesForTransform = {
    0.f, 0.f, 1.f, 0.f, 0.f, 1.f};

gfx::Transform ConvertUvsToTransformMatrix(const std::vector<float>& uvs) {
  // We're creating a matrix that transforms viewport UV coordinates (for a
  // screen-filling quad, origin at bottom left, u=1 at right, v=1 at top) to
  // camera texture UV coordinates. This matrix is used with
  // vr::WebVrRenderer to compute texture coordinates for copying an
  // appropriately cropped and rotated subsection of the camera image. The
  // SampleData is a bit unfortunate. ARCore doesn't provide a way to get a
  // matrix directly. There's a function to transform UV vectors individually,
  // which obviously can't be used from a shader, so we run that on selected
  // vectors and recreate the matrix from the result.

  // Assumes that |uvs| is the result of transforming the display coordinates
  // from kDisplayCoordinatesForTransform. This combines the solved matrix with
  // a Y flip because ARCore's "normalized screen space" coordinates have the
  // origin at the top left to match 2D Android APIs, so it needs a Y flip to
  // get an origin at bottom left as used for textures.
  DCHECK(uvs.size() == 6);
  float u00 = uvs[0];
  float v00 = uvs[1];
  float u10 = uvs[2];
  float v10 = uvs[3];
  float u01 = uvs[4];
  float v01 = uvs[5];

  // Transform initializes to the identity matrix and then is modified by uvs.
  gfx::Transform result;
  result.matrix().set(0, 0, u10 - u00);
  result.matrix().set(0, 1, -(u01 - u00));
  result.matrix().set(0, 3, u01);
  result.matrix().set(1, 0, v10 - v00);
  result.matrix().set(1, 1, -(v01 - v00));
  result.matrix().set(1, 3, v01);
  return result;
}

}  // namespace

ARCoreGl::ARCoreGl(std::unique_ptr<vr::MailboxToSurfaceBridge> mailbox_bridge)
    : gl_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      arcore_(std::make_unique<ARCoreImpl>()),
      ar_image_transport_(
          std::make_unique<ARImageTransport>(std::move(mailbox_bridge))),
      weak_ptr_factory_(this) {}

ARCoreGl::~ARCoreGl() {}

bool ARCoreGl::Initialize() {
  DCHECK(IsOnGlThread());
  DCHECK(!is_initialized_);

  if (gl::GetGLImplementation() == gl::kGLImplementationNone &&
      !gl::init::InitializeGLOneOff()) {
    DLOG(ERROR) << "gl::init::InitializeGLOneOff failed";
    return false;
  }

  scoped_refptr<gl::GLSurface> surface =
      gl::init::CreateOffscreenGLSurface(gfx::Size());
  if (!surface.get()) {
    DLOG(ERROR) << "gl::init::CreateOffscreenGLSurface failed";
    return false;
  }

  scoped_refptr<gl::GLContext> context =
      gl::init::CreateGLContext(nullptr, surface.get(), gl::GLContextAttribs());
  if (!context.get()) {
    DLOG(ERROR) << "gl::init::CreateGLContext failed";
    return false;
  }
  if (!context->MakeCurrent(surface.get())) {
    DLOG(ERROR) << "gl::GLContext::MakeCurrent() failed";
    return false;
  }

  if (!arcore_->Initialize()) {
    DLOG(ERROR) << "ARCore failed to initialize";

    return false;
  }

  if (!ar_image_transport_->Initialize()) {
    DLOG(ERROR) << "ARImageTransport failed to initialize";
    return false;
  }
  // Set the texture on ARCore to render the camera.
  arcore_->SetCameraTexture(ar_image_transport_->GetCameraTextureId());

  // Assign the surface and context members now that initialization has
  // succeeded.
  surface_ = std::move(surface);
  context_ = std::move(context);

  is_initialized_ = true;
  return true;
}

void ARCoreGl::ProduceFrame(
    const gfx::Size& frame_size,
    display::Display::Rotation display_rotation,
    mojom::VRMagicWindowProvider::GetFrameDataCallback callback) {
  TRACE_EVENT0("gpu", __FUNCTION__);
  DCHECK(IsOnGlThread());
  DCHECK(is_initialized_);

  // Set display geometry before calling Update. It's a pending request that
  // applies to the next frame.
  // TODO(klausw): Only call if there was a change, this may be an expensive
  // operation. If there was no change, the previous projection matrix and UV
  // transform remain valid.
  gfx::Size transfer_size = frame_size;
  arcore_->SetDisplayGeometry(transfer_size, display_rotation);

  TRACE_EVENT_BEGIN0("gpu", "ARCore Update");
  mojom::VRPosePtr ar_pose = arcore_->Update();
  TRACE_EVENT_END0("gpu", "ARCore Update");
  if (!ar_pose) {
    DLOG(ERROR) << "Failed get pose from arcore_->Update()!";
    std::move(callback).Run(nullptr);
    return;
  }

  // Get the UV transform matrix from ARCore's UV transform. TODO(klausw): do
  // this only on changes, not every frame.
  std::vector<float> uvs_transformed =
      arcore_->TransformDisplayUvCoords(kDisplayCoordinatesForTransform);
  gfx::Transform uv_transform = ConvertUvsToTransformMatrix(uvs_transformed);

  // Transfer the camera image texture to a MailboxHolder for transport to
  // the renderer process.
  gpu::MailboxHolder buffer_holder =
      ar_image_transport_->TransferFrame(transfer_size, uv_transform);

  // Create the frame data to return to the renderer.
  mojom::VRMagicWindowFrameDataPtr frame_data =
      mojom::VRMagicWindowFrameData::New();
  frame_data->pose = std::move(ar_pose);
  frame_data->buffer_holder = buffer_holder;
  frame_data->buffer_size = transfer_size;
  frame_data->time_delta = base::TimeTicks::Now() - base::TimeTicks();
  // We need near/far distances to make a projection matrix. The actual
  // values don't matter, the Renderer will recalculate dependent values
  // based on the application's near/far settngs.
  constexpr float depth_near = 0.1f;
  constexpr float depth_far = 1000.f;
  gfx::Transform projection =
      arcore_->GetProjectionMatrix(depth_near, depth_far);
  // Convert the Transform's 4x4 matrix to 16 floats in column-major order.
  frame_data->projection_matrix.resize(16);
  projection.matrix().asColMajorf(&frame_data->projection_matrix[0]);

  fps_meter_.AddFrame(base::TimeTicks::Now());
  TRACE_COUNTER1("gpu", "WebXR FPS", fps_meter_.GetFPS());

  std::move(callback).Run(std::move(frame_data));
}

bool ARCoreGl::IsOnGlThread() const {
  return gl_thread_task_runner_->BelongsToCurrentThread();
}

base::WeakPtr<ARCoreGl> ARCoreGl::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace device
