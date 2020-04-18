// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UKM_CONTENT_SOURCE_URL_RECORDER_H_
#define COMPONENTS_UKM_CONTENT_SOURCE_URL_RECORDER_H_

#include "services/metrics/public/cpp/ukm_source_id.h"

namespace content {
class WebContents;
}  // namespace content

namespace ukm {

// Initializes recording of UKM source URLs for the given WebContents.
void InitializeSourceUrlRecorderForWebContents(
    content::WebContents* web_contents);

// Get a UKM SourceId for the currently committed document of web contents.
// Returns kInvalidSourceId if no commit has been observed.
SourceId GetSourceIdForWebContentsDocument(
    const content::WebContents* web_contents);

}  // namespace ukm

#endif  // COMPONENTS_UKM_CONTENT_SOURCE_URL_RECORDER_H_
