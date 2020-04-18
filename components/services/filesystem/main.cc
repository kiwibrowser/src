// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/c/main.h"
#include "base/macros.h"
#include "components/services/filesystem/file_system_app.h"
#include "services/service_manager/public/cpp/service_runner.h"

MojoResult ServiceMain(MojoHandle request) {
  service_manager::ServiceRunner runner(new filesystem::FileSystemApp());
  return runner.Run(request);
}
