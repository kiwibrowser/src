// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/caching_permuted_entropy_provider.h"

#include <string>

#include "base/base64.h"
#include "base/logging.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/variations/pref_names.h"

namespace variations {

CachingPermutedEntropyProvider::CachingPermutedEntropyProvider(
    PrefService* local_state,
    uint16_t low_entropy_source,
    size_t low_entropy_source_max)
    : PermutedEntropyProvider(low_entropy_source, low_entropy_source_max),
      local_state_(local_state) {
  ReadFromLocalState();
}

CachingPermutedEntropyProvider::~CachingPermutedEntropyProvider() {
}

// static
void CachingPermutedEntropyProvider::RegisterPrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterStringPref(
      variations::prefs::kVariationsPermutedEntropyCache, std::string());
}

// static
void CachingPermutedEntropyProvider::ClearCache(PrefService* local_state) {
  local_state->ClearPref(variations::prefs::kVariationsPermutedEntropyCache);
}

uint16_t CachingPermutedEntropyProvider::GetPermutedValue(
    uint32_t randomization_seed) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  uint16_t value = 0;
  if (!FindValue(randomization_seed, &value)) {
    value = PermutedEntropyProvider::GetPermutedValue(randomization_seed);
    AddToCache(randomization_seed, value);
  }
  return value;
}

void CachingPermutedEntropyProvider::ReadFromLocalState() const {
  const std::string base64_cache_data = local_state_->GetString(
      variations::prefs::kVariationsPermutedEntropyCache);
  std::string cache_data;
  if (!base::Base64Decode(base64_cache_data, &cache_data) ||
      !cache_.ParseFromString(cache_data)) {
    local_state_->ClearPref(variations::prefs::kVariationsPermutedEntropyCache);
    NOTREACHED();
  }
}

void CachingPermutedEntropyProvider::UpdateLocalState() const {
  std::string serialized;
  cache_.SerializeToString(&serialized);

  std::string base64_encoded;
  base::Base64Encode(serialized, &base64_encoded);
  local_state_->SetString(variations::prefs::kVariationsPermutedEntropyCache,
                          base64_encoded);
}

void CachingPermutedEntropyProvider::AddToCache(uint32_t randomization_seed,
                                                uint16_t value) const {
  PermutedEntropyCache::Entry* entry;
  const int kMaxSize = 25;
  if (cache_.entry_size() >= kMaxSize) {
    // If the cache is full, evict the first entry, swapping later entries in
    // to take its place. This effectively creates a FIFO cache, which is good
    // enough here because the expectation is that there shouldn't be more than
    // |kMaxSize| field trials at any given time, so eviction should happen very
    // rarely, only as new trials are introduced, evicting old expired trials.
    for (int i = 1; i < kMaxSize; ++i)
      cache_.mutable_entry()->SwapElements(i - 1, i);
    entry = cache_.mutable_entry(kMaxSize - 1);
  } else {
    entry = cache_.add_entry();
  }

  entry->set_randomization_seed(randomization_seed);
  entry->set_value(value);

  UpdateLocalState();
}

bool CachingPermutedEntropyProvider::FindValue(uint32_t randomization_seed,
                                               uint16_t* value) const {
  for (int i = 0; i < cache_.entry_size(); ++i) {
    if (cache_.entry(i).randomization_seed() == randomization_seed) {
      *value = cache_.entry(i).value();
      return true;
    }
  }
  return false;
}

}  // namespace variations
