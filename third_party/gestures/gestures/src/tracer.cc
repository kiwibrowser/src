// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/tracer.h"

#include <string.h>

namespace gestures {

Tracer::Tracer(PropRegistry* prop_reg, WriteFn write_fn)
    : write_fn_(write_fn),
      tracing_enabled_(prop_reg, "Tracing Enabled", false) {}

void Tracer::Trace(const char* message, const char* name) {
  if (tracing_enabled_.val_ && write_fn_) {
    char write_msg[1024];
    size_t len = strlen(message);
    size_t len2 = strlen(name);
    if ((len + len2) >= sizeof(write_msg)) {
      strcpy(write_msg, "Error!! Gestures Library: Message too long!!");
    } else {
      strcpy(write_msg, message);
      strcpy(write_msg + len, name);
    }
    (*write_fn_)(write_msg);
  }
}
}  // namespace gestures
