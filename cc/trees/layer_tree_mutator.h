// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_LAYER_TREE_MUTATOR_H_
#define CC_TREES_LAYER_TREE_MUTATOR_H_

#include "base/callback_forward.h"
#include "base/time/time.h"
#include "cc/cc_export.h"

#include <memory>
#include <string>
#include <vector>

namespace cc {

// TODO(majidvp): Currently the sync mechanism between cc and worklet is
// stateless meaning that it sends a new copy of the world every sync cycle.
// This has the benefit of keeping things very simple but we should revisit this
// and only send data relevant to particular phase of animator lifecycle e.g.,
// name and options dictionary are used just for construction.
struct CC_EXPORT MutatorInputState {
  struct CC_EXPORT AnimationState {
    int animation_id = 0;
    // Name associated with worklet animation.
    std::string name;
    // Worklet animation's current time, from its associated timeline.
    double current_time = 0;
  };

  MutatorInputState();
  ~MutatorInputState();

  std::vector<AnimationState> animations;
};

struct CC_EXPORT MutatorOutputState {
  struct CC_EXPORT AnimationState {
    int animation_id = 0;
    // The animator effect's local time.
    // TODO(majidvp): This assumes each animator has a single output effect
    // which does not hold once we state support group effects.
    // http://crbug.com/767043
    base::TimeDelta local_time;
  };

  MutatorOutputState();
  ~MutatorOutputState();

  std::vector<AnimationState> animations;
};

class LayerTreeMutatorClient {
 public:
  // Called when mutator needs to update its output.
  //
  // |output_state|: Most recent output of the mutator.
  virtual void SetMutationUpdate(
      std::unique_ptr<MutatorOutputState> output_state) = 0;
};

class CC_EXPORT LayerTreeMutator {
 public:
  virtual ~LayerTreeMutator() {}

  virtual void SetClient(LayerTreeMutatorClient* client) = 0;

  virtual void Mutate(std::unique_ptr<MutatorInputState> input_state) = 0;
  // TODO(majidvp): Remove when timeline inputs are known.
  virtual bool HasAnimators() = 0;
};

}  // namespace cc

#endif  // CC_TREES_LAYER_TREE_MUTATOR_H_
