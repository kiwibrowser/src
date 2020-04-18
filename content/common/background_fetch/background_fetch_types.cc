// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/background_fetch/background_fetch_types.h"

namespace content {

IconDefinition::IconDefinition() = default;

IconDefinition::IconDefinition(const IconDefinition& other) = default;

IconDefinition::~IconDefinition() = default;

BackgroundFetchOptions::BackgroundFetchOptions() = default;

BackgroundFetchOptions::BackgroundFetchOptions(
    const BackgroundFetchOptions& other) = default;

BackgroundFetchOptions::~BackgroundFetchOptions() = default;

BackgroundFetchRegistration::BackgroundFetchRegistration() = default;

BackgroundFetchRegistration::BackgroundFetchRegistration(
    const BackgroundFetchRegistration& other) = default;

BackgroundFetchRegistration::~BackgroundFetchRegistration() = default;

BackgroundFetchSettledFetch::BackgroundFetchSettledFetch() = default;

BackgroundFetchSettledFetch::BackgroundFetchSettledFetch(
    const BackgroundFetchSettledFetch& other) = default;

BackgroundFetchSettledFetch::~BackgroundFetchSettledFetch() = default;

}  // namespace content
