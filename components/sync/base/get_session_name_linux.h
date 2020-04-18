// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_GET_SESSION_NAME_LINUX_H_
#define COMPONENTS_SYNC_BASE_GET_SESSION_NAME_LINUX_H_

#include <string>

namespace syncer {
namespace internal {

std::string GetHostname();

}  // namespace internal
}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_GET_SESSION_NAME_LINUX_H_
