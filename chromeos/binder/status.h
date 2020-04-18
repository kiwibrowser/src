// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_STATUS_H_
#define CHROMEOS_BINDER_STATUS_H_

#include <errno.h>
#include <stdint.h>

namespace binder {

// Status code.
// Using the same values as used by libbinder.
enum class Status : int32_t {
  OK = 0,

  UNKNOWN_ERROR = INT32_MIN,

  NO_MEMORY = -ENOMEM,
  INVALID_OPERATION = -ENOSYS,
  BAD_VALUE = -EINVAL,
  BAD_TYPE = (UNKNOWN_ERROR + 1),
  NAME_NOT_FOUND = -ENOENT,
  PERMISSION_DENIED = -EPERM,
  NO_INIT = -ENODEV,
  ALREADY_EXISTS = -EEXIST,
  DEAD_OBJECT = -EPIPE,
  FAILED_TRANSACTION = (UNKNOWN_ERROR + 2),
  BAD_INDEX = -EOVERFLOW,
  NOT_ENOUGH_DATA = -ENODATA,
  WOULD_BLOCK = -EWOULDBLOCK,
  TIMED_OUT = -ETIMEDOUT,
  UNKNOWN_TRANSACTION = -EBADMSG,
  FDS_NOT_ALLOWED = (UNKNOWN_ERROR + 7),
  UNEXPECTED_NULL = (UNKNOWN_ERROR + 8),
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_STATUS_H_
