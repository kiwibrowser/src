// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/compositor_mutator_client.h"

#include <memory>
#include "base/trace_event/trace_event.h"
#include "third_party/blink/renderer/platform/graphics/compositor_mutator_impl.h"

namespace blink {

CompositorMutatorClient::CompositorMutatorClient(
    std::unique_ptr<CompositorMutatorImpl> mutator)
    : mutator_(std::move(mutator)) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("cc"),
               "CompositorMutatorClient::CompositorMutatorClient");
  mutator_->SetClient(this);
}

CompositorMutatorClient::~CompositorMutatorClient() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("cc"),
               "CompositorMutatorClient::~CompositorMutatorClient");
}

void CompositorMutatorClient::Mutate(
    std::unique_ptr<cc::MutatorInputState> input_state) {
  TRACE_EVENT0("cc", "CompositorMutatorClient::Mutate");
  mutator_->Mutate(std::move(input_state));
}

void CompositorMutatorClient::SetMutationUpdate(
    std::unique_ptr<cc::MutatorOutputState> output_state) {
  TRACE_EVENT0("cc", "CompositorMutatorClient::SetMutationUpdate");
  client_->SetMutationUpdate(std::move(output_state));
}

void CompositorMutatorClient::SetClient(cc::LayerTreeMutatorClient* client) {
  TRACE_EVENT0("cc", "CompositorMutatorClient::SetClient");
  client_ = client;
}

bool CompositorMutatorClient::HasAnimators() {
  return mutator_->HasAnimators();
}

}  // namespace blink
