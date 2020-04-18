// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_GET_SESSION_NAME_WIN_H_
#define COMPONENTS_SYNC_BASE_GET_SESSION_NAME_WIN_H_

#include <string>

namespace syncer {
namespace internal {

std::string GetComputerName();

}  // namespace internal
}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_GET_SESSION_NAME_WIN_H_
