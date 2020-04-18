// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_ACTIVATION_STATE_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_ACTIVATION_STATE_H_

#include <memory>

#include "components/subresource_filter/core/common/activation_level.h"

namespace base {
namespace trace_event {
class TracedValue;
}  // namespace trace_event
}  // namespace base

namespace subresource_filter {

// Encompasses all details of whether/how subresource filtering should be
// activated in a given frame in the frame hierarchy.
struct ActivationState {
  ActivationState() = default;

  explicit ActivationState(ActivationLevel activation_level)
      : activation_level(activation_level) {}

  bool operator==(const ActivationState& rhs) const {
    return activation_level == rhs.activation_level &&
           filtering_disabled_for_document ==
               rhs.filtering_disabled_for_document &&
           (filtering_disabled_for_document ||
            generic_blocking_rules_disabled ==
                rhs.generic_blocking_rules_disabled) &&
           measure_performance == rhs.measure_performance &&
           enable_logging == rhs.enable_logging;
  }

  bool operator!=(const ActivationState& rhs) const { return !operator==(rhs); }

  std::unique_ptr<base::trace_event::TracedValue> ToTracedValue() const;

  // The degree to which subresource filtering is activated for the page load.
  ActivationLevel activation_level = ActivationLevel::DISABLED;

  // Even when subresource filtering is activated at the page level, a document
  // in the current frame (and/or ancestors thereof) may still match special
  // filtering rules that specifically disable using certain types of rules for
  // filtering subresources of that document (and/or of documents in descendent
  // frames). See proto::ActivationType for details.
  //
  // |filtering_disabled_for_document| indicates whether the document in this
  // frame is subject to a whitelist rule with DOCUMENT activation type.
  //
  // |generic_blocking_rules_disabled| indicates whether the document in this
  // frame is subject to a whitelist rule with GENERICBLOCK activation type, and
  // is only defined if |filtering_disabled_for_document| is false.
  bool filtering_disabled_for_document = false;
  bool generic_blocking_rules_disabled = false;

  // Whether or not extended performance measurements are enabled for the
  // current page load (across all frames).
  bool measure_performance = false;

  // Whether or not to log messages in the devtools console.
  bool enable_logging = false;
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_ACTIVATION_STATE_H_
