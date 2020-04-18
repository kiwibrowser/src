// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_INTERVENTION_REPORT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_INTERVENTION_REPORT_H_

#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/core/frame/message_report.h"

namespace blink {

class CORE_EXPORT InterventionReport : public MessageReport {
  DEFINE_WRAPPERTYPEINFO();

 public:
  InterventionReport(const String& message,
                     std::unique_ptr<SourceLocation> location)
      : MessageReport(message, std::move(location)) {}

  ~InterventionReport() override = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_INTERVENTION_REPORT_H_
