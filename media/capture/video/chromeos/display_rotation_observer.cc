// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/chromeos/display_rotation_observer.h"

#include <utility>

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"

namespace media {

ScreenObserverDelegate::ScreenObserverDelegate(
    DisplayRotationObserver* observer,
    scoped_refptr<base::SingleThreadTaskRunner> display_task_runner)
    : observer_(observer),
      display_task_runner_(std::move(display_task_runner)),
      delegate_task_runner_(base::ThreadTaskRunnerHandle::Get()) {
  display_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&ScreenObserverDelegate::AddObserverOnDisplayThread,
                     this));
}

void ScreenObserverDelegate::RemoveObserver() {
  DCHECK(delegate_task_runner_->BelongsToCurrentThread());
  observer_ = NULL;
  display_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&ScreenObserverDelegate::RemoveObserverOnDisplayThread,
                     this));
}

ScreenObserverDelegate::~ScreenObserverDelegate() {
  DCHECK(!observer_);
}

void ScreenObserverDelegate::OnDisplayAdded(
    const display::Display& /*new_display*/) {}

void ScreenObserverDelegate::OnDisplayRemoved(
    const display::Display& /*old_display*/) {}

void ScreenObserverDelegate::OnDisplayMetricsChanged(
    const display::Display& display,
    uint32_t metrics) {
  DCHECK(display_task_runner_->BelongsToCurrentThread());
  if (!(metrics & DISPLAY_METRIC_ROTATION))
    return;
  SendDisplayRotation(display);
}

void ScreenObserverDelegate::AddObserverOnDisplayThread() {
  DCHECK(display_task_runner_->BelongsToCurrentThread());
  display::Screen* screen = display::Screen::GetScreen();
  if (screen) {
    screen->AddObserver(this);
    SendDisplayRotation(screen->GetPrimaryDisplay());
  }
}

void ScreenObserverDelegate::RemoveObserverOnDisplayThread() {
  DCHECK(display_task_runner_->BelongsToCurrentThread());
  display::Screen* screen = display::Screen::GetScreen();
  if (screen) {
    screen->RemoveObserver(this);
  }
}

// Post the screen rotation change from the UI thread to capture thread
void ScreenObserverDelegate::SendDisplayRotation(
    const display::Display& display) {
  DCHECK(display_task_runner_->BelongsToCurrentThread());
  delegate_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ScreenObserverDelegate::SendDisplayRotationOnCaptureThread, this,
          display));
}

void ScreenObserverDelegate::SendDisplayRotationOnCaptureThread(
    const display::Display& display) {
  DCHECK(delegate_task_runner_->BelongsToCurrentThread());
  if (observer_)
    observer_->SetDisplayRotation(display);
}

}  // namespace media
