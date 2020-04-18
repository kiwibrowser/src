// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/renderer/subresource_filter_agent.h"

#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "components/subresource_filter/content/common/subresource_filter_messages.h"
#include "components/subresource_filter/content/common/subresource_filter_utils.h"
#include "components/subresource_filter/content/renderer/unverified_ruleset_dealer.h"
#include "components/subresource_filter/content/renderer/web_document_subresource_filter_impl.h"
#include "components/subresource_filter/core/common/document_load_statistics.h"
#include "components/subresource_filter/core/common/document_subresource_filter.h"
#include "components/subresource_filter/core/common/memory_mapped_ruleset.h"
#include "components/subresource_filter/core/common/scoped_timers.h"
#include "components/subresource_filter/core/common/time_measurements.h"
#include "content/public/common/content_features.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/render_frame.h"
#include "ipc/ipc_message.h"
#include "third_party/blink/public/platform/web_worker_fetch_context.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_document_loader.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/url_constants.h"

namespace subresource_filter {

SubresourceFilterAgent::SubresourceFilterAgent(
    content::RenderFrame* render_frame,
    UnverifiedRulesetDealer* ruleset_dealer)
    : content::RenderFrameObserver(render_frame),
      content::RenderFrameObserverTracker<SubresourceFilterAgent>(render_frame),
      ruleset_dealer_(ruleset_dealer) {
  DCHECK(ruleset_dealer);
}

SubresourceFilterAgent::~SubresourceFilterAgent() = default;

GURL SubresourceFilterAgent::GetDocumentURL() {
  return render_frame()->GetWebFrame()->GetDocument().Url();
}

bool SubresourceFilterAgent::IsMainFrame() {
  return render_frame()->IsMainFrame();
}

void SubresourceFilterAgent::SetSubresourceFilterForCommittedLoad(
    std::unique_ptr<blink::WebDocumentSubresourceFilter> filter) {
  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  web_frame->GetDocumentLoader()->SetSubresourceFilter(filter.release());
}

void SubresourceFilterAgent::
    SignalFirstSubresourceDisallowedForCommittedLoad() {
  render_frame()->Send(new SubresourceFilterHostMsg_DidDisallowFirstSubresource(
      render_frame()->GetRoutingID()));
}

void SubresourceFilterAgent::SendDocumentLoadStatistics(
    const DocumentLoadStatistics& statistics) {
  render_frame()->Send(new SubresourceFilterHostMsg_DocumentLoadStatistics(
      render_frame()->GetRoutingID(), statistics));
}

void SubresourceFilterAgent::SendFrameIsAdSubframe() {
  render_frame()->Send(new SubresourceFilterHostMsg_FrameIsAdSubframe(
      render_frame()->GetRoutingID()));
}

bool SubresourceFilterAgent::IsAdSubframe() {
  return render_frame()->GetWebFrame()->IsAdSubframe();
}

void SubresourceFilterAgent::SetIsAdSubframe() {
  render_frame()->GetWebFrame()->SetIsAdSubframe();
}

// static
ActivationState SubresourceFilterAgent::GetParentActivationState(
    content::RenderFrame* render_frame) {
  blink::WebFrame* parent =
      render_frame ? render_frame->GetWebFrame()->Parent() : nullptr;
  if (parent && parent->IsWebLocalFrame()) {
    auto* agent = SubresourceFilterAgent::Get(
        content::RenderFrame::FromWebFrame(parent->ToWebLocalFrame()));
    if (agent && agent->filter_for_last_committed_load_)
      return agent->filter_for_last_committed_load_->activation_state();
  }
  return ActivationState(ActivationLevel::DISABLED);
}

void SubresourceFilterAgent::OnActivateForNextCommittedLoad(
    const ActivationState& activation_state,
    bool is_ad_subframe) {
  activation_state_for_next_commit_ = activation_state;
  if (is_ad_subframe)
    SetIsAdSubframe();
}

void SubresourceFilterAgent::RecordHistogramsOnLoadCommitted(
    const ActivationState& activation_state) {
  // Note: ActivationLevel used to be called ActivationState, the legacy name is
  // kept for the histogram.
  ActivationLevel activation_level = activation_state.activation_level;
  UMA_HISTOGRAM_ENUMERATION("SubresourceFilter.DocumentLoad.ActivationState",
                            static_cast<int>(activation_level),
                            static_cast<int>(ActivationLevel::LAST) + 1);

  if (activation_level != ActivationLevel::DISABLED) {
    UMA_HISTOGRAM_BOOLEAN("SubresourceFilter.DocumentLoad.RulesetIsAvailable",
                          ruleset_dealer_->IsRulesetFileAvailable());
  }
}

void SubresourceFilterAgent::RecordHistogramsOnLoadFinished() {
  DCHECK(filter_for_last_committed_load_);
  const auto& statistics =
      filter_for_last_committed_load_->filter().statistics();

  UMA_HISTOGRAM_COUNTS_1000(
      "SubresourceFilter.DocumentLoad.NumSubresourceLoads.Total",
      statistics.num_loads_total);
  UMA_HISTOGRAM_COUNTS_1000(
      "SubresourceFilter.DocumentLoad.NumSubresourceLoads.Evaluated",
      statistics.num_loads_evaluated);
  UMA_HISTOGRAM_COUNTS_1000(
      "SubresourceFilter.DocumentLoad.NumSubresourceLoads.MatchedRules",
      statistics.num_loads_matching_rules);
  UMA_HISTOGRAM_COUNTS_1000(
      "SubresourceFilter.DocumentLoad.NumSubresourceLoads.Disallowed",
      statistics.num_loads_disallowed);

  // If ThreadTicks is not supported or performance measuring is switched off,
  // then no time measurements have been collected.
  if (ScopedThreadTimers::IsSupported() &&
      filter_for_last_committed_load_->filter()
          .activation_state()
          .measure_performance) {
    UMA_HISTOGRAM_CUSTOM_MICRO_TIMES(
        "SubresourceFilter.DocumentLoad.SubresourceEvaluation."
        "TotalWallDuration",
        statistics.evaluation_total_wall_duration,
        base::TimeDelta::FromMicroseconds(1), base::TimeDelta::FromSeconds(10),
        50);
    UMA_HISTOGRAM_CUSTOM_MICRO_TIMES(
        "SubresourceFilter.DocumentLoad.SubresourceEvaluation.TotalCPUDuration",
        statistics.evaluation_total_cpu_duration,
        base::TimeDelta::FromMicroseconds(1), base::TimeDelta::FromSeconds(10),
        50);
  } else {
    DCHECK(statistics.evaluation_total_wall_duration.is_zero());
    DCHECK(statistics.evaluation_total_cpu_duration.is_zero());
  }

  SendDocumentLoadStatistics(statistics);
}

void SubresourceFilterAgent::ResetInfoForNextCommit() {
  activation_state_for_next_commit_ =
      ActivationState(ActivationLevel::DISABLED);
}

void SubresourceFilterAgent::OnDestruct() {
  delete this;
}

void SubresourceFilterAgent::DidCreateNewDocument() {
  if (!first_document_)
    return;
  first_document_ = false;

  // Local subframes will first navigate to kAboutBlankURL. Frames created by
  // the browser initialize the LocalFrame before creating
  // RenderFrameObservers, so the about:blank document isn't observed. We only
  // care about local subframes.
  if (IsAdSubframe() && GetDocumentURL() == url::kAboutBlankURL)
    SendFrameIsAdSubframe();
}

void SubresourceFilterAgent::DidCommitProvisionalLoad(
    bool is_new_navigation,
    bool is_same_document_navigation) {
  if (is_same_document_navigation)
    return;

  filter_for_last_committed_load_.reset();

  // TODO(csharrison): Use WebURL and WebSecurityOrigin for efficiency here,
  // which require changes to the unit tests.
  const GURL& url = GetDocumentURL();

  bool use_parent_activation = !IsMainFrame() && ShouldUseParentActivation(url);

  const ActivationState activation_state =
      use_parent_activation ? GetParentActivationState(render_frame())
                            : activation_state_for_next_commit_;

  ResetInfoForNextCommit();

  // Do not pollute the histograms for empty main frame documents.
  if (IsMainFrame() && !url.SchemeIsHTTPOrHTTPS() && !url.SchemeIsFile())
    return;

  RecordHistogramsOnLoadCommitted(activation_state);
  if (activation_state.activation_level == ActivationLevel::DISABLED ||
      !ruleset_dealer_->IsRulesetFileAvailable())
    return;

  scoped_refptr<const MemoryMappedRuleset> ruleset =
      ruleset_dealer_->GetRuleset();
  DCHECK(ruleset);
  // Data can be null even if the original file is valid, if there is a
  // memory mapping issue.
  if (!ruleset->data())
    return;

  base::OnceClosure first_disallowed_load_callback(base::BindOnce(
      &SubresourceFilterAgent::SignalFirstSubresourceDisallowedForCommittedLoad,
      AsWeakPtr()));
  auto filter = std::make_unique<WebDocumentSubresourceFilterImpl>(
      url::Origin::Create(url), activation_state, std::move(ruleset),
      std::move(first_disallowed_load_callback), IsAdSubframe());

  filter_for_last_committed_load_ = filter->AsWeakPtr();
  SetSubresourceFilterForCommittedLoad(std::move(filter));
}

void SubresourceFilterAgent::DidFailProvisionalLoad(
    const blink::WebURLError& error) {
  // TODO(engedy): Add a test with `frame-ancestor` violation to exercise this.
  ResetInfoForNextCommit();
}

void SubresourceFilterAgent::DidFinishLoad() {
  if (!filter_for_last_committed_load_)
    return;
  RecordHistogramsOnLoadFinished();
}

bool SubresourceFilterAgent::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(SubresourceFilterAgent, message)
    IPC_MESSAGE_HANDLER(SubresourceFilterMsg_ActivateForNextCommittedLoad,
                        OnActivateForNextCommittedLoad)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void SubresourceFilterAgent::WillCreateWorkerFetchContext(
    blink::WebWorkerFetchContext* worker_fetch_context) {
  if (!filter_for_last_committed_load_)
    return;
  if (!ruleset_dealer_->IsRulesetFileAvailable())
    return;
  base::File ruleset_file = ruleset_dealer_->DuplicateRulesetFile();
  if (!ruleset_file.IsValid())
    return;

  worker_fetch_context->SetSubresourceFilterBuilder(
      std::make_unique<WebDocumentSubresourceFilterImpl::BuilderImpl>(
          url::Origin::Create(GetDocumentURL()),
          filter_for_last_committed_load_->filter().activation_state(),
          std::move(ruleset_file),
          base::BindOnce(&SubresourceFilterAgent::
                             SignalFirstSubresourceDisallowedForCommittedLoad,
                         AsWeakPtr()),
          IsAdSubframe()));
}

}  // namespace subresource_filter
