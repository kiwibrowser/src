// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/network/network_instrumentation.h"

#include "base/trace_event/trace_event.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/traced_value.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_load_priority.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"

namespace blink {
namespace network_instrumentation {

using network_instrumentation::RequestOutcome;
using blink::TracedValue;

const char kBlinkResourceID[] = "BlinkResourceID";
const char kResourceLoadTitle[] = "ResourceLoad";
const char kResourcePrioritySetTitle[] = "ResourcePrioritySet";
const char kNetInstrumentationCategory[] = TRACE_DISABLED_BY_DEFAULT("network");

const char* RequestOutcomeToString(RequestOutcome outcome) {
  switch (outcome) {
    case RequestOutcome::kSuccess:
      return "Success";
    case RequestOutcome::kFail:
      return "Fail";
    default:
      NOTREACHED();
      // We need to return something to avoid compiler warning.
      return "This should never happen";
  }
}

// Note: network_instrumentation code should do as much work as possible inside
// the arguments of trace macros so that very little instrumentation overhead is
// incurred if the trace category is disabled. See https://crbug.com/669666.

namespace {

std::unique_ptr<TracedValue> ScopedResourceTrackerBeginData(
    const blink::ResourceRequest& request) {
  std::unique_ptr<TracedValue> data = TracedValue::Create();
  data->SetString("url", request.Url().GetString());
  return data;
}

std::unique_ptr<TracedValue> ResourcePrioritySetData(
    blink::ResourceLoadPriority priority) {
  std::unique_ptr<TracedValue> data = TracedValue::Create();
  data->SetInteger("priority", static_cast<int>(priority));
  return data;
}

std::unique_ptr<TracedValue> EndResourceLoadData(RequestOutcome outcome) {
  std::unique_ptr<TracedValue> data = TracedValue::Create();
  data->SetString("outcome", RequestOutcomeToString(outcome));
  return data;
}

}  // namespace

ScopedResourceLoadTracker::ScopedResourceLoadTracker(
    unsigned long resource_id,
    const blink::ResourceRequest& request)
    : resource_load_continues_beyond_scope_(false), resource_id_(resource_id) {
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(
      kNetInstrumentationCategory, kResourceLoadTitle,
      TRACE_ID_WITH_SCOPE(kBlinkResourceID, TRACE_ID_LOCAL(resource_id)),
      "beginData", ScopedResourceTrackerBeginData(request));
}

ScopedResourceLoadTracker::~ScopedResourceLoadTracker() {
  if (!resource_load_continues_beyond_scope_)
    EndResourceLoad(resource_id_, RequestOutcome::kFail);
}

void ScopedResourceLoadTracker::ResourceLoadContinuesBeyondScope() {
  resource_load_continues_beyond_scope_ = true;
}

void ResourcePrioritySet(unsigned long resource_id,
                         blink::ResourceLoadPriority priority) {
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT1(
      kNetInstrumentationCategory, kResourcePrioritySetTitle,
      TRACE_ID_WITH_SCOPE(kBlinkResourceID, TRACE_ID_LOCAL(resource_id)),
      "data", ResourcePrioritySetData(priority));
}

void EndResourceLoad(unsigned long resource_id, RequestOutcome outcome) {
  TRACE_EVENT_NESTABLE_ASYNC_END1(
      kNetInstrumentationCategory, kResourceLoadTitle,
      TRACE_ID_WITH_SCOPE(kBlinkResourceID, TRACE_ID_LOCAL(resource_id)),
      "endData", EndResourceLoadData(outcome));
}

}  // namespace network_instrumentation
}  // namespace blink
