// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/test/fake_window.h"

#include "android_webview/browser/browser_view_renderer.h"
#include "android_webview/browser/child_frame.h"
#include "android_webview/browser/render_thread_manager.h"
#include "base/location.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/init/gl_factory.h"

namespace android_webview {

class FakeWindow::ScopedMakeCurrent {
 public:
  ScopedMakeCurrent(FakeWindow* view_root) : view_root_(view_root) {
    DCHECK(!view_root_->context_current_);
    view_root_->context_current_ = true;
    bool result = view_root_->context_->MakeCurrent(view_root_->surface_.get());
    DCHECK(result);
  };

  ~ScopedMakeCurrent() {
    DCHECK(view_root_->context_current_);
    view_root_->context_current_ = false;

    // Release the underlying EGLContext. This is required because the real
    // GLContextEGL may no longer be current here and to satisfy DCHECK in
    // GLContextEGL::IsCurrent.
    eglMakeCurrent(view_root_->surface_->GetDisplay(), EGL_NO_SURFACE,
                   EGL_NO_SURFACE, EGL_NO_CONTEXT);
    view_root_->context_->ReleaseCurrent(view_root_->surface_.get());
  }

 private:
  FakeWindow* view_root_;
};

FakeWindow::FakeWindow(BrowserViewRenderer* view,
                       WindowHooks* hooks,
                       gfx::Rect location)
    : view_(view),
      hooks_(hooks),
      surface_size_(100, 100),
      location_(location),
      on_draw_hardware_pending_(false),
      context_current_(false),
      weak_ptr_factory_(this) {
  CheckCurrentlyOnUIThread();
  DCHECK(view_);
  view_->OnAttachedToWindow(location_.width(), location_.height());
  view_->SetWindowVisibility(true);
  view_->SetViewVisibility(true);
}

FakeWindow::~FakeWindow() {
  CheckCurrentlyOnUIThread();
  if (render_thread_loop_) {
    base::WaitableEvent completion(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    render_thread_loop_->PostTask(
        FROM_HERE, base::BindOnce(&FakeWindow::DestroyOnRT,
                                  base::Unretained(this), &completion));
    completion.Wait();
  }

  render_thread_.reset();
}

void FakeWindow::Detach() {
  CheckCurrentlyOnUIThread();
  view_->OnDetachedFromWindow();
}

void FakeWindow::RequestInvokeGL(FakeFunctor* functor,
                                 bool wait_for_completion) {
  CheckCurrentlyOnUIThread();
  base::WaitableEvent completion(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  render_thread_loop_->PostTask(
      FROM_HERE,
      base::BindOnce(&FakeWindow::InvokeFunctorOnRT, base::Unretained(this),
                     functor, wait_for_completion ? &completion : nullptr));
  if (wait_for_completion)
    completion.Wait();
}

void FakeWindow::InvokeFunctorOnRT(FakeFunctor* functor,
                                   base::WaitableEvent* sync) {
  CheckCurrentlyOnRT();
  ScopedMakeCurrent make_current(this);
  functor->Invoke(hooks_);
  if (sync)
    sync->Signal();
}

void FakeWindow::RequestDrawGL(FakeFunctor* functor) {
  CheckCurrentlyOnUIThread();
  render_thread_loop_->PostTask(
      FROM_HERE, base::BindOnce(&FakeWindow::ProcessDrawOnRT,
                                base::Unretained(this), functor));
}

void FakeWindow::PostInvalidate() {
  CheckCurrentlyOnUIThread();
  if (on_draw_hardware_pending_)
    return;
  on_draw_hardware_pending_ = true;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&FakeWindow::OnDrawHardware,
                                weak_ptr_factory_.GetWeakPtr()));
}

void FakeWindow::OnDrawHardware() {
  CheckCurrentlyOnUIThread();
  DCHECK(on_draw_hardware_pending_);
  on_draw_hardware_pending_ = false;

  view_->PrepareToDraw(gfx::Vector2d(), location_);
  hooks_->WillOnDraw();
  bool success = view_->OnDrawHardware();
  hooks_->DidOnDraw(success);
  FakeFunctor* functor = hooks_->GetFunctor();
  if (success && functor) {
    CreateRenderThreadIfNeeded();

    base::WaitableEvent completion(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    render_thread_loop_->PostTask(
        FROM_HERE,
        base::BindOnce(&FakeWindow::DrawFunctorOnRT, base::Unretained(this),
                       functor, &completion));
    completion.Wait();
  }
}

void FakeWindow::ProcessSyncOnRT(FakeFunctor* functor,
                                 base::WaitableEvent* sync) {
  CheckCurrentlyOnRT();
  functor->Sync(location_, hooks_);
  sync->Signal();
}

void FakeWindow::ProcessDrawOnRT(FakeFunctor* functor) {
  CheckCurrentlyOnRT();
  ScopedMakeCurrent make_current(this);
  functor->Draw(hooks_);
}

void FakeWindow::DrawFunctorOnRT(FakeFunctor* functor,
                                 base::WaitableEvent* sync) {
  ProcessSyncOnRT(functor, sync);
  ProcessDrawOnRT(functor);
}

void FakeWindow::CheckCurrentlyOnUIThread() {
  DCHECK(ui_checker_.CalledOnValidSequence());
}

void FakeWindow::CreateRenderThreadIfNeeded() {
  CheckCurrentlyOnUIThread();
  if (render_thread_) {
    DCHECK(render_thread_loop_);
    return;
  }
  render_thread_.reset(new base::Thread("TestRenderThread"));
  render_thread_->Start();
  render_thread_loop_ = render_thread_->task_runner();
  rt_checker_.DetachFromSequence();

  base::WaitableEvent completion(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  render_thread_loop_->PostTask(
      FROM_HERE, base::BindOnce(&FakeWindow::InitializeOnRT,
                                base::Unretained(this), &completion));
  completion.Wait();
}

void FakeWindow::InitializeOnRT(base::WaitableEvent* sync) {
  CheckCurrentlyOnRT();
  surface_ = gl::init::CreateOffscreenGLSurface(surface_size_);
  DCHECK(surface_);
  DCHECK(surface_->GetHandle());
  context_ = gl::init::CreateGLContext(nullptr, surface_.get(),
                                       gl::GLContextAttribs());
  DCHECK(context_);
  sync->Signal();
}

void FakeWindow::DestroyOnRT(base::WaitableEvent* sync) {
  CheckCurrentlyOnRT();
  if (context_) {
    DCHECK(!context_->IsCurrent(surface_.get()));
    context_ = nullptr;
    surface_ = nullptr;
  }
  sync->Signal();
}

void FakeWindow::CheckCurrentlyOnRT() {
  DCHECK(rt_checker_.CalledOnValidSequence());
}

FakeFunctor::FakeFunctor() : window_(nullptr) {}

FakeFunctor::~FakeFunctor() {
  render_thread_manager_.reset();
}

void FakeFunctor::Init(
    FakeWindow* window,
    std::unique_ptr<RenderThreadManager> render_thread_manager) {
  window_ = window;
  render_thread_manager_ = std::move(render_thread_manager);
  callback_ =
      base::BindRepeating(&RenderThreadManager::DrawGL,
                          base::Unretained(render_thread_manager_.get()));
}

void FakeFunctor::Sync(const gfx::Rect& location,
                       WindowHooks* hooks) {
  DCHECK(!callback_.is_null());
  committed_location_ = location;
  AwDrawGLInfo sync_info;
  sync_info.version = kAwDrawGLInfoVersion;
  sync_info.mode = AwDrawGLInfo::kModeSync;
  hooks->WillSyncOnRT();
  callback_.Run(&sync_info);
  hooks->DidSyncOnRT();
}

void FakeFunctor::Draw(WindowHooks* hooks) {
  DCHECK(!callback_.is_null());
  AwDrawGLInfo draw_info;
  draw_info.version = kAwDrawGLInfoVersion;
  draw_info.mode = AwDrawGLInfo::kModeDraw;
  draw_info.clip_left = committed_location_.x();
  draw_info.clip_top = committed_location_.y();
  draw_info.clip_right = committed_location_.x() + committed_location_.width();
  draw_info.clip_bottom =
      committed_location_.y() + committed_location_.height();
  if (!hooks->WillDrawOnRT(&draw_info))
    return;
  callback_.Run(&draw_info);
  hooks->DidDrawOnRT();
}

CompositorFrameConsumer* FakeFunctor::GetCompositorFrameConsumer() {
  return render_thread_manager_.get();
}

void FakeFunctor::Invoke(WindowHooks* hooks) {
  DCHECK(!callback_.is_null());
  AwDrawGLInfo invoke_info;
  invoke_info.version = kAwDrawGLInfoVersion;
  invoke_info.mode = AwDrawGLInfo::kModeProcess;
  hooks->WillProcessOnRT();
  callback_.Run(&invoke_info);
  hooks->DidProcessOnRT();
}

bool FakeFunctor::RequestInvokeGL(bool wait_for_completion) {
  DCHECK(window_);
  window_->RequestInvokeGL(this, wait_for_completion);
  return true;
}

void FakeFunctor::DetachFunctorFromView() {}

}  // namespace android_webview
