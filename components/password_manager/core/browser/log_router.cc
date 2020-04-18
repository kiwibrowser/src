// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/log_router.h"

#include "base/stl_util.h"
#include "components/password_manager/core/browser/log_manager.h"
#include "components/password_manager/core/browser/log_receiver.h"

namespace password_manager {

LogRouter::LogRouter() = default;

LogRouter::~LogRouter() = default;

void LogRouter::ProcessLog(const std::string& text) {
  // This may not be called when there are no receivers (i.e., the router is
  // inactive), because in that case the logs cannot be displayed.
  DCHECK(receivers_.might_have_observers());
  accumulated_logs_.append(text);
  for (LogReceiver& receiver : receivers_)
    receiver.LogSavePasswordProgress(text);
}

bool LogRouter::RegisterManager(LogManager* manager) {
  DCHECK(manager);
  managers_.AddObserver(manager);
  return receivers_.might_have_observers();
}

void LogRouter::UnregisterManager(LogManager* manager) {
  DCHECK(managers_.HasObserver(manager));
  managers_.RemoveObserver(manager);
}

std::string LogRouter::RegisterReceiver(LogReceiver* receiver) {
  DCHECK(receiver);
  DCHECK(accumulated_logs_.empty() || receivers_.might_have_observers());

  if (!receivers_.might_have_observers()) {
    for (LogManager& manager : managers_)
      manager.OnLogRouterAvailabilityChanged(true);
  }
  receivers_.AddObserver(receiver);
  return accumulated_logs_;
}

void LogRouter::UnregisterReceiver(LogReceiver* receiver) {
  DCHECK(receivers_.HasObserver(receiver));
  receivers_.RemoveObserver(receiver);
  if (!receivers_.might_have_observers()) {
    // |accumulated_logs_| can become very long; use the swap instead of clear()
    // to ensure that the memory is freed.
    std::string().swap(accumulated_logs_);
    for (LogManager& manager : managers_)
      manager.OnLogRouterAvailabilityChanged(false);
  }
}

}  // namespace password_manager
