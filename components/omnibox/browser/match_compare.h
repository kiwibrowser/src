// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_MATCH_COMPARE_H_
#define COMPONENTS_OMNIBOX_BROWSER_MATCH_COMPARE_H_

#include "components/omnibox/browser/omnibox_field_trial.h"

// This class implements a special version of AutocompleteMatch::MoreRelevant
// that allows matches of particular types to be demoted in AutocompleteResult.
template <class Match>
class CompareWithDemoteByType {
 public:
  CompareWithDemoteByType(
      metrics::OmniboxEventProto::PageClassification page_classification) {
    OmniboxFieldTrial::GetDemotionsByType(page_classification, &demotions_);
  }

  // Returns the relevance score of |match| demoted appropriately by
  // |demotions_by_type_|.
  int GetDemotedRelevance(const Match& match) const {
    OmniboxFieldTrial::DemotionMultipliers::const_iterator demotion_it =
        demotions_.find(match.type);
    return (demotion_it == demotions_.end())
               ? match.relevance
               : (match.relevance * demotion_it->second);
  }

  // Comparison function.
  bool operator()(const Match& elem1, const Match& elem2) {
    // Compute demoted relevance scores for each match.
    const int demoted_relevance1 = GetDemotedRelevance(elem1);
    const int demoted_relevance2 = GetDemotedRelevance(elem2);
    // For equal-relevance matches, we sort alphabetically, so that providers
    // who return multiple elements at the same priority get a "stable" sort
    // across multiple updates.
    return (demoted_relevance1 == demoted_relevance2)
               ? (elem1.contents < elem2.contents)
               : (demoted_relevance1 > demoted_relevance2);
  }

 private:
  OmniboxFieldTrial::DemotionMultipliers demotions_;
};

template <class Match>
class DestinationSort {
 public:
  DestinationSort(
      metrics::OmniboxEventProto::PageClassification page_classification)
      : demote_by_type_(page_classification) {}
  bool operator()(const Match& elem1, const Match& elem2) {
    // Sort identical destination_urls together.
    // Place the most relevant matches first, so that when we call
    // std::unique(), these are the ones that get preserved.
    return (elem1.stripped_destination_url == elem2.stripped_destination_url)
               ? demote_by_type_(elem1, elem2)
               : (elem1.stripped_destination_url <
                  elem2.stripped_destination_url);
  }

 private:
  CompareWithDemoteByType<Match> demote_by_type_;
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_MATCH_COMPARE_H_
