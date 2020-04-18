// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/drm_thread.h"

#include <gbm.h>
#include <utility>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "ui/display/types/display_mode.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/gfx/presentation_feedback.h"
#include "ui/ozone/platform/drm/common/drm_util.h"
#include "ui/ozone/platform/drm/gpu/drm_buffer.h"
#include "ui/ozone/platform/drm/gpu/drm_device_generator.h"
#include "ui/ozone/platform/drm/gpu/drm_device_manager.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_display_manager.h"
#include "ui/ozone/platform/drm/gpu/drm_window.h"
#include "ui/ozone/platform/drm/gpu/drm_window_proxy.h"
#include "ui/ozone/platform/drm/gpu/gbm_buffer.h"
#include "ui/ozone/platform/drm/gpu/gbm_device.h"
#include "ui/ozone/platform/drm/gpu/gbm_surface_factory.h"
#include "ui/ozone/platform/drm/gpu/proxy_helpers.h"
#include "ui/ozone/platform/drm/gpu/screen_manager.h"
#include "ui/ozone/public/ozone_switches.h"

namespace ui {

namespace {

class GbmBufferGenerator : public ScanoutBufferGenerator {
 public:
  GbmBufferGenerator() {}
  ~GbmBufferGenerator() override {}

  // ScanoutBufferGenerator:
  scoped_refptr<ScanoutBuffer> Create(const scoped_refptr<DrmDevice>& drm,
                                      uint32_t format,
                                      const std::vector<uint64_t>& modifiers,
                                      const gfx::Size& size) override {
    scoped_refptr<GbmDevice> gbm(static_cast<GbmDevice*>(drm.get()));
    if (modifiers.size() > 0) {
      return GbmBuffer::CreateBufferWithModifiers(
          gbm, format, size, GBM_BO_USE_SCANOUT, modifiers);
    } else {
      return GbmBuffer::CreateBuffer(gbm, format, size, GBM_BO_USE_SCANOUT);
    }
  }

 protected:
  DISALLOW_COPY_AND_ASSIGN(GbmBufferGenerator);
};

class GbmDeviceGenerator : public DrmDeviceGenerator {
 public:
  GbmDeviceGenerator() {}
  ~GbmDeviceGenerator() override {}

  // DrmDeviceGenerator:
  scoped_refptr<DrmDevice> CreateDevice(const base::FilePath& path,
                                        base::File file,
                                        bool is_primary_device) override {
    scoped_refptr<DrmDevice> drm =
        new GbmDevice(path, std::move(file), is_primary_device);
    if (drm->Initialize())
      return drm;

    return nullptr;
  }

 private:

  DISALLOW_COPY_AND_ASSIGN(GbmDeviceGenerator);
};

}  // namespace

DrmThread::DrmThread() : base::Thread("DrmThread"), weak_ptr_factory_(this) {}

DrmThread::~DrmThread() {
  Stop();
}

void DrmThread::Start(base::OnceClosure binding_completer) {
  complete_early_binding_requests_ = std::move(binding_completer);
  base::Thread::Options thread_options;
  thread_options.message_loop_type = base::MessageLoop::TYPE_IO;
  thread_options.priority = base::ThreadPriority::DISPLAY;

  if (!StartWithOptions(thread_options))
    LOG(FATAL) << "Failed to create DRM thread";
}

void DrmThread::Init() {
  device_manager_.reset(
      new DrmDeviceManager(std::make_unique<GbmDeviceGenerator>()));
  buffer_generator_.reset(new GbmBufferGenerator());
  screen_manager_.reset(new ScreenManager(buffer_generator_.get()));

  display_manager_.reset(
      new DrmGpuDisplayManager(screen_manager_.get(), device_manager_.get()));

  DCHECK(task_runner())
      << "DrmThread::Init -- thread doesn't have a task_runner";

  // DRM thread is running now so can safely handle binding requests. So drain
  // the queue of as-yet unhandled binding requests if there are any.
  std::move(complete_early_binding_requests_).Run();
}

void DrmThread::CreateBuffer(gfx::AcceleratedWidget widget,
                             const gfx::Size& size,
                             gfx::BufferFormat format,
                             gfx::BufferUsage usage,
                             scoped_refptr<GbmBuffer>* buffer) {
  scoped_refptr<GbmDevice> gbm =
      static_cast<GbmDevice*>(device_manager_->GetDrmDevice(widget).get());
  DCHECK(gbm);

  uint32_t flags = 0;
  switch (usage) {
    case gfx::BufferUsage::GPU_READ:
      flags = GBM_BO_USE_TEXTURING;
      break;
    case gfx::BufferUsage::SCANOUT:
      flags = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT | GBM_BO_USE_TEXTURING;
      break;
    case gfx::BufferUsage::SCANOUT_CAMERA_READ_WRITE:
      flags = GBM_BO_USE_LINEAR | GBM_BO_USE_CAMERA_WRITE | GBM_BO_USE_SCANOUT |
              GBM_BO_USE_TEXTURING;
      break;
    case gfx::BufferUsage::CAMERA_AND_CPU_READ_WRITE:
      flags =
          GBM_BO_USE_LINEAR | GBM_BO_USE_CAMERA_WRITE | GBM_BO_USE_TEXTURING;
      break;
    case gfx::BufferUsage::SCANOUT_CPU_READ_WRITE:
      flags = GBM_BO_USE_LINEAR | GBM_BO_USE_SCANOUT | GBM_BO_USE_TEXTURING;
      break;
    case gfx::BufferUsage::SCANOUT_VDA_WRITE:
      flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_TEXTURING;
      break;
    case gfx::BufferUsage::GPU_READ_CPU_READ_WRITE:
    case gfx::BufferUsage::GPU_READ_CPU_READ_WRITE_PERSISTENT:
      flags = GBM_BO_USE_LINEAR | GBM_BO_USE_TEXTURING;
      break;
  }

  DrmWindow* window = screen_manager_->GetWindow(widget);
  std::vector<uint64_t> modifiers;

  uint32_t fourcc_format = ui::GetFourCCFormatFromBufferFormat(format);

  // TODO(hoegsberg): We shouldn't really get here without a window,
  // but it happens during init. Need to figure out why.
  if (window && window->GetController())
    modifiers = window->GetController()->GetFormatModifiers(fourcc_format);

  // NOTE: BufferUsage::SCANOUT is used to create buffers that will be
  // explicitly set via kms on a CRTC (e.g: BufferQueue buffers), therefore
  // allocation should fail if it's not possible to allocate a BO_USE_SCANOUT
  // buffer in that case.
  bool retry_without_scanout = usage != gfx::BufferUsage::SCANOUT;
  do {
    if (modifiers.size() > 0 && !(flags & GBM_BO_USE_LINEAR))
      *buffer = GbmBuffer::CreateBufferWithModifiers(gbm, fourcc_format, size,
                                                     flags, modifiers);
    else
      *buffer = GbmBuffer::CreateBuffer(gbm, fourcc_format, size, flags);
    retry_without_scanout =
        retry_without_scanout && !*buffer && (flags & GBM_BO_USE_SCANOUT);
    flags &= ~GBM_BO_USE_SCANOUT;
  } while (retry_without_scanout);
}

void DrmThread::CreateBufferFromFds(
    gfx::AcceleratedWidget widget,
    const gfx::Size& size,
    gfx::BufferFormat format,
    std::vector<base::ScopedFD>&& fds,
    const std::vector<gfx::NativePixmapPlane>& planes,
    scoped_refptr<GbmBuffer>* buffer) {
  scoped_refptr<GbmDevice> gbm =
      static_cast<GbmDevice*>(device_manager_->GetDrmDevice(widget).get());
  DCHECK(gbm);
  *buffer = GbmBuffer::CreateBufferFromFds(
      gbm, ui::GetFourCCFormatFromBufferFormat(format), size, std::move(fds),
      planes);
}

void DrmThread::GetScanoutFormats(
    gfx::AcceleratedWidget widget,
    std::vector<gfx::BufferFormat>* scanout_formats) {
  display_manager_->GetScanoutFormats(widget, scanout_formats);
}

void DrmThread::SchedulePageFlip(gfx::AcceleratedWidget widget,
                                 const std::vector<OverlayPlane>& planes,
                                 SwapCompletionOnceCallback callback) {
  scoped_refptr<ui::DrmDevice> drm_device =
      device_manager_->GetDrmDevice(widget);

  drm_device->plane_manager()->RequestPlanesReadyCallback(
      planes, base::BindOnce(&DrmThread::OnPlanesReadyForPageFlip,
                             weak_ptr_factory_.GetWeakPtr(), widget, planes,
                             std::move(callback)));
}

void DrmThread::OnPlanesReadyForPageFlip(
    gfx::AcceleratedWidget widget,
    const std::vector<OverlayPlane>& planes,
    SwapCompletionOnceCallback callback) {
  DrmWindow* window = screen_manager_->GetWindow(widget);
  if (window) {
    bool result = window->SchedulePageFlip(planes, std::move(callback));
    CHECK(result) << "DrmThread::SchedulePageFlip failed.";
  } else {
    std::move(callback).Run(gfx::SwapResult::SWAP_ACK,
                            gfx::PresentationFeedback());
  }
}

void DrmThread::GetVSyncParameters(
    gfx::AcceleratedWidget widget,
    const gfx::VSyncProvider::UpdateVSyncCallback& callback) {
  DrmWindow* window = screen_manager_->GetWindow(widget);
  // No need to call the callback if there isn't a window since the vsync
  // provider doesn't require the callback to be called if there isn't a vsync
  // data source.
  if (window)
    window->GetVSyncParameters(callback);
}

void DrmThread::CreateWindow(const gfx::AcceleratedWidget& widget) {
  std::unique_ptr<DrmWindow> window(
      new DrmWindow(widget, device_manager_.get(), screen_manager_.get()));
  window->Initialize(buffer_generator_.get());
  screen_manager_->AddWindow(widget, std::move(window));
}

void DrmThread::DestroyWindow(const gfx::AcceleratedWidget& widget) {
  std::unique_ptr<DrmWindow> window = screen_manager_->RemoveWindow(widget);
  window->Shutdown();
}

void DrmThread::SetWindowBounds(const gfx::AcceleratedWidget& widget,
                                const gfx::Rect& bounds) {
  screen_manager_->GetWindow(widget)->SetBounds(bounds);
}

void DrmThread::SetCursor(const gfx::AcceleratedWidget& widget,
                          const std::vector<SkBitmap>& bitmaps,
                          const gfx::Point& location,
                          int32_t frame_delay_ms) {
  screen_manager_->GetWindow(widget)
      ->SetCursor(bitmaps, location, frame_delay_ms);
}

void DrmThread::MoveCursor(const gfx::AcceleratedWidget& widget,
                           const gfx::Point& location) {
  screen_manager_->GetWindow(widget)->MoveCursor(location);
}

void DrmThread::CheckOverlayCapabilities(
    const gfx::AcceleratedWidget& widget,
    const OverlaySurfaceCandidateList& overlays,
    base::OnceCallback<void(const gfx::AcceleratedWidget&,
                            const OverlaySurfaceCandidateList&,
                            const OverlayStatusList&)> callback) {
  TRACE_EVENT0("drm,hwoverlays", "DrmThread::CheckOverlayCapabilities");

  auto params = CreateParamsFromOverlaySurfaceCandidate(overlays);
  std::move(callback).Run(
      widget, overlays,
      CreateOverlayStatusListFrom(
          screen_manager_->GetWindow(widget)->TestPageFlip(params)));
}

void DrmThread::RefreshNativeDisplays(
    base::OnceCallback<void(MovableDisplaySnapshots)> callback) {
  std::move(callback).Run(display_manager_->GetDisplays());
}

void DrmThread::ConfigureNativeDisplay(
    int64_t id,
    std::unique_ptr<display::DisplayMode> mode,
    const gfx::Point& origin,
    base::OnceCallback<void(int64_t, bool)> callback) {
  std::move(callback).Run(
      id, display_manager_->ConfigureDisplay(id, *mode, origin));
}

void DrmThread::DisableNativeDisplay(
    int64_t id,
    base::OnceCallback<void(int64_t, bool)> callback) {
  std::move(callback).Run(id, display_manager_->DisableDisplay(id));
}

void DrmThread::TakeDisplayControl(base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(display_manager_->TakeDisplayControl());
}

void DrmThread::RelinquishDisplayControl(
    base::OnceCallback<void(bool)> callback) {
  display_manager_->RelinquishDisplayControl();
  std::move(callback).Run(true);
}

void DrmThread::AddGraphicsDevice(const base::FilePath& path, base::File file) {
  device_manager_->AddDrmDevice(path, std::move(file));
}

void DrmThread::RemoveGraphicsDevice(const base::FilePath& path) {
  device_manager_->RemoveDrmDevice(path);
}

void DrmThread::GetHDCPState(
    int64_t display_id,
    base::OnceCallback<void(int64_t, bool, display::HDCPState)> callback) {
  display::HDCPState state = display::HDCP_STATE_UNDESIRED;
  bool success = display_manager_->GetHDCPState(display_id, &state);
  std::move(callback).Run(display_id, success, state);
}

void DrmThread::SetHDCPState(int64_t display_id,
                             display::HDCPState state,
                             base::OnceCallback<void(int64_t, bool)> callback) {
  std::move(callback).Run(display_id,
                          display_manager_->SetHDCPState(display_id, state));
}

void DrmThread::SetColorCorrection(
    int64_t display_id,
    const std::vector<display::GammaRampRGBEntry>& degamma_lut,
    const std::vector<display::GammaRampRGBEntry>& gamma_lut,
    const std::vector<float>& correction_matrix) {
  display_manager_->SetColorCorrection(display_id, degamma_lut, gamma_lut,
                                       correction_matrix);
}

void DrmThread::StartDrmDevice(StartDrmDeviceCallback callback) {
  // We currently assume that |Init| always succeeds so return true to indicate
  // when the DRM thread has completed launching.  In particular, the invocation
  // of the callback in the client triggers the invocation of DRM thread
  // readiness observers.
  std::move(callback).Run(true);
}

// DrmThread requires a BindingSet instead of a simple Binding because it will
// be used from multiple threads in multiple processes.
void DrmThread::AddBindingCursorDevice(
    ozone::mojom::DeviceCursorRequest request) {
  cursor_bindings_.AddBinding(this, std::move(request));
}

void DrmThread::AddBindingDrmDevice(ozone::mojom::DrmDeviceRequest request) {
  TRACE_EVENT0("drm", "DrmThread::AddBindingDrmDevice");
  drm_bindings_.AddBinding(this, std::move(request));
}

}  // namespace ui
