// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_TEST_RULESET_UTILS_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_TEST_RULESET_UTILS_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/strings/string_piece.h"
#include "components/url_pattern_index/proto/rules.pb.h"

namespace subresource_filter {
namespace testing {

// Creates a blacklist URL rule which targets subresources of any type such that
// the resource URL ends with |suffix|.
url_pattern_index::proto::UrlRule CreateSuffixRule(base::StringPiece suffix);

// Creates a white URL rule which targets subresources of any type such that
// the resource URL ends with |suffix|.
url_pattern_index::proto::UrlRule CreateWhitelistSuffixRule(
    base::StringPiece suffix);

// Same as CreateUrlRule(pattern, proto::URL_PATTERN_TYPE_WILDCARDED), but the
// rule applies to the specified |activation_types|, and to no element types.
// Additionally, it is restricted to a set of |domains| (if provided).
url_pattern_index::proto::UrlRule CreateWhitelistRuleForDocument(
    base::StringPiece pattern,
    int32_t activation_types =
        url_pattern_index::proto::ACTIVATION_TYPE_DOCUMENT,
    std::vector<std::string> domains = std::vector<std::string>());

}  // namespace testing
}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_TEST_RULESET_UTILS_H_
