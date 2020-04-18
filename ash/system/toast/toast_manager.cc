// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/toast/toast_manager.h"

#include <algorithm>

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"

namespace ash {

namespace {

// Minimum duration for a toast to be visible (in millisecond).
const int32_t kMinimumDurationMs = 200;

}  // anonymous namespace

ToastManager::ToastManager() : weak_ptr_factory_(this) {}

ToastManager::~ToastManager() = default;

void ToastManager::Show(const ToastData& data) {
  const std::string& id = data.id;
  DCHECK(!id.empty());

  if (current_toast_id_ == id) {
    // TODO(yoshiki): Replaces the visible toast.
    return;
  }

  auto existing_toast =
      std::find_if(queue_.begin(), queue_.end(),
                   [&id](const ToastData& data) { return data.id == id; });

  if (existing_toast == queue_.end()) {
    queue_.emplace_back(data);
  } else {
    *existing_toast = data;
  }

  if (queue_.size() == 1 && overlay_ == nullptr)
    ShowLatest();
}

void ToastManager::Cancel(const std::string& id) {
  if (id == current_toast_id_) {
    overlay_->Show(false);
    return;
  }

  auto cancelled_toast =
      std::find_if(queue_.begin(), queue_.end(),
                   [&id](const ToastData& data) { return data.id == id; });
  if (cancelled_toast != queue_.end())
    queue_.erase(cancelled_toast);
}

void ToastManager::OnClosed() {
  overlay_.reset();
  current_toast_id_.clear();

  // Show the next toast if available.
  if (!queue_.empty())
    ShowLatest();
}

void ToastManager::ShowLatest() {
  DCHECK(!overlay_);

  const ToastData data = std::move(queue_.front());
  queue_.pop_front();

  current_toast_id_ = data.id;
  serial_++;

  overlay_.reset(new ToastOverlay(this, data.text, data.dismiss_text));
  overlay_->Show(true);

  if (data.duration_ms != ToastData::kInfiniteDuration) {
    int32_t duration_ms = std::max(data.duration_ms, kMinimumDurationMs);
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::Bind(&ToastManager::OnDurationPassed,
                              weak_ptr_factory_.GetWeakPtr(), serial_),
        base::TimeDelta::FromMilliseconds(duration_ms));
  }
}

void ToastManager::OnDurationPassed(int toast_number) {
  if (overlay_ && serial_ == toast_number)
    overlay_->Show(false);
}

}  // namespace ash
