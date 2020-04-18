// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_FEATURES_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_FEATURES_H_

#include "base/feature_list.h"
#include "components/download/public/common/download_export.h"

namespace download {
namespace features {

// Whether a download can be handled by parallel jobs.
COMPONENTS_DOWNLOAD_EXPORT extern const base::Feature kParallelDownloading;

}  // namespace features
}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_FEATURES_H_
