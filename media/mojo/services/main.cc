// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "media/mojo/services/media_service_factory.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/service_runner.h"

MojoResult ServiceMain(MojoHandle mojo_handle) {
  // Enable logging.
  service_manager::ServiceRunner::InitBaseCommandLine();

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);

  std::unique_ptr<service_manager::Service> service =
      media::CreateMediaServiceForTesting();
  service_manager::ServiceRunner runner(service.release());
  return runner.Run(mojo_handle, false /* init_base */);
}
