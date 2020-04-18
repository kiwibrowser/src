// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_MUTATOR_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_MUTATOR_CLIENT_H_

#include <memory>
#include "cc/trees/layer_tree_mutator.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class CompositorMutatorImpl;

class PLATFORM_EXPORT CompositorMutatorClient : public cc::LayerTreeMutator {
 public:
  explicit CompositorMutatorClient(std::unique_ptr<CompositorMutatorImpl>);
  ~CompositorMutatorClient() override;

  void SetMutationUpdate(std::unique_ptr<cc::MutatorOutputState>);

  // cc::LayerTreeMutator
  void SetClient(cc::LayerTreeMutatorClient*) override;
  void Mutate(std::unique_ptr<cc::MutatorInputState>) override;
  bool HasAnimators() override;

 private:
  std::unique_ptr<CompositorMutatorImpl> mutator_;
  cc::LayerTreeMutatorClient* client_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_MUTATOR_CLIENT_H_
