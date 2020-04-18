// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/experiment_labels.h"

#include <stddef.h>

#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "components/variations/variations_associated_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace variations {

TEST(ExperimentLabelsTest, ExtractNonVariationLabels) {
  struct {
    const char* input_label;
    const char* expected_output;
  } test_cases[] = {
      // Empty
      {"", ""},
      // One
      {"gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT",
       "gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT"},
      // Three
      {"CrVar1=123|Tue, 21 Jan 2014 15:30:21 GMT;"
       "experiment1=456|Tue, 21 Jan 2014 15:30:21 GMT;"
       "experiment2=789|Tue, 21 Jan 2014 15:30:21 GMT;"
       "CrVar1=123|Tue, 21 Jan 2014 15:30:21 GMT",
       "experiment1=456|Tue, 21 Jan 2014 15:30:21 GMT;"
       "experiment2=789|Tue, 21 Jan 2014 15:30:21 GMT"},
      // One and one Variation
      {"gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT;"
       "CrVar1=3310002|Tue, 21 Jan 2014 15:30:21 GMT",
       "gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT"},
      // One and one Variation, flipped
      {"CrVar1=3310002|Tue, 21 Jan 2014 15:30:21 GMT;"
       "gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT",
       "gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT"},
      // Sandwiched
      {"CrVar1=3310002|Tue, 21 Jan 2014 15:30:21 GMT;"
       "gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT;"
       "CrVar2=3310003|Tue, 21 Jan 2014 15:30:21 GMT;"
       "CrVar3=3310004|Tue, 21 Jan 2014 15:30:21 GMT",
       "gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT"},
      // Only Variations
      {"CrVar1=3310002|Tue, 21 Jan 2014 15:30:21 GMT;"
       "CrVar2=3310003|Tue, 21 Jan 2014 15:30:21 GMT;"
       "CrVar3=3310004|Tue, 21 Jan 2014 15:30:21 GMT",
       ""},
      // Empty values
      {"gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT;"
       "CrVar1=3310002|Tue, 21 Jan 2014 15:30:21 GMT",
       "gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT"},
      // Trailing semicolon
      {"gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT;"
       "CrVar1=3310002|Tue, 21 Jan 2014 15:30:21 GMT;",  // Note the semi here.
       "gcapi_brand=123|Tue, 21 Jan 2014 15:30:21 GMT"},
      // Semis
      {";;;;", ""},
      // Three non-Variation labels
      // Testing that the order is preserved.
      {"experiment1=456|Tue, 21 Jan 2014 15:30:21 GMT;"
       "experiment2=789|Tue, 21 Jan 2014 15:30:21 GMT;"
       "experiment3=123|Tue, 21 Jan 2014 15:30:21 GMT",
       "experiment1=456|Tue, 21 Jan 2014 15:30:21 GMT;"
       "experiment2=789|Tue, 21 Jan 2014 15:30:21 GMT;"
       "experiment3=123|Tue, 21 Jan 2014 15:30:21 GMT"},
  };

  for (size_t i = 0; i < arraysize(test_cases); ++i) {
    std::string non_variation_labels = base::UTF16ToUTF8(
        ExtractNonVariationLabels(
            base::ASCIIToUTF16(test_cases[i].input_label)));
    EXPECT_EQ(test_cases[i].expected_output, non_variation_labels);
  }
}

}  // namespace variations
