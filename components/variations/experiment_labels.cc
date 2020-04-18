// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/experiment_labels.h"

#include <vector>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/variations/variations_associated_data.h"
#include "components/variations/variations_experiment_util.h"

namespace variations {
namespace {

const char kVariationPrefix[] = "CrVar";

}  // namespace

base::string16 ExtractNonVariationLabels(const base::string16& labels) {
  // First, split everything by the label separator.
  std::vector<base::StringPiece16> entries = base::SplitStringPiece(
      labels, base::StringPiece16(&kExperimentLabelSeparator, 1),
      base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  // For each label, keep the ones that do not look like a Variations label.
  base::string16 non_variation_labels;
  for (const base::StringPiece16& entry : entries) {
    if (entry.empty() ||
        base::StartsWith(entry,
                         base::ASCIIToUTF16(kVariationPrefix),
                         base::CompareCase::INSENSITIVE_ASCII)) {
      continue;
    }

    // Dump the whole thing, including the timestamp.
    if (!non_variation_labels.empty())
      non_variation_labels += kExperimentLabelSeparator;
    entry.AppendToString(&non_variation_labels);
  }

  return non_variation_labels;
}

}  // namespace variations
