// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_UNACKED_INVALIDATION_SET_TEST_UTIL_H_
#define COMPONENTS_INVALIDATION_IMPL_UNACKED_INVALIDATION_SET_TEST_UTIL_H_

#include "components/invalidation/impl/unacked_invalidation_set.h"

#include "testing/gmock/include/gmock/gmock-matchers.h"

namespace syncer {

namespace test_util {

void PrintTo(const UnackedInvalidationSet& invalidations, ::std::ostream* os);

void PrintTo(const UnackedInvalidationsMap& map, ::std::ostream* os);

::testing::Matcher<const UnackedInvalidationSet&> Eq(
    const UnackedInvalidationSet& expected);

::testing::Matcher<const UnackedInvalidationsMap&> Eq(
    const UnackedInvalidationsMap& expected);

}  // namespace test_util

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_IMPL_UNACKED_INVALIDATION_SET_TEST_UTIL_H_
