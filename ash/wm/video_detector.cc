// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/video_detector.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/wm/window_state.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/wm/core/window_util.h"

namespace ash {

VideoDetector::VideoDetector(viz::mojom::VideoDetectorObserverRequest request)
    : state_(State::NOT_PLAYING),
      video_is_playing_(false),
      window_observer_manager_(this),
      scoped_session_observer_(this),
      is_shutting_down_(false),
      binding_(this, std::move(request)) {
  aura::Env::GetInstance()->AddObserver(this);
  Shell::Get()->AddShellObserver(this);
}

VideoDetector::~VideoDetector() {
  Shell::Get()->RemoveShellObserver(this);
  aura::Env::GetInstance()->RemoveObserver(this);
}

void VideoDetector::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void VideoDetector::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void VideoDetector::OnWindowInitialized(aura::Window* window) {
  window_observer_manager_.Add(window);
}

void VideoDetector::OnWindowDestroying(aura::Window* window) {
  if (fullscreen_root_windows_.count(window)) {
    window_observer_manager_.Remove(window);
    fullscreen_root_windows_.erase(window);
    UpdateState();
  }
}

void VideoDetector::OnWindowDestroyed(aura::Window* window) {
  window_observer_manager_.Remove(window);
}

void VideoDetector::OnChromeTerminating() {
  // Stop checking video activity once the shutdown
  // process starts. crbug.com/231696.
  is_shutting_down_ = true;
}

void VideoDetector::OnFullscreenStateChanged(bool is_fullscreen,
                                             aura::Window* root_window) {
  if (is_fullscreen && !fullscreen_root_windows_.count(root_window)) {
    fullscreen_root_windows_.insert(root_window);
    if (!window_observer_manager_.IsObserving(root_window))
      window_observer_manager_.Add(root_window);
    UpdateState();
  } else if (!is_fullscreen && fullscreen_root_windows_.count(root_window)) {
    fullscreen_root_windows_.erase(root_window);
    window_observer_manager_.Remove(root_window);
    UpdateState();
  }
}

void VideoDetector::UpdateState() {
  State new_state = State::NOT_PLAYING;
  if (video_is_playing_) {
    new_state = fullscreen_root_windows_.empty() ? State::PLAYING_WINDOWED
                                                 : State::PLAYING_FULLSCREEN;
  }

  if (state_ != new_state) {
    state_ = new_state;
    for (auto& observer : observers_)
      observer.OnVideoStateChanged(state_);
  }
}

void VideoDetector::OnVideoActivityStarted() {
  if (is_shutting_down_)
    return;
  video_is_playing_ = true;
  UpdateState();
}

void VideoDetector::OnVideoActivityEnded() {
  video_is_playing_ = false;
  UpdateState();
}

}  // namespace ash
