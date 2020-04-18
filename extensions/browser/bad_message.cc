// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/bad_message.h"

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/render_process_host.h"

namespace extensions {
namespace bad_message {

namespace {

void LogBadMessage(BadMessageReason reason) {
  // TODO(creis): We should add a crash key similar to the "bad_message_reason"
  // key logged in content::bad_message::ReceivedBadMessage, for disambiguating
  // multiple kills in the same method.  It's important not to overlap with the
  // content::bad_message::BadMessageReason enum values.
  LOG(ERROR) << "Terminating extension renderer for bad IPC message, reason "
             << reason;
  base::UmaHistogramSparse("Stability.BadMessageTerminated.Extensions", reason);
}

}  // namespace

void ReceivedBadMessage(content::RenderProcessHost* host,
                        BadMessageReason reason) {
  LogBadMessage(reason);
  host->ShutdownForBadMessage(
      content::RenderProcessHost::CrashReportMode::GENERATE_CRASH_DUMP);
}

void ReceivedBadMessage(int render_process_id, BadMessageReason reason) {
  content::RenderProcessHost* rph =
      content::RenderProcessHost::FromID(render_process_id);
  // The render process was already terminated.
  if (!rph)
    return;

  ReceivedBadMessage(rph, reason);
}

void ReceivedBadMessage(content::BrowserMessageFilter* filter,
                        BadMessageReason reason) {
  LogBadMessage(reason);
  filter->ShutdownForBadMessage();
}

}  // namespace bad_message
}  // namespace extensions
