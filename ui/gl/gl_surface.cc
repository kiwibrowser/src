// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_surface.h"

#include <vector>

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/threading/thread_local.h"
#include "base/trace_event/trace_event.h"
#include "ui/gfx/gpu_fence.h"
#include "ui/gfx/swap_result.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_image.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface_format.h"
#include "ui/gl/gl_switches.h"

namespace gl {

namespace {
base::LazyInstance<base::ThreadLocalPointer<GLSurface>>::Leaky
    current_surface_ = LAZY_INSTANCE_INITIALIZER;
}  // namespace

GLSurface::GLSurface() {}

bool GLSurface::Initialize() {
  return Initialize(GLSurfaceFormat());
}

bool GLSurface::Initialize(GLSurfaceFormat format) {
  return true;
}

void GLSurface::PrepareToDestroy(bool have_context) {}

bool GLSurface::Resize(const gfx::Size& size,
                       float scale_factor,
                       ColorSpace color_space,
                       bool has_alpha) {
  NOTIMPLEMENTED();
  return false;
}

bool GLSurface::Recreate() {
  NOTIMPLEMENTED();
  return false;
}

bool GLSurface::DeferDraws() {
  return false;
}

bool GLSurface::SupportsPresentationCallback() {
  return false;
}

bool GLSurface::SupportsSwapBuffersWithBounds() {
  return false;
}

bool GLSurface::SupportsPostSubBuffer() {
  return false;
}

bool GLSurface::SupportsCommitOverlayPlanes() {
  return false;
}

bool GLSurface::SupportsAsyncSwap() {
  return false;
}

unsigned int GLSurface::GetBackingFramebufferObject() {
  return 0;
}

void GLSurface::SwapBuffersAsync(
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  NOTREACHED();
}

gfx::SwapResult GLSurface::SwapBuffersWithBounds(
    const std::vector<gfx::Rect>& rects,
    const PresentationCallback& callback) {
  return gfx::SwapResult::SWAP_FAILED;
}

gfx::SwapResult GLSurface::PostSubBuffer(int x,
                                         int y,
                                         int width,
                                         int height,
                                         const PresentationCallback& callback) {
  return gfx::SwapResult::SWAP_FAILED;
}

void GLSurface::PostSubBufferAsync(
    int x,
    int y,
    int width,
    int height,
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  NOTREACHED();
}

gfx::SwapResult GLSurface::CommitOverlayPlanes(
    const PresentationCallback& callback) {
  NOTREACHED();
  return gfx::SwapResult::SWAP_FAILED;
}

void GLSurface::CommitOverlayPlanesAsync(
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  NOTREACHED();
}

bool GLSurface::OnMakeCurrent(GLContext* context) {
  return true;
}

bool GLSurface::SetBackbufferAllocation(bool allocated) {
  return true;
}

void GLSurface::SetFrontbufferAllocation(bool allocated) {
}

void* GLSurface::GetShareHandle() {
  NOTIMPLEMENTED();
  return NULL;
}

void* GLSurface::GetDisplay() {
  NOTIMPLEMENTED();
  return NULL;
}

void* GLSurface::GetConfig() {
  NOTIMPLEMENTED();
  return NULL;
}

unsigned long GLSurface::GetCompatibilityKey() {
  return 0;
}

gfx::VSyncProvider* GLSurface::GetVSyncProvider() {
  return NULL;
}

void GLSurface::SetVSyncEnabled(bool enabled) {}

bool GLSurface::ScheduleOverlayPlane(int z_order,
                                     gfx::OverlayTransform transform,
                                     GLImage* image,
                                     const gfx::Rect& bounds_rect,
                                     const gfx::RectF& crop_rect,
                                     bool enable_blend,
                                     std::unique_ptr<gfx::GpuFence> gpu_fence) {
  NOTIMPLEMENTED();
  return false;
}

bool GLSurface::ScheduleCALayer(const ui::CARendererLayerParams& params) {
  NOTIMPLEMENTED();
  return false;
}

void GLSurface::ScheduleCALayerInUseQuery(
    std::vector<CALayerInUseQuery> queries) {
  NOTIMPLEMENTED();
}

bool GLSurface::ScheduleDCLayer(const ui::DCRendererLayerParams& params) {
  NOTIMPLEMENTED();
  return false;
}

bool GLSurface::SetEnableDCLayers(bool enable) {
  NOTIMPLEMENTED();
  return false;
}

bool GLSurface::IsSurfaceless() const {
  return false;
}

bool GLSurface::FlipsVertically() const {
  return false;
}

bool GLSurface::BuffersFlipped() const {
  return false;
}

bool GLSurface::SupportsDCLayers() const {
  return false;
}

bool GLSurface::UseOverlaysForVideo() const {
  return false;
}

bool GLSurface::SupportsProtectedVideo() const {
  return false;
}

bool GLSurface::SetDrawRectangle(const gfx::Rect& rect) {
  return false;
}

gfx::Vector2d GLSurface::GetDrawOffset() const {
  return gfx::Vector2d();
}

void GLSurface::WaitForSnapshotRendering() {
  // By default, just executing the SwapBuffers is normally enough.
}

void GLSurface::SetRelyOnImplicitSync() {
  // Some GLSurface derived classes might not implement this workaround while
  // still being allocated on devices where the workaround is enabled.
  // It is fine to ignore this call in those cases.
}

bool GLSurface::SupportsSwapTimestamps() const {
  return false;
}

void GLSurface::SetEnableSwapTimestamps() {
  NOTREACHED();
}

void GLSurface::SetUsePlaneGpuFences() {
  // It's fine for GLSurface derived classes to ignore the fences
  // and synchronize using other methods.
}

GLSurface* GLSurface::GetCurrent() {
  return current_surface_.Pointer()->Get();
}

GLSurface::~GLSurface() {
  if (GetCurrent() == this)
    SetCurrent(NULL);
}

void GLSurface::SetCurrent(GLSurface* surface) {
  current_surface_.Pointer()->Set(surface);
}

bool GLSurface::ExtensionsContain(const char* c_extensions, const char* name) {
  DCHECK(name);
  if (!c_extensions)
    return false;
  std::string extensions(c_extensions);
  extensions += " ";

  std::string delimited_name(name);
  delimited_name += " ";

  return extensions.find(delimited_name) != std::string::npos;
}

GLSurfaceAdapter::GLSurfaceAdapter(GLSurface* surface) : surface_(surface) {}

bool GLSurfaceAdapter::Initialize(GLSurfaceFormat format) {
  return surface_->Initialize(format);
}

void GLSurfaceAdapter::Destroy() {
  surface_->Destroy();
}

bool GLSurfaceAdapter::Resize(const gfx::Size& size,
                              float scale_factor,
                              ColorSpace color_space,
                              bool has_alpha) {
  return surface_->Resize(size, scale_factor, color_space, has_alpha);
}

bool GLSurfaceAdapter::Recreate() {
  return surface_->Recreate();
}

bool GLSurfaceAdapter::DeferDraws() {
  return surface_->DeferDraws();
}

bool GLSurfaceAdapter::IsOffscreen() {
  return surface_->IsOffscreen();
}

gfx::SwapResult GLSurfaceAdapter::SwapBuffers(
    const PresentationCallback& callback) {
  return surface_->SwapBuffers(callback);
}

void GLSurfaceAdapter::SwapBuffersAsync(
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  surface_->SwapBuffersAsync(completion_callback, presentation_callback);
}

gfx::SwapResult GLSurfaceAdapter::SwapBuffersWithBounds(
    const std::vector<gfx::Rect>& rects,
    const PresentationCallback& callback) {
  return surface_->SwapBuffersWithBounds(rects, callback);
}

gfx::SwapResult GLSurfaceAdapter::PostSubBuffer(
    int x,
    int y,
    int width,
    int height,
    const PresentationCallback& callback) {
  return surface_->PostSubBuffer(x, y, width, height, callback);
}

void GLSurfaceAdapter::PostSubBufferAsync(
    int x,
    int y,
    int width,
    int height,
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  surface_->PostSubBufferAsync(x, y, width, height, completion_callback,
                               presentation_callback);
}

gfx::SwapResult GLSurfaceAdapter::CommitOverlayPlanes(
    const PresentationCallback& callback) {
  return surface_->CommitOverlayPlanes(callback);
}

void GLSurfaceAdapter::CommitOverlayPlanesAsync(
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  surface_->CommitOverlayPlanesAsync(completion_callback,
                                     presentation_callback);
}

bool GLSurfaceAdapter::SupportsPresentationCallback() {
  return surface_->SupportsPresentationCallback();
}

bool GLSurfaceAdapter::SupportsSwapBuffersWithBounds() {
  return surface_->SupportsSwapBuffersWithBounds();
}

bool GLSurfaceAdapter::SupportsPostSubBuffer() {
  return surface_->SupportsPostSubBuffer();
}

bool GLSurfaceAdapter::SupportsCommitOverlayPlanes() {
  return surface_->SupportsCommitOverlayPlanes();
}

bool GLSurfaceAdapter::SupportsAsyncSwap() {
  return surface_->SupportsAsyncSwap();
}

gfx::Size GLSurfaceAdapter::GetSize() {
  return surface_->GetSize();
}

void* GLSurfaceAdapter::GetHandle() {
  return surface_->GetHandle();
}

unsigned int GLSurfaceAdapter::GetBackingFramebufferObject() {
  return surface_->GetBackingFramebufferObject();
}

bool GLSurfaceAdapter::OnMakeCurrent(GLContext* context) {
  return surface_->OnMakeCurrent(context);
}

bool GLSurfaceAdapter::SetBackbufferAllocation(bool allocated) {
  return surface_->SetBackbufferAllocation(allocated);
}

void GLSurfaceAdapter::SetFrontbufferAllocation(bool allocated) {
  surface_->SetFrontbufferAllocation(allocated);
}

void* GLSurfaceAdapter::GetShareHandle() {
  return surface_->GetShareHandle();
}

void* GLSurfaceAdapter::GetDisplay() {
  return surface_->GetDisplay();
}

void* GLSurfaceAdapter::GetConfig() {
  return surface_->GetConfig();
}

unsigned long GLSurfaceAdapter::GetCompatibilityKey() {
  return surface_->GetCompatibilityKey();
}

GLSurfaceFormat GLSurfaceAdapter::GetFormat() {
  return surface_->GetFormat();
}

gfx::VSyncProvider* GLSurfaceAdapter::GetVSyncProvider() {
  return surface_->GetVSyncProvider();
}

void GLSurfaceAdapter::SetVSyncEnabled(bool enabled) {
  surface_->SetVSyncEnabled(enabled);
}

bool GLSurfaceAdapter::ScheduleOverlayPlane(
    int z_order,
    gfx::OverlayTransform transform,
    GLImage* image,
    const gfx::Rect& bounds_rect,
    const gfx::RectF& crop_rect,
    bool enable_blend,
    std::unique_ptr<gfx::GpuFence> gpu_fence) {
  return surface_->ScheduleOverlayPlane(z_order, transform, image, bounds_rect,
                                        crop_rect, enable_blend,
                                        std::move(gpu_fence));
}

bool GLSurfaceAdapter::ScheduleDCLayer(
    const ui::DCRendererLayerParams& params) {
  return surface_->ScheduleDCLayer(params);
}

bool GLSurfaceAdapter::SetEnableDCLayers(bool enable) {
  return surface_->SetEnableDCLayers(enable);
}

bool GLSurfaceAdapter::IsSurfaceless() const {
  return surface_->IsSurfaceless();
}

bool GLSurfaceAdapter::FlipsVertically() const {
  return surface_->FlipsVertically();
}

bool GLSurfaceAdapter::BuffersFlipped() const {
  return surface_->BuffersFlipped();
}

bool GLSurfaceAdapter::SupportsDCLayers() const {
  return surface_->SupportsDCLayers();
}

bool GLSurfaceAdapter::UseOverlaysForVideo() const {
  return surface_->UseOverlaysForVideo();
}

bool GLSurfaceAdapter::SupportsProtectedVideo() const {
  return surface_->SupportsProtectedVideo();
}

bool GLSurfaceAdapter::SetDrawRectangle(const gfx::Rect& rect) {
  return surface_->SetDrawRectangle(rect);
}

gfx::Vector2d GLSurfaceAdapter::GetDrawOffset() const {
  return surface_->GetDrawOffset();
}

void GLSurfaceAdapter::WaitForSnapshotRendering() {
  surface_->WaitForSnapshotRendering();
}

void GLSurfaceAdapter::SetRelyOnImplicitSync() {
  surface_->SetRelyOnImplicitSync();
}

bool GLSurfaceAdapter::SupportsSwapTimestamps() const {
  return surface_->SupportsSwapTimestamps();
}

void GLSurfaceAdapter::SetEnableSwapTimestamps() {
  return surface_->SetEnableSwapTimestamps();
}

void GLSurfaceAdapter::SetUsePlaneGpuFences() {
  surface_->SetUsePlaneGpuFences();
}

GLSurfaceAdapter::~GLSurfaceAdapter() {}

scoped_refptr<GLSurface> InitializeGLSurfaceWithFormat(
    scoped_refptr<GLSurface> surface, GLSurfaceFormat format) {
  if (!surface->Initialize(format))
    return nullptr;
  return surface;
}

scoped_refptr<GLSurface> InitializeGLSurface(scoped_refptr<GLSurface> surface) {
  return InitializeGLSurfaceWithFormat(surface, GLSurfaceFormat());
}

GLSurface::CALayerInUseQuery::CALayerInUseQuery() = default;
GLSurface::CALayerInUseQuery::CALayerInUseQuery(const CALayerInUseQuery&) =
    default;
GLSurface::CALayerInUseQuery::~CALayerInUseQuery() = default;

}  // namespace gl
