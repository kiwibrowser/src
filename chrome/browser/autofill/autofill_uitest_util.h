// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOFILL_AUTOFILL_UITEST_UTIL_H_
#define CHROME_BROWSER_AUTOFILL_AUTOFILL_UITEST_UTIL_H_

#include <vector>

class Browser;

namespace autofill {

class AutofillProfile;
class CreditCard;

void AddTestProfile(Browser* browser, const AutofillProfile& profile);
void SetTestProfile(Browser* browser, const AutofillProfile& profile);
void SetTestProfiles(Browser* browser, std::vector<AutofillProfile>* profiles);

void AddTestCreditCard(Browser* browser, const CreditCard& card);

}  // namespace autofill

#endif  // CHROME_BROWSER_AUTOFILL_AUTOFILL_UITEST_UTIL_H_
