// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/display/output_protection_delegate.h"

#include "chrome/browser/chromeos/display/output_protection_controller_ash.h"
#include "chrome/browser/chromeos/display/output_protection_controller_mus.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"

namespace chromeos {

namespace {

bool GetCurrentDisplayId(content::RenderFrameHost* rfh, int64_t* display_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(rfh);
  DCHECK(display_id);

  display::Screen* screen = display::Screen::GetScreen();
  if (!screen)
    return false;
  display::Display display =
      screen->GetDisplayNearestView(rfh->GetNativeView());
  *display_id = display.id();
  return true;
}

}  // namespace

OutputProtectionDelegate::Controller::Controller() {}

OutputProtectionDelegate::Controller::~Controller() {}

OutputProtectionDelegate::OutputProtectionDelegate(int render_process_id,
                                                   int render_frame_id)
    : render_process_id_(render_process_id),
      render_frame_id_(render_frame_id),
      window_(nullptr),
      display_id_(display::kInvalidDisplayId),
      weak_ptr_factory_(this) {
  // This can be constructed on IO or UI thread.
}

OutputProtectionDelegate::~OutputProtectionDelegate() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (window_)
    window_->RemoveObserver(this);
}

void OutputProtectionDelegate::QueryStatus(
    const QueryStatusCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!InitializeControllerIfNecessary()) {
    callback.Run(false, 0, 0);
    return;
  }

  controller_->QueryStatus(display_id_, callback);
}

void OutputProtectionDelegate::SetProtection(
    uint32_t desired_method_mask,
    const SetProtectionCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!InitializeControllerIfNecessary()) {
    callback.Run(false);
    return;
  }
  controller_->SetProtection(display_id_, desired_method_mask, callback);
  desired_method_mask_ = desired_method_mask;
}

bool OutputProtectionDelegate::InitializeControllerIfNecessary() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (controller_)
    return true;

  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  if (!rfh) {
    DLOG(WARNING) << "RenderFrameHost is not alive.";
    return false;
  }

  int64_t display_id = display::kInvalidDisplayId;
  if (!GetCurrentDisplayId(rfh, &display_id))
    return false;

  aura::Window* window = rfh->GetNativeView();
  if (!window)
    return false;

  if (ash_util::IsRunningInMash())
    controller_ = std::make_unique<OutputProtectionControllerMus>();
  else
    controller_ = std::make_unique<OutputProtectionControllerAsh>();

  display_id_ = display_id;
  window_ = window;
  window_->AddObserver(this);
  return true;
}

void OutputProtectionDelegate::OnWindowHierarchyChanged(
    const aura::WindowObserver::HierarchyChangeParams& params) {
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  if (!rfh) {
    DLOG(WARNING) << "RenderFrameHost is not alive.";
    return;
  }

  int64_t new_display_id = display::kInvalidDisplayId;
  if (!GetCurrentDisplayId(rfh, &new_display_id))
    return;

  if (display_id_ == new_display_id)
    return;

  if (desired_method_mask_ != display::CONTENT_PROTECTION_METHOD_NONE) {
    DCHECK(controller_);
    controller_->SetProtection(new_display_id, desired_method_mask_,
                               base::DoNothing());
    controller_->SetProtection(display_id_,
                               display::CONTENT_PROTECTION_METHOD_NONE,
                               base::DoNothing());
  }
  display_id_ = new_display_id;
}

void OutputProtectionDelegate::OnWindowDestroying(aura::Window* window) {
  DCHECK_EQ(window, window_);
  window_->RemoveObserver(this);
  window_ = nullptr;
}

}  // namespace chromeos
