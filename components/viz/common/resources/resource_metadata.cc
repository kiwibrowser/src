// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/resources/resource_metadata.h"

namespace viz {

ResourceMetadata::ResourceMetadata() = default;

ResourceMetadata::ResourceMetadata(ResourceMetadata&& other) = default;

ResourceMetadata::~ResourceMetadata() = default;

ResourceMetadata& ResourceMetadata::operator=(ResourceMetadata&& other) =
    default;

}  // namespace viz
