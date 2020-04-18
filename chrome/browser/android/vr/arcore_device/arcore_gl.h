// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_GL_H_
#define CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_GL_H_

#include <memory>
#include <utility>
#include <vector>
#include "base/cancelable_callback.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "chrome/browser/vr/fps_meter.h"
#include "chrome/browser/vr/renderers/web_vr_renderer.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/display/display.h"
#include "ui/gfx/geometry/quaternion.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/native_widget_types.h"

namespace gl {
class GLContext;
class GLSurface;
}  // namespace gl

namespace vr {
class MailboxToSurfaceBridge;
}  // namespace vr

namespace device {

class ARCore;
class ARImageTransport;

// All of this class's methods must be called on the same valid GL thread with
// the exception of GetGlThreadTaskRunner() and GetWeakPtr().
class ARCoreGl {
 public:
  explicit ARCoreGl(std::unique_ptr<vr::MailboxToSurfaceBridge> mailbox_bridge);
  ~ARCoreGl();

  bool Initialize();

  void ProduceFrame(const gfx::Size& frame_size,
                    display::Display::Rotation display_rotation,
                    mojom::VRMagicWindowProvider::GetFrameDataCallback);

  const scoped_refptr<base::SingleThreadTaskRunner>& GetGlThreadTaskRunner() {
    return gl_thread_task_runner_;
  }

  base::WeakPtr<ARCoreGl> GetWeakPtr();

 private:
  bool IsOnGlThread() const;

  scoped_refptr<gl::GLSurface> surface_;
  scoped_refptr<gl::GLContext> context_;
  scoped_refptr<base::SingleThreadTaskRunner> gl_thread_task_runner_;

  // Created on GL thread and should only be accessed on that thread.
  std::unique_ptr<ARCore> arcore_;
  std::unique_ptr<ARImageTransport> ar_image_transport_;

  bool is_initialized_ = false;

  vr::FPSMeter fps_meter_;
  // Must be last.
  base::WeakPtrFactory<ARCoreGl> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(ARCoreGl);
};

}  // namespace device

#endif  // CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_GL_H_
