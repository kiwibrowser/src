// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MASH_COMMON_CONFIG_H_
#define MASH_COMMON_CONFIG_H_

#include <string>

namespace mash {
namespace common {

// This file contains configuration functions that can be used by any mash
// service.

// Returns the name of the window manager service to run. By default this will
// return "ash". A different value can be specified on the command line by
// passing --window-manager=<foo>.
std::string GetWindowManagerServiceName();

}  // namespace common
}  // namespace mash

#endif  // MASH_COMMON_CONFIG_H_
