// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/thread_watcher_report_hang.h"

// We disable optimizations for the whole file so the compiler doesn't merge
// them all together.
MSVC_DISABLE_OPTIMIZE()
MSVC_PUSH_DISABLE_WARNING(4748)

#include "base/debug/debugger.h"
#include "base/debug/dump_without_crashing.h"

namespace metrics {

// The following are unique function names for forcing the crash when a thread
// is unresponsive. This makes it possible to tell from the callstack alone what
// thread was unresponsive.
NOINLINE void ReportThreadHang() {
  volatile const char* inhibit_comdat = __func__;
  ALLOW_UNUSED_LOCAL(inhibit_comdat);
#if defined(NDEBUG)
  base::debug::DumpWithoutCrashing();
#else
  base::debug::BreakDebugger();
#endif
}

#if !defined(OS_ANDROID)

NOINLINE void StartupHang() {
  volatile int inhibit_comdat = __LINE__;
  ALLOW_UNUSED_LOCAL(inhibit_comdat);
  // TODO(rtenneti): http://crbug.com/440885 enable crashing after fixing false
  // positive startup hang data.
  // ReportThreadHang();
}

NOINLINE void ShutdownHang() {
  volatile int inhibit_comdat = __LINE__;
  ALLOW_UNUSED_LOCAL(inhibit_comdat);
  ReportThreadHang();
}

#endif  // !defined(OS_ANDROID)

NOINLINE void ThreadUnresponsive_UI() {
  volatile int inhibit_comdat = __LINE__;
  ALLOW_UNUSED_LOCAL(inhibit_comdat);
  ReportThreadHang();
}

NOINLINE void ThreadUnresponsive_IO() {
  volatile int inhibit_comdat = __LINE__;
  ALLOW_UNUSED_LOCAL(inhibit_comdat);
  ReportThreadHang();
}

NOINLINE void CrashBecauseThreadWasUnresponsive(
    content::BrowserThread::ID thread_id) {
  switch (thread_id) {
    case content::BrowserThread::UI:
      return ThreadUnresponsive_UI();
    case content::BrowserThread::IO:
      return ThreadUnresponsive_IO();
    case content::BrowserThread::ID_COUNT:
      NOTREACHED();
      break;
  }
}

}  // namespace metrics

MSVC_POP_WARNING()
MSVC_ENABLE_OPTIMIZE();
