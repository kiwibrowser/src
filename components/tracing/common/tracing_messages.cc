// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Get basic type definitions.
#define IPC_MESSAGE_IMPL
#undef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#include "components/tracing/common/tracing_messages.h"
#ifndef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#error "Failed to include components/tracing/common/tracing_messages.h"
#endif

// Generate constructors.
#include "ipc/struct_constructor_macros.h"
#undef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#include "components/tracing/common/tracing_messages.h"
#ifndef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#error "Failed to include components/tracing/common/tracing_messages.h"
#endif

// Generate destructors.
#include "ipc/struct_destructor_macros.h"
#undef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#include "components/tracing/common/tracing_messages.h"
#ifndef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#error "Failed to include components/tracing/common/tracing_messages.h"
#endif

// Generate param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#undef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#include "components/tracing/common/tracing_messages.h"
#ifndef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#error "Failed to include components/tracing/common/tracing_messages.h"
#endif
}  // namespace IPC

// Generate param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#undef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#include "components/tracing/common/tracing_messages.h"
#ifndef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#error "Failed to include components/tracing/common/tracing_messages.h"
#endif
}  // namespace IPC

// Generate param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#undef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#include "components/tracing/common/tracing_messages.h"
#ifndef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#error "Failed to include components/tracing/common/tracing_messages.h"
#endif
}  // namespace IPC
