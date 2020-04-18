// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/get_session_name_linux.h"

#include <limits.h>  // for HOST_NAME_MAX
#include <unistd.h>  // for gethostname()

#include "base/linux_util.h"

namespace syncer {
namespace internal {

std::string GetHostname() {
  char hostname[HOST_NAME_MAX];
  if (gethostname(hostname, HOST_NAME_MAX) == 0)  // Success.
    return hostname;
  return base::GetLinuxDistro();
}

}  // namespace internal
}  // namespace syncer
