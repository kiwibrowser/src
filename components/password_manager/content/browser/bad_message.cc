// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/content/browser/bad_message.h"

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "content/public/browser/render_process_host.h"

namespace password_manager {
namespace bad_message {

void ReceivedBadMessage(content::RenderProcessHost* host,
                        BadMessageReason reason) {
  LOG(ERROR)
      << "Terminating renderer for bad PasswordManager IPC message, reason "
      << static_cast<int>(reason);
  base::UmaHistogramSparse("Stability.BadMessageTerminated.PasswordManager",
                           static_cast<int>(reason));
  host->ShutdownForBadMessage(
      content::RenderProcessHost::CrashReportMode::GENERATE_CRASH_DUMP);
}

}  // namespace bad_message
}  // namespace password_manager
