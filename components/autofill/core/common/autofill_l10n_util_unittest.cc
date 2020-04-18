// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/autofill_l10n_util.h"

#include "base/test/histogram_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/icu/source/common/unicode/locid.h"

namespace autofill {
namespace l10n {

// Test the success in the creation of the ICU Collator.
TEST(CaseInsensitiveCompareTest, IcuCollatorCreation_Success) {
  base::HistogramTester histogram_tester;
  CaseInsensitiveCompare compare;
  histogram_tester.ExpectUniqueSample("Autofill.IcuCollatorCreationSuccess",
                                      true, 1);
}

// Test the failure in creating the ICU Collator.
TEST(CaseInsensitiveCompareTest, IcuCollatorCreation_FailureBadLocale) {
  // Setting the locale to a bogus value.
  icu::Locale bogusLocale = icu::Locale::createFromName("bogus");
  bogusLocale.setToBogus();

  base::HistogramTester histogram_tester;
  CaseInsensitiveCompare compare(bogusLocale);
  histogram_tester.ExpectUniqueSample("Autofill.IcuCollatorCreationSuccess",
                                      false, 1);
}

}  // namespace l10n
}  // namespace autofill
