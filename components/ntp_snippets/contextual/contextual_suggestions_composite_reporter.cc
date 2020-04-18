// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestions_composite_reporter.h"

#include "components/ntp_snippets/contextual/contextual_suggestions_ukm_entry.h"

namespace contextual_suggestions {

ContextualSuggestionsCompositeReporter::
    ContextualSuggestionsCompositeReporter() = default;

ContextualSuggestionsCompositeReporter::
    ~ContextualSuggestionsCompositeReporter() = default;

void ContextualSuggestionsCompositeReporter::SetupForPage(
    const std::string& url,
    ukm::SourceId source_id) {
  for (ContextualSuggestionsReporter* reporter : raw_reporters_)
    reporter->SetupForPage(url, source_id);
}

void ContextualSuggestionsCompositeReporter::RecordEvent(
    ContextualSuggestionsEvent event) {
  for (ContextualSuggestionsReporter* reporter : raw_reporters_)
    reporter->RecordEvent(event);
}

void ContextualSuggestionsCompositeReporter::Flush() {
  for (ContextualSuggestionsReporter* reporter : raw_reporters_)
    reporter->Flush();
}

void ContextualSuggestionsCompositeReporter::AddOwnedReporter(
    std::unique_ptr<ContextualSuggestionsReporter> reporter) {
  // It's possible the raw pointer has already been added. This case will
  // be taken care of by the set used to store the raw pointers.
  // This allows for the composite reporter to retroactively take ownership
  // after a raw pointer has already been added.
  ContextualSuggestionsReporter* raw_reporter = reporter.get();
  owned_reporters_.insert(std::move(reporter));
  AddRawReporter(raw_reporter);
}

void ContextualSuggestionsCompositeReporter::AddRawReporter(
    ContextualSuggestionsReporter* reporter) {
  CHECK(reporter);
  raw_reporters_.insert(reporter);
}

}  // namespace contextual_suggestions
