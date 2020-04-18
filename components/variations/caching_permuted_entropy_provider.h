// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_CACHING_PERMUTED_ENTROPY_PROVIDER_H_
#define COMPONENTS_VARIATIONS_CACHING_PERMUTED_ENTROPY_PROVIDER_H_

#include <stddef.h>
#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/variations/entropy_provider.h"
#include "components/variations/proto/permuted_entropy_cache.pb.h"

class PrefService;
class PrefRegistrySimple;

namespace variations {

// CachingPermutedEntropyProvider is an entropy provider that uses the same
// algorithm as the PermutedEntropyProvider, but caches the results in Local
// State between runs.
class CachingPermutedEntropyProvider : public PermutedEntropyProvider {
 public:
  // Creates a CachingPermutedEntropyProvider using the given |local_state|
  // prefs service with the specified |low_entropy_source|, which should have a
  // value in the range of [0, low_entropy_source_max).
  CachingPermutedEntropyProvider(PrefService* local_state,
                                 uint16_t low_entropy_source,
                                 size_t low_entropy_source_max);
  ~CachingPermutedEntropyProvider() override;

  // Registers pref keys used by this class in the Local State pref registry.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Clears the cache in local state. Should be called when the low entropy
  // source value gets reset.
  static void ClearCache(PrefService* local_state);

 private:
  // PermutedEntropyProvider overrides:
  uint16_t GetPermutedValue(uint32_t randomization_seed) const override;

  // Reads the cache from local state.
  void ReadFromLocalState() const;

  // Updates local state with the state of the cache.
  void UpdateLocalState() const;

  // Adds |randomization_seed| -> |value| to the cache.
  void AddToCache(uint32_t randomization_seed, uint16_t value) const;

  // Finds the value corresponding to |randomization_seed|, setting |value| and
  // returning true if found.
  bool FindValue(uint32_t randomization_seed, uint16_t* value) const;

  base::ThreadChecker thread_checker_;
  PrefService* local_state_;
  mutable PermutedEntropyCache cache_;

  DISALLOW_COPY_AND_ASSIGN(CachingPermutedEntropyProvider);
};

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_CACHING_PERMUTED_ENTROPY_PROVIDER_H_
