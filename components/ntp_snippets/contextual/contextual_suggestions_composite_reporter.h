// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_COMPOSITE_REPORTER_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_COMPOSITE_REPORTER_H_

#include <memory>
#include <set>

#include "base/macros.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_reporter.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace contextual_suggestions {

// Reporter specialized for reporting to many different reporters.
// This is an add-only collection of reporters because right now reporters live
// as long as the service, or a proxy owning the composite. Therefore everything
// is torn down together.
class ContextualSuggestionsCompositeReporter
    : public ContextualSuggestionsReporter {
 public:
  ContextualSuggestionsCompositeReporter();
  ~ContextualSuggestionsCompositeReporter() override;

  // ContextualSuggestionsReporter
  void SetupForPage(const std::string& url, ukm::SourceId source_id) override;
  void RecordEvent(ContextualSuggestionsEvent event) override;
  void Flush() override;

  // Composite pattern.
  // Add a reporter which will be cleaned up with this. Use this if you want the
  // reporter to be destroyed with the composite reporter.
  void AddOwnedReporter(
      std::unique_ptr<ContextualSuggestionsReporter> reporter);
  // Add an unmanaged reporter. Use this if you want to manage the lifetime of
  // this reporter yourself.
  void AddRawReporter(ContextualSuggestionsReporter* reporter);

 private:
  // List of owned reporters.
  std::set<std::unique_ptr<ContextualSuggestionsReporter>> owned_reporters_;

  // List of all reporters that get the events.
  std::set<ContextualSuggestionsReporter*> raw_reporters_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsCompositeReporter);
};
}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_COMPOSITE_REPORTER_H_
