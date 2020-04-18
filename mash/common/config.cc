// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mash/common/config.h"

#include "base/command_line.h"

#if defined(OS_CHROMEOS)
#include "ash/public/interfaces/constants.mojom.h"  // nogncheck
#endif

namespace mash {
namespace common {

const char kWindowManagerSwitch[] = "window-manager";

std::string GetWindowManagerServiceName() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kWindowManagerSwitch)) {
    std::string service_name =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            kWindowManagerSwitch);
    return service_name;
  }
#if defined(OS_CHROMEOS)
  return ash::mojom::kServiceName;
#else
  LOG(FATAL)
      << "You must specify a window manager e.g. --window-manager=simple_wm";
  return std::string();
#endif
}

}  // namespace common
}  // namespace mash
