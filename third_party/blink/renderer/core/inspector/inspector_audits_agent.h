// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_AUDITS_AGENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_AUDITS_AGENT_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/inspector/inspector_base_agent.h"
#include "third_party/blink/renderer/core/inspector/protocol/Audits.h"

namespace blink {

class CORE_EXPORT InspectorAuditsAgent final
    : public InspectorBaseAgent<protocol::Audits::Metainfo> {
 public:
  explicit InspectorAuditsAgent(InspectorNetworkAgent*);
  ~InspectorAuditsAgent() override;

  void Trace(blink::Visitor*) override;

  // Protocol methods.
  protocol::Response getEncodedResponse(const String& request_id,
                                        const String& encoding,
                                        protocol::Maybe<double> quality,
                                        protocol::Maybe<bool> size_only,
                                        protocol::Maybe<String>* out_body,
                                        int* out_original_size,
                                        int* out_encoded_size) override;

 private:
  Member<InspectorNetworkAgent> network_agent_;

  DISALLOW_COPY_AND_ASSIGN(InspectorAuditsAgent);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_AUDITS_AGENT_H_
