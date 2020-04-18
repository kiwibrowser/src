// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mouse_watcher.h"

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_handler.h"
#include "ui/events/event_utils.h"
#include "ui/events/platform_event.h"
#include "ui/views/event_monitor.h"

namespace views {

// Amount of time between when the mouse moves outside the Host's zone and when
// the listener is notified.
const int kNotifyListenerTimeMs = 300;

class MouseWatcher::Observer : public ui::EventHandler {
 public:
  explicit Observer(MouseWatcher* mouse_watcher)
      : mouse_watcher_(mouse_watcher),
        event_monitor_(EventMonitor::CreateApplicationMonitor(this)),
        notify_listener_factory_(this) {
  }

  // ui::EventHandler implementation:
  void OnMouseEvent(ui::MouseEvent* event) override {
    switch (event->type()) {
      case ui::ET_MOUSE_MOVED:
      case ui::ET_MOUSE_DRAGGED:
        HandleMouseEvent(MouseWatcherHost::MOUSE_MOVE);
        break;
      case ui::ET_MOUSE_EXITED:
        HandleMouseEvent(MouseWatcherHost::MOUSE_EXIT);
        break;
      case ui::ET_MOUSE_PRESSED:
        HandleMouseEvent(MouseWatcherHost::MOUSE_PRESS);
        break;
      default:
        break;
    }
  }

 private:
  MouseWatcherHost* host() const { return mouse_watcher_->host_.get(); }

  // Called when a mouse event we're interested is seen.
  void HandleMouseEvent(MouseWatcherHost::MouseEventType event_type) {
    // It's safe to use last_mouse_location() here as this function is invoked
    // during event dispatching.
    if (!host()->Contains(EventMonitor::GetLastMouseLocation(), event_type)) {
      if (event_type == MouseWatcherHost::MOUSE_PRESS) {
        NotifyListener();
      } else if (!notify_listener_factory_.HasWeakPtrs()) {
        // Mouse moved outside the host's zone, start a timer to notify the
        // listener.
        base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
            FROM_HERE,
            base::BindOnce(&Observer::NotifyListener,
                           notify_listener_factory_.GetWeakPtr()),
            event_type == MouseWatcherHost::MOUSE_MOVE
                ? base::TimeDelta::FromMilliseconds(kNotifyListenerTimeMs)
                : mouse_watcher_->notify_on_exit_time_);
      }
    } else {
      // Mouse moved quickly out of the host and then into it again, so cancel
      // the timer.
      notify_listener_factory_.InvalidateWeakPtrs();
    }
  }

  void NotifyListener() {
    mouse_watcher_->NotifyListener();
    // WARNING: we've been deleted.
  }

 private:
  MouseWatcher* mouse_watcher_;
  std::unique_ptr<views::EventMonitor> event_monitor_;

  // A factory that is used to construct a delayed callback to the listener.
  base::WeakPtrFactory<Observer> notify_listener_factory_;

  DISALLOW_COPY_AND_ASSIGN(Observer);
};

MouseWatcherListener::~MouseWatcherListener() {
}

MouseWatcherHost::~MouseWatcherHost() {
}

MouseWatcher::MouseWatcher(std::unique_ptr<MouseWatcherHost> host,
                           MouseWatcherListener* listener)
    : host_(std::move(host)),
      listener_(listener),
      notify_on_exit_time_(
          base::TimeDelta::FromMilliseconds(kNotifyListenerTimeMs)) {}

MouseWatcher::~MouseWatcher() {
}

void MouseWatcher::Start() {
  if (!is_observing())
    observer_ = std::make_unique<Observer>(this);
}

void MouseWatcher::Stop() {
  observer_.reset();
}

void MouseWatcher::NotifyListener() {
  observer_.reset();
  listener_->MouseMovedOutOfHost();
}

}  // namespace views
