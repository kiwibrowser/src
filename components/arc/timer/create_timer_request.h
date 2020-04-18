// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_CREATE_TIMER_REQUEST_H_
#define COMPONENTS_ARC_TIMER_CREATE_TIMER_REQUEST_H_

#include <stdint.h>

#include "base/files/scoped_file.h"

// Typemapping for |CreateTimerRequest| in timer.mojom
namespace arc {

struct CreateTimerRequest {
  CreateTimerRequest();
  CreateTimerRequest(CreateTimerRequest&&);
  ~CreateTimerRequest();

  int32_t clock_id;
  base::ScopedFD expiration_fd;

  DISALLOW_COPY_AND_ASSIGN(CreateTimerRequest);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TIMER_CREATE_TIMER_REQUEST_H_
