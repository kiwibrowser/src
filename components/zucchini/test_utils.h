// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ZUCCHINI_TEST_UTILS_H_
#define COMPONENTS_ZUCCHINI_TEST_UTILS_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace zucchini {

// Parses space-separated list of byte hex values into list.
std::vector<uint8_t> ParseHexString(const std::string& hex_string);

}  // namespace zucchini

#endif  // COMPONENTS_ZUCCHINI_TEST_UTILS_H_
