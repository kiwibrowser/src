// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/safe_browsing/scorer.h"

#include <math.h>

#include <memory>
#include <unordered_map>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_piece.h"
#include "chrome/common/safe_browsing/client_model.pb.h"
#include "chrome/renderer/safe_browsing/features.h"

namespace {
// Enum used to keep stats about the status of the Scorer creation.
enum ScorerCreationStatus {
  SCORER_SUCCESS,
  SCORER_FAIL_MODEL_OPEN_FAIL,  // Not used anymore
  SCORER_FAIL_MODEL_FILE_EMPTY,  // Not used anymore
  SCORER_FAIL_MODEL_FILE_TOO_LARGE,  // Not used anymore
  SCORER_FAIL_MODEL_PARSE_ERROR,
  SCORER_FAIL_MODEL_MISSING_FIELDS,
  SCORER_STATUS_MAX  // Always add new values before this one.
};

void RecordScorerCreationStatus(ScorerCreationStatus status) {
  UMA_HISTOGRAM_ENUMERATION("SBClientPhishing.ScorerCreationStatus",
                            status,
                            SCORER_STATUS_MAX);
}
}  // namespace

namespace safe_browsing {

// Helper function which converts log odds to a probability in the range
// [0.0,1.0].
static double LogOdds2Prob(double log_odds) {
  // 709 = floor(1023*ln(2)).  2**1023 is the largest finite double.
  // Small log odds aren't a problem.  as the odds will be 0.  It's only
  // when we get +infinity for the odds, that odds/(odds+1) would be NaN.
  if (log_odds >= 709) {
    return 1.0;
  }
  double odds = exp(log_odds);
  return odds/(odds+1.0);
}

Scorer::Scorer() {}
Scorer::~Scorer() {}

/* static */
Scorer* Scorer::Create(const base::StringPiece& model_str) {
  std::unique_ptr<Scorer> scorer(new Scorer());
  ClientSideModel& model = scorer->model_;
  if (!model.ParseFromArray(model_str.data(), model_str.size())) {
    DLOG(ERROR) << "Unable to parse phishing model.  This Scorer object is "
                << "invalid.";
    RecordScorerCreationStatus(SCORER_FAIL_MODEL_PARSE_ERROR);
    return NULL;
  } else if (!model.IsInitialized()) {
    DLOG(ERROR) << "Unable to parse phishing model.  The model is missing "
                << "some required fields.  Maybe the .proto file changed?";
    RecordScorerCreationStatus(SCORER_FAIL_MODEL_MISSING_FIELDS);
    return NULL;
  }
  RecordScorerCreationStatus(SCORER_SUCCESS);
  for (int i = 0; i < model.page_term_size(); ++i) {
    scorer->page_terms_.insert(model.hashes(model.page_term(i)));
  }
  for (int i = 0; i < model.page_word_size(); ++i) {
    scorer->page_words_.insert(model.page_word(i));
  }
  return scorer.release();
}

double Scorer::ComputeScore(const FeatureMap& features) const {
  double logodds = 0.0;
  for (int i = 0; i < model_.rule_size(); ++i) {
    logodds += ComputeRuleScore(model_.rule(i), features);
  }
  return LogOdds2Prob(logodds);
}

int Scorer::model_version() const {
  return model_.version();
}

const base::hash_set<std::string>& Scorer::page_terms() const {
  return page_terms_;
}

const base::hash_set<uint32_t>& Scorer::page_words() const {
  return page_words_;
}

size_t Scorer::max_words_per_term() const {
  return model_.max_words_per_term();
}

uint32_t Scorer::murmurhash3_seed() const {
  return model_.murmur_hash_seed();
}

size_t Scorer::max_shingles_per_page() const {
  return model_.max_shingles_per_page();
}

size_t Scorer::shingle_size() const {
  return model_.shingle_size();
}

double Scorer::ComputeRuleScore(const ClientSideModel::Rule& rule,
                                const FeatureMap& features) const {
  const std::unordered_map<std::string, double>& feature_map =
      features.features();
  double rule_score = 1.0;
  for (int i = 0; i < rule.feature_size(); ++i) {
    const auto it = feature_map.find(model_.hashes(rule.feature(i)));
    if (it == feature_map.end() || it->second == 0.0) {
      // If the feature of the rule does not exist in the given feature map the
      // feature weight is considered to be zero.  If the feature weight is zero
      // we leave early since we know that the rule score will be zero.
      return 0.0;
    }
    rule_score *= it->second;
  }
  return rule_score * rule.weight();
}
}  // namespace safe_browsing
