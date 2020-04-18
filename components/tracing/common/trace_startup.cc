// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/tracing/common/trace_startup.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/trace_log.h"
#include "components/tracing/common/trace_startup_config.h"
#include "components/tracing/common/trace_to_console.h"
#include "components/tracing/common/tracing_switches.h"

namespace tracing {

void EnableStartupTracingIfNeeded() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  // Ensure TraceLog is initialized first.
  // https://crbug.com/764357
  base::trace_event::TraceLog::GetInstance();

  if (TraceStartupConfig::GetInstance()->IsEnabled()) {
    const base::trace_event::TraceConfig& trace_config =
        TraceStartupConfig::GetInstance()->GetTraceConfig();
    uint8_t modes = base::trace_event::TraceLog::RECORDING_MODE;
    if (!trace_config.event_filters().empty())
      modes |= base::trace_event::TraceLog::FILTERING_MODE;
    // This checks kTraceConfigFile switch.
    base::trace_event::TraceLog::GetInstance()->SetEnabled(
        TraceStartupConfig::GetInstance()->GetTraceConfig(), modes);
  } else if (command_line.HasSwitch(switches::kTraceToConsole)) {
    base::trace_event::TraceConfig trace_config = GetConfigForTraceToConsole();
    LOG(ERROR) << "Start " << switches::kTraceToConsole
               << " with CategoryFilter '"
               << trace_config.ToCategoryFilterString() << "'.";
    base::trace_event::TraceLog::GetInstance()->SetEnabled(
        trace_config, base::trace_event::TraceLog::RECORDING_MODE);
  }
}

}  // namespace tracing
