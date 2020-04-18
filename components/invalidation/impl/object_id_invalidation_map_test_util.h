// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_OBJECT_ID_INVALIDATION_MAP_TEST_UTIL_H_
#define COMPONENTS_INVALIDATION_IMPL_OBJECT_ID_INVALIDATION_MAP_TEST_UTIL_H_

// Convince googletest to use the correct overload for PrintTo().
#include "components/invalidation/impl/invalidation_test_util.h"
#include "components/invalidation/public/object_id_invalidation_map.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace syncer {

::testing::Matcher<const ObjectIdInvalidationMap&> Eq(
    const ObjectIdInvalidationMap& expected);

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_IMPL_OBJECT_ID_INVALIDATION_MAP_TEST_UTIL_H_
