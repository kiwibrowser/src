// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/shared_impl/ppapi_features.h"

namespace ppapi {
namespace features {

// With this feature you can turn back on the deprecated StreamToFile API (which
// lets you stream a URL request directly to a file).
// TODO(https://crbug.com/823522): Remove the feature and flag entirely.
const base::Feature kStreamToFile{"PPAPIStreamToFile",
                                  base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
}  // namespace ppapi
