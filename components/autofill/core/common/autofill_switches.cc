// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "components/autofill/core/common/autofill_switches.h"

namespace autofill {
namespace switches {

// Override the default autofill server URL with "scheme://host[:port]/prefix/".
const char kAutofillServerURL[] = "autofill-server-url";

// Force hiding the local save checkbox in the autofill dialog box for getting
// the full credit card number for a wallet card. The card will never be stored
// locally.
const char kDisableOfferStoreUnmaskedWalletCards[] =
    "disable-offer-store-unmasked-wallet-cards";

// Disables password generation when we detect that the user is going through
// account creation.
const char kDisablePasswordGeneration[]     = "disable-password-generation";

// Force showing the local save checkbox in the autofill dialog box for getting
// the full credit card number for a wallet card.
const char kEnableOfferStoreUnmaskedWalletCards[] =
    "enable-offer-store-unmasked-wallet-cards";

// Enables password generation when we detect that the user is going through
// account creation.
const char kEnablePasswordGeneration[]      = "enable-password-generation";

// Enables suggestions with substring matching instead of prefix matching.
const char kEnableSuggestionsWithSubstringMatch[] =
    "enable-suggestions-with-substring-match";

// Ignores autocomplete="off" for Autofill data (profiles + credit cards).
const char kIgnoreAutocompleteOffForAutofill[] =
    "ignore-autocomplete-off-autofill";

// Removes the requirement that we recieved a ping from the autofill servers
// and that the user doesn't have the given form blacklisted. Used in testing.
const char kLocalHeuristicsOnlyForPasswordGeneration[] =
    "local-heuristics-only-for-password-generation";

// Annotates forms with Autofill field type predictions.
const char kShowAutofillTypePredictions[]   = "show-autofill-type-predictions";

// Annotates forms and fields with Autofill signatures.
const char kShowAutofillSignatures[] = "show-autofill-signatures";

// Use the sandbox Online Wallet service URL (for developer testing).
const char kWalletServiceUseSandbox[]       = "wallet-service-use-sandbox";

}  // namespace switches
}  // namespace autofill
