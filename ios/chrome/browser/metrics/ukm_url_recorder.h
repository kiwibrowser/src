// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_METRICS_IOS_UKM_URL_RECORDER_H_
#define IOS_CHROME_BROWSER_METRICS_IOS_UKM_URL_RECORDER_H_

#include "services/metrics/public/cpp/ukm_source_id.h"

namespace web {
class WebState;
}  // namespace web

namespace ukm {

// Initializes recording of UKM source URLs for the given WebState.
void InitializeSourceUrlRecorderForWebState(web::WebState* web_state);

// Gets a UKM SourceId for the currently committed document of web state.
// Returns kInvalidSourceId if no commit has been observed.
SourceId GetSourceIdForWebStateDocument(web::WebState* web_state);

}  // namespace ukm

#endif  // IOS_CHROME_BROWSER_METRICS_IOS_UKM_URL_RECORDER_H_
