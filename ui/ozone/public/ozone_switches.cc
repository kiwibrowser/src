// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/public/ozone_switches.h"

namespace switches {

// Specify ozone platform implementation to use.
const char kOzonePlatform[] = "ozone-platform";

// Specify location for image dumps.
const char kOzoneDumpFile[] = "ozone-dump-file";

// Use mojo communication in the drm platform instead of paramtraits. Remove
// this switch (and associated code) when the drm platform always uses mojo
// communication.
const char kEnableDrmMojo[] = "enable-drm-mojo";

}  // namespace switches
