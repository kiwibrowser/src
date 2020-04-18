// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TRACING_BACKGROUND_TRACING_FIELD_TRIAL_H_
#define CHROME_BROWSER_TRACING_BACKGROUND_TRACING_FIELD_TRIAL_H_

#include <string>

namespace tracing {

void SetupBackgroundTracingFieldTrial();

typedef void (*ConfigTextFilterForTesting)(std::string*);
void SetConfigTextFilterForTesting(ConfigTextFilterForTesting predicate);

}  // namespace tracing

#endif  // CHROME_BROWSER_TRACING_BACKGROUND_TRACING_FIELD_TRIAL_H_
