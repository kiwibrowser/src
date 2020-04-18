// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/core/tab_restore_service.h"

#include "base/trace_event/memory_usage_estimator.h"

namespace sessions {

// TimeFactory-----------------------------------------------------------------

TabRestoreService::TimeFactory::~TimeFactory() {}

// Entry ----------------------------------------------------------------------

TabRestoreService::Entry::~Entry() = default;

TabRestoreService::Entry::Entry(Type type)
    : id(SessionID::NewUnique()), type(type) {}

size_t TabRestoreService::Entry::EstimateMemoryUsage() const {
  return 0;
}

TabRestoreService::Tab::Tab() : Entry(TAB) {}
TabRestoreService::Tab::~Tab() = default;

size_t TabRestoreService::Tab::EstimateMemoryUsage() const {
  using base::trace_event::EstimateMemoryUsage;
  return
      EstimateMemoryUsage(navigations) +
      EstimateMemoryUsage(extension_app_id) +
      EstimateMemoryUsage(user_agent_override);
}

TabRestoreService::Window::Window() : Entry(WINDOW) {}
TabRestoreService::Window::~Window() = default;

size_t TabRestoreService::Window::EstimateMemoryUsage() const {
  using base::trace_event::EstimateMemoryUsage;
  return
      EstimateMemoryUsage(tabs) +
      EstimateMemoryUsage(app_name);
}

// TabRestoreService ----------------------------------------------------------

TabRestoreService::~TabRestoreService() {
}

// PlatformSpecificTabData
// ------------------------------------------------------

PlatformSpecificTabData::~PlatformSpecificTabData() {}

}  // namespace sessions
