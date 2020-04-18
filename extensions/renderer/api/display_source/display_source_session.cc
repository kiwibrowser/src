// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/api/display_source/display_source_session.h"

#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_WIFI_DISPLAY)
#include "extensions/renderer/api/display_source/wifi_display/wifi_display_session.h"
#endif

namespace extensions {

DisplaySourceSessionParams::DisplaySourceSessionParams()
    : auth_method(api::display_source::AUTHENTICATION_METHOD_NONE) {
}

DisplaySourceSessionParams::DisplaySourceSessionParams(
    const DisplaySourceSessionParams&) = default;

DisplaySourceSessionParams::~DisplaySourceSessionParams() = default;

DisplaySourceSession::DisplaySourceSession()
    : state_(Idle) {
}

DisplaySourceSession::~DisplaySourceSession() = default;

void DisplaySourceSession::SetNotificationCallbacks(
    const base::Closure& terminated_callback,
    const ErrorCallback& error_callback) {
  DCHECK(terminated_callback_.is_null());
  DCHECK(error_callback_.is_null());

  terminated_callback_ = terminated_callback;
  error_callback_ = error_callback;
}

std::unique_ptr<DisplaySourceSession>
DisplaySourceSessionFactory::CreateSession(
    const DisplaySourceSessionParams& params) {
#if BUILDFLAG(ENABLE_WIFI_DISPLAY)
  return std::unique_ptr<DisplaySourceSession>(new WiFiDisplaySession(params));
#else
  return nullptr;
#endif
}

}  // namespace extensions
