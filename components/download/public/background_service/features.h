// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_FEATURES_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_FEATURES_H_

#include "base/feature_list.h"

namespace download {

extern const base::Feature kDownloadServiceFeature;

// Incognito support of download service. No database or file IO is allowed if
// this feature is enabled. The download data will be saved to blob.
extern const base::Feature kDownloadServiceIncognito;

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_BACKGROUND_SERVICE_FEATURES_H_
