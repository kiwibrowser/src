// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/autofill_features.h"

namespace autofill {
namespace features {

// Controls whether the AddressNormalizer is supplied. If available, it may be
// used to normalize address and will incur fetching rules from the server.
const base::Feature kAutofillAddressNormalizer{
    "AutofillAddressNormalizer", base::FEATURE_ENABLED_BY_DEFAULT};

// Controls the use of GET (instead of POST) to fetch cacheable autofill query
// responses.
const base::Feature kAutofillCacheQueryResponses{
    "AutofillCacheQueryResponses", base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether Autofill attemps to fill dynamically changing forms.
const base::Feature kAutofillDynamicForms{"AutofillDynamicForms",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether the server credit cards are offered to be filled and
// uploaded to Google Pay if the sync service is in auth error.
const base::Feature kAutofillEnablePaymentsInteractionsOnAuthError{
    "AutofillDontOfferServerCardsOnAuthError",
    base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether or not a minimum number of fields is required before
// heuristic field type prediction is run for a form.
const base::Feature kAutofillEnforceMinRequiredFieldsForHeuristics{
    "AutofillEnforceMinRequiredFieldsForHeuristics",
    base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether or not a minimum number of fields is required before
// crowd-sourced field type predictions are queried for a form.
const base::Feature kAutofillEnforceMinRequiredFieldsForQuery{
    "AutofillEnforceMinRequiredFieldsForQuery",
    base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether or not a minimum number of fields is required before
// field type votes are uploaded to the crowd-sourcing server.
const base::Feature kAutofillEnforceMinRequiredFieldsForUpload{
    "AutofillEnforceMinRequiredFieldsForUpload",
    base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether credit card suggestions are made on insecure pages.
const base::Feature kAutofillRequireSecureCreditCardContext{
    "AutofillRequireSecureCreditCardContext", base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether Full Server credit cards should be reset when the sync
// service is in an auth error state.
const base::Feature kAutofillResetFullServerCardsOnAuthError{
    "AutofillResetFullServerCardsOnAuthError",
    base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether or not a group of fields not enclosed in a form can be
// considered a form. If this is enabled, unowned fields will only constitute
// a form if there are signals to suggest that this might a checkout page.
const base::Feature kAutofillRestrictUnownedFieldsToFormlessCheckout{
    "AutofillRestrictUnownedFieldsToFormlessCheckout",
    base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether experiment ids should be sent through
// Google Payments RPCs or not.
const base::Feature kAutofillSendExperimentIdsInPaymentsRPCs{
    "AutofillSendExperimentIdsInPaymentsRPCs",
    base::FEATURE_ENABLED_BY_DEFAULT};

// Controls attaching the autofill type predictions to their respective
// element in the DOM.
const base::Feature kAutofillShowTypePredictions{
    "AutofillShowTypePredictions", base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether inferred label is considered for comparing in
// FormFieldData.SimilarFieldAs.
const base::Feature kAutofillSkipComparingInferredLabels{
    "AutofillSkipComparingInferredLabels", base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether the credit card upload bubble shows the Google Pay logo and
// a shorter "Save card?" header message on mobile.
const base::Feature kAutofillUpstreamUseGooglePayBrandingOnMobile{
    "AutofillUpstreamUseGooglePayOnAndroidBranding",
    base::FEATURE_DISABLED_BY_DEFAULT};

// Controls whether or not the autofill UI triggers on a single click.
const base::Feature kSingleClickAutofill{"SingleClickAutofill",
                                         base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace features
}  // namespace autofill
