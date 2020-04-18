// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/hit_test/hit_test_region_list.h"

namespace viz {

HitTestRegionList::HitTestRegionList() = default;
HitTestRegionList::~HitTestRegionList() = default;

HitTestRegionList::HitTestRegionList(HitTestRegionList&&) = default;
HitTestRegionList& HitTestRegionList::operator=(HitTestRegionList&&) = default;

}  // namespace viz
