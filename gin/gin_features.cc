// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gin/gin_features.h"

namespace features {

// Enables optimization of JavaScript in V8.
const base::Feature kV8OptimizeJavascript{"V8OptimizeJavascript",
                                          base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace features
