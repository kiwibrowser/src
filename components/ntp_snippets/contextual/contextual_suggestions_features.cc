// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestions_features.h"

namespace contextual_suggestions {

const base::Feature kContextualSuggestionsBottomSheet{
    "ContextualSuggestionsBottomSheet", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kContextualSuggestionsEnterprisePolicyBypass{
    "ContextualSuggestionsEnterprisePolicyBypass",
    base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kContextualSuggestionsSlimPeekUI{
    "ContextualSuggestionsSlimPeekUI", base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace contextual_suggestions
