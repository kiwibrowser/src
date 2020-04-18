// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_SAFE_BROWSING_TEST_UTILS_H_
#define CHROME_RENDERER_SAFE_BROWSING_TEST_UTILS_H_

namespace safe_browsing {
class FeatureMap;

// Compares two FeatureMap objects using gMock.  Always use this instead of
// operator== or ContainerEq, since hash_map's equality operator may return
// false if the elements were inserted in different orders.
void ExpectFeatureMapsAreEqual(const FeatureMap& first,
                               const FeatureMap& second);

}  // namespace safe_browsing

#endif  // CHROME_RENDERER_SAFE_BROWSING_TEST_UTILS_H_
