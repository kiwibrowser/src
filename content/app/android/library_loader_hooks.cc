// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/app/android/library_loader_hooks.h"

#include "base/android/library_loader/library_loader_hooks.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "components/tracing/common/trace_startup.h"
#include "content/common/content_constants_internal.h"

namespace content {

bool LibraryLoaded(JNIEnv* env, jclass clazz) {
  // Enable startup tracing asap to avoid early TRACE_EVENT calls being ignored.
  tracing::EnableStartupTracingIfNeeded();

  // Android's main browser loop is custom so we set the browser
  // name here as early as possible.
  base::trace_event::TraceLog::GetInstance()->set_process_name("Browser");
  base::trace_event::TraceLog::GetInstance()->SetProcessSortIndex(
      kTraceEventBrowserProcessSortIndex);

  // Can only use event tracing after setting up the command line.
  TRACE_EVENT0("jni", "JNI_OnLoad continuation");

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);
  // To view log output with IDs and timestamps use "adb logcat -v threadtime".
  logging::SetLogItems(false,    // Process ID
                       false,    // Thread ID
                       false,    // Timestamp
                       false);   // Tick count
  VLOG(0) << "Chromium logging enabled: level = " << logging::GetMinLogLevel()
          << ", default verbosity = " << logging::GetVlogVerbosity();

  return true;
}

}  // namespace content
