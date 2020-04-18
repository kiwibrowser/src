// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_EMBEDDER_RESULT_CODES_H_
#define SERVICES_SERVICE_MANAGER_EMBEDDER_RESULT_CODES_H_

namespace service_manager {

// The return code returned by the main method.
// Extended by embedders.

enum ResultCode {
  // Process terminated normally.
  RESULT_CODE_NORMAL_EXIT,

  // Last return code (keep this last).
  RESULT_CODE_LAST_CODE
};

// Embedders may rely on these result codes not changing.
static_assert(RESULT_CODE_LAST_CODE == 1, "This enum is frozen.");

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_EMBEDDER_RESULT_CODES_H_
