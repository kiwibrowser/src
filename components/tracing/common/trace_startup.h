// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRACING_COMMON_TRACE_STARTUP_H
#define COMPONENTS_TRACING_COMMON_TRACE_STARTUP_H

#include "components/tracing/tracing_export.h"

namespace tracing {

// Enables TraceLog with config based on the command line flags of the process.
void TRACING_EXPORT EnableStartupTracingIfNeeded();

}  // namespace tracing

#endif  // COMPONENTS_TRACING_COMMON_TRACE_STARTUP_H
