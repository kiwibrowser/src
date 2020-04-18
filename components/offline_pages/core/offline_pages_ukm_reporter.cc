// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_pages_ukm_reporter.h"

#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "url/gurl.h"

namespace offline_pages {

void OfflinePagesUkmReporter::ReportUrlOfflineRequest(const GURL& gurl,
                                                      bool foreground) {
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (ukm_recorder == nullptr)
    return;

  // This unique ID represents this whole navigation.
  int32_t source_id = ukm::UkmRecorder::GetNewSourceID();

  // Associate the URL with this navigation.
  // TODO(petewil): re-enable once crbug/792197 is addressed.
  // ukm_recorder->UpdateSourceURL(source_id, gurl);

  // Tag this metric as an offline page request for the URL.  This is a private
  // member of UkmRecorder, so we need to be friends to use it.
  ukm::builders::OfflinePages_SavePageRequested(source_id)
      .SetRequestedFromForeground(foreground ? 1 : 0)
      .Record(ukm_recorder);
}

}  // namespace offline_pages
