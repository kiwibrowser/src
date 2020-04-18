// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_COMMON_AUTOFILL_PREF_NAMES_H_
#define COMPONENTS_AUTOFILL_CORE_COMMON_AUTOFILL_PREF_NAMES_H_

namespace autofill {
namespace prefs {

// Alphabetical list of preference names specific to the Autofill
// component. Keep alphabetized, and document each in the .cc file.

extern const char kAutofillAcceptSaveCreditCardPromptState[];
extern const char kAutofillBillingCustomerNumber[];
extern const char kAutofillCreditCardEnabled[];
extern const char kAutofillCreditCardSigninPromoImpressionCount[];
extern const char kAutofillEnabled[];
extern const char kAutofillLastVersionDeduped[];
extern const char kAutofillLastVersionDisusedAddressesDeleted[];
extern const char kAutofillLastVersionDisusedCreditCardsDeleted[];
extern const char kAutofillOrphanRowsRemoved[];
extern const char kAutofillWalletImportEnabled[];
extern const char kAutofillWalletImportStorageCheckboxState[];

// Possible values for previous user decision when we displayed a save credit
// card prompt.
enum PreviousSaveCreditCardPromptUserDecision {
  PREVIOUS_SAVE_CREDIT_CARD_PROMPT_USER_DECISION_NONE,
  PREVIOUS_SAVE_CREDIT_CARD_PROMPT_USER_DECISION_ACCEPTED,
  PREVIOUS_SAVE_CREDIT_CARD_PROMPT_USER_DECISION_DENIED,
  NUM_PREVIOUS_SAVE_CREDIT_CARD_PROMPT_USER_DECISIONS
};

}  // namespace prefs
}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_COMMON_AUTOFILL_PREF_NAMES_H_
