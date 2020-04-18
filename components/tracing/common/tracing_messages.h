// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
#define COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_

#include <stdint.h>

#include <string>

#include "base/metrics/histogram.h"
#include "base/sync_socket.h"
#include "components/tracing/tracing_export.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_message_utils.h"
#include "ipc/ipc_platform_file.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT TRACING_EXPORT
#define IPC_MESSAGE_START TracingMsgStart

// Sent to all child processes to set tracing process id.
IPC_MESSAGE_CONTROL1(TracingMsg_SetTracingProcessId,
                     uint64_t /* Tracing process id (hash of child id) */)

IPC_MESSAGE_CONTROL4(TracingMsg_SetUMACallback,
                     std::string /* histogram_name */,
                     base::HistogramBase::Sample /* histogram_lower_value */,
                     base::HistogramBase::Sample /* histogram_uppwer_value */,
                     bool /* repeat */)

IPC_MESSAGE_CONTROL1(TracingMsg_ClearUMACallback,
                     std::string /* histogram_name */)

// Notify the browser that this child process supports tracing.
IPC_MESSAGE_CONTROL0(TracingHostMsg_ChildSupportsTracing)

IPC_MESSAGE_CONTROL1(TracingHostMsg_TriggerBackgroundTrace,
                     std::string /* name */)

IPC_MESSAGE_CONTROL0(TracingHostMsg_AbortBackgroundTrace)

#endif  // COMPONENTS_TRACING_COMMON_TRACING_MESSAGES_H_
