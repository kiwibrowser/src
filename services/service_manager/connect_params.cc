// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/connect_params.h"

namespace service_manager {

ConnectParams::ConnectParams() {}
ConnectParams::~ConnectParams() {
  if (!start_service_callback_.is_null())
    std::move(start_service_callback_).Run(result_, resolved_identity_);
}

}  // namespace service_manager
