// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Get basic type definitions.
#include "chrome/common/common_param_traits.h"

// Generate param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#undef CHROME_COMMON_COMMON_PARAM_TRAITS_MACROS_H_
#include "chrome/common/common_param_traits_macros.h"
}  // namespace IPC

// Generate param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#undef CHROME_COMMON_COMMON_PARAM_TRAITS_MACROS_H_
#include "chrome/common/common_param_traits_macros.h"
}  // namespace IPC

// Generate param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#undef CHROME_COMMON_COMMON_PARAM_TRAITS_MACROS_H_
#include "chrome/common/common_param_traits_macros.h"
}  // namespace IPC
