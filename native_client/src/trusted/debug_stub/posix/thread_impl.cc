/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/debug_stub/thread.h"

/*
 * Define the OS specific portions of Thread.
 */

namespace port {

int Thread::ExceptionToSignal(int exception_code) {
  return exception_code;
}

}  // namespace port

