// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/model/omnibox_suggestions.h"

namespace vr {

Autocompletion::Autocompletion() = default;

Autocompletion::Autocompletion(const base::string16& new_input,
                               const base::string16& new_suffix)
    : input(new_input), suffix(new_suffix) {}

bool Autocompletion::operator==(const Autocompletion& other) const {
  return input == other.input && suffix == other.suffix;
}

OmniboxSuggestion::OmniboxSuggestion() {}

OmniboxSuggestion::OmniboxSuggestion(
    const base::string16& new_contents,
    const base::string16& new_description,
    const AutocompleteMatch::ACMatchClassifications&
        new_contents_classifications,
    const AutocompleteMatch::ACMatchClassifications&
        new_description_classifications,
    AutocompleteMatch::Type new_type,
    GURL new_destination,
    const base::string16& new_input,
    const base::string16& new_inline_autocompletion)
    : contents(new_contents),
      description(new_description),
      contents_classifications(new_contents_classifications),
      description_classifications(new_description_classifications),
      type(new_type),
      destination(new_destination),
      autocompletion(Autocompletion(new_input, new_inline_autocompletion)) {}

OmniboxSuggestion::~OmniboxSuggestion() = default;

OmniboxSuggestion::OmniboxSuggestion(const OmniboxSuggestion& other) {
  contents = other.contents;
  contents_classifications = other.contents_classifications;
  description = other.description;
  description_classifications = other.description_classifications;
  type = other.type;
  destination = other.destination;
  autocompletion = other.autocompletion;
}

OmniboxSuggestions::OmniboxSuggestions() {}

OmniboxSuggestions::~OmniboxSuggestions() {}

}  // namespace vr
