// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/service.h"
#include "services/audio/service_factory.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/service_runner.h"


MojoResult ServiceMain(MojoHandle service_request_handle) {
  return service_manager::ServiceRunner(
             audio::CreateStandaloneService().release())
      .Run(service_request_handle);
}
