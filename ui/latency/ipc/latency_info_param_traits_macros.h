// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_LATENCY_IPC_LATENCY_INFO_PARAM_TRAITS_MACROS_H_
#define UI_LATENCY_IPC_LATENCY_INFO_PARAM_TRAITS_MACROS_H_

#include "ipc/ipc_message_macros.h"
#include "ui/latency/latency_info.h"

IPC_ENUM_TRAITS_MAX_VALUE(ui::LatencyComponentType,
                          ui::LATENCY_COMPONENT_TYPE_LAST)

IPC_STRUCT_TRAITS_BEGIN(ui::LatencyInfo::LatencyComponent)
  IPC_STRUCT_TRAITS_MEMBER(event_time)
  IPC_STRUCT_TRAITS_MEMBER(event_count)
  IPC_STRUCT_TRAITS_MEMBER(first_event_time)
  IPC_STRUCT_TRAITS_MEMBER(last_event_time)
IPC_STRUCT_TRAITS_END()

IPC_ENUM_TRAITS_MAX_VALUE(ui::SourceEventType,
                          ui::SourceEventType::SOURCE_EVENT_TYPE_LAST)

#endif  // UI_LATENCY_IPC_LATENCY_INFO_PARAM_TRAITS_MACROS_H_
