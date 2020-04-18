// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "remoting/test/app_remoting_test_driver_environment.h"

namespace remoting {
namespace test {

std::unique_ptr<AppRemotingTestDriverEnvironment>
CreateAppRemotingTestDriverEnvironment(
    const AppRemotingTestDriverEnvironment::EnvironmentOptions& options) {
  return base::WrapUnique<AppRemotingTestDriverEnvironment>(
      new AppRemotingTestDriverEnvironment(options));
}

}  // namespace test
}  // namespace remoting
