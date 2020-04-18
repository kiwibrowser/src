// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_OPENVR_RENDER_LOOP_H
#define DEVICE_VR_OPENVR_RENDER_LOOP_H

#include "base/memory/scoped_refptr.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "third_party/openvr/src/headers/openvr.h"
#include "ui/gfx/geometry/rect_f.h"

#if defined(OS_WIN)
#include "device/vr/windows/d3d11_texture_helper.h"
#endif

namespace device {

class OpenVRRenderLoop : public base::Thread, mojom::VRPresentationProvider {
 public:
  OpenVRRenderLoop(vr::IVRSystem* vr);
  ~OpenVRRenderLoop() override;

  void RequestPresent(
      mojom::VRSubmitFrameClientPtrInfo submit_client_info,
      mojom::VRPresentationProviderRequest request,
      device::mojom::VRRequestPresentOptionsPtr present_options,
      device::mojom::VRDisplayHost::RequestPresentCallback callback);
  void ExitPresent();
  base::WeakPtr<OpenVRRenderLoop> GetWeakPtr();

  // VRPresentationProvider overrides:
  void SubmitFrameMissing(int16_t frame_index, const gpu::SyncToken&) override;
  void SubmitFrame(int16_t frame_index,
                   const gpu::MailboxHolder& mailbox,
                   base::TimeDelta time_waited) override;
  void SubmitFrameDrawnIntoTexture(int16_t frame_index,
                                   const gpu::SyncToken&,
                                   base::TimeDelta time_waited) override;
  void SubmitFrameWithTextureHandle(int16_t frame_index,
                                    mojo::ScopedHandle texture_handle) override;
  void UpdateLayerBounds(int16_t frame_id,
                         const gfx::RectF& left_bounds,
                         const gfx::RectF& right_bounds,
                         const gfx::Size& source_size) override;
  void GetVSync(GetVSyncCallback callback) override;

 private:
  // base::Thread overrides:
  void Init() override;
  void CleanUp() override;

  mojom::VRPosePtr GetPose();
  std::vector<mojom::XRInputSourceStatePtr> GetInputState(
      vr::TrackedDevicePose_t* poses,
      uint32_t count);

  struct InputActiveState {
    bool active;
    bool primary_input_pressed;
    vr::ETrackedDeviceClass device_class;
    vr::ETrackedControllerRole controller_role;
  };

#if defined(OS_WIN)
  D3D11TextureHelper texture_helper_;
#endif

  int16_t next_frame_id_ = 0;
  bool is_presenting_ = false;
  bool report_webxr_input_ = false;
  InputActiveState input_active_states_[vr::k_unMaxTrackedDeviceCount];
  gfx::RectF left_bounds_;
  gfx::RectF right_bounds_;
  gfx::Size source_size_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  vr::IVRSystem* vr_system_;
  vr::IVRCompositor* vr_compositor_;
  mojom::VRSubmitFrameClientPtr submit_client_;
  mojo::Binding<mojom::VRPresentationProvider> binding_;
  base::WeakPtrFactory<OpenVRRenderLoop> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OpenVRRenderLoop);
};

}  // namespace device

#endif  // DEVICE_VR_OPENVR_RENDER_LOOP_H
