// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_VARIATIONS_HTTP_HEADER_PROVIDER_H_
#define COMPONENTS_VARIATIONS_VARIATIONS_HTTP_HEADER_PROVIDER_H_

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/synchronization/lock.h"
#include "components/variations/synthetic_trials.h"
#include "components/variations/variations_associated_data.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace variations {

// A helper class for maintaining client experiments and metrics state
// transmitted in custom HTTP request headers.
// This class is a thread-safe singleton.
class VariationsHttpHeaderProvider : public base::FieldTrialList::Observer,
                                     public SyntheticTrialObserver {
 public:
  static VariationsHttpHeaderProvider* GetInstance();

  // Returns the value of the client data header, computing and caching it if
  // necessary. If |is_signed_in| is false, variation ids that should only be
  // sent for signed in users (i.e. GOOGLE_WEB_PROPERTIES_SIGNED_IN entries)
  // will not be included.
  std::string GetClientDataHeader(bool is_signed_in);

  // Returns a space-separated string containing the list of current active
  // variations (as would be reported in the |variation_id| repeated field of
  // the ClientVariations proto). Does not include variation ids that should be
  // sent for signed-in users only. The returned string is guaranteed to have a
  // a leading and trailing space, e.g. " 123 234 345 ".
  std::string GetVariationsString();

  // Result of ForceVariationIds() call.
  enum class ForceIdsResult {
    SUCCESS,
    INVALID_VECTOR_ENTRY,  // Invalid entry in |variation_ids|.
    INVALID_SWITCH_ENTRY,  // Invalid entry in |command_line_variation_ids|.
  };

  // Sets *additional* variation ids and trigger variation ids to be encoded in
  // the X-Client-Data request header. This is intended for development use to
  // force a server side experiment id. |variation_ids| should be a list of
  // strings of numeric experiment ids. Ids explicitly passed in |variation_ids|
  // and those in the comma-separated |command_line_variation_ids| are added.
  ForceIdsResult ForceVariationIds(
      const std::vector<std::string>& variation_ids,
      const std::string& command_line_variation_ids);

  // Resets any cached state for tests.
  void ResetForTesting();

 private:
  friend struct base::DefaultSingletonTraits<VariationsHttpHeaderProvider>;

  typedef std::pair<VariationID, IDCollectionKey> VariationIDEntry;

  FRIEND_TEST_ALL_PREFIXES(VariationsHttpHeaderProviderTest,
                           ForceVariationIds_Valid);
  FRIEND_TEST_ALL_PREFIXES(VariationsHttpHeaderProviderTest,
                           ForceVariationIds_ValidCommandLine);
  FRIEND_TEST_ALL_PREFIXES(VariationsHttpHeaderProviderTest,
                           ForceVariationIds_Invalid);
  FRIEND_TEST_ALL_PREFIXES(VariationsHttpHeaderProviderTest,
                           OnFieldTrialGroupFinalized);
  FRIEND_TEST_ALL_PREFIXES(VariationsHttpHeaderProviderTest,
                           GetVariationsString);

  VariationsHttpHeaderProvider();
  ~VariationsHttpHeaderProvider() override;

  // base::FieldTrialList::Observer:
  // This will add the variation ID associated with |trial_name| and
  // |group_name| to the variation ID cache.
  void OnFieldTrialGroupFinalized(const std::string& trial_name,
                                  const std::string& group_name) override;

  // metrics::SyntheticTrialObserver:
  void OnSyntheticTrialsChanged(
      const std::vector<SyntheticTrialGroup>& groups) override;

  // Prepares the variation IDs cache with initial values if not already done.
  // This method also registers the caller with the FieldTrialList to receive
  // new variation IDs.
  void InitVariationIDsCacheIfNeeded();

  // Looks up the associated id for the given trial/group and adds an entry for
  // it to |variation_ids_set_| if found.
  void CacheVariationsId(const std::string& trial_name,
                         const std::string& group_name,
                         IDCollectionKey key);

  // Takes whatever is currently in |variation_ids_set_| and recreates
  // |variation_ids_header_| with it.  Assumes the the |lock_| is currently
  // held.
  void UpdateVariationIDsHeaderValue();

  // Generates a base64-encoded proto to be used as a header value for the given
  // |is_signed_in| state.
  std::string GenerateBase64EncodedProto(bool is_signed_in);

  // Adds *additional* variation ids and trigger variation ids to be encoded in
  // the X-Client-Data request header.
  bool AddDefaultVariationIds(const std::vector<std::string>& variation_ids);

  // Returns the currently active set of variation ids, which includes any
  // default values, synthetic variations and actual field trial variations.
  std::set<VariationIDEntry> GetAllVariationIds();

  // Guards access to variables below.
  base::Lock lock_;

  // Whether or not we've initialized the caches.
  bool variation_ids_cache_initialized_;

  // Keep a cache of variation IDs that are transmitted in headers to Google.
  // This consists of a list of valid IDs, and the actual transmitted header.
  std::set<VariationIDEntry> variation_ids_set_;

  // Provides the google experiment ids forced from command line.
  std::set<VariationIDEntry> default_variation_ids_set_;

  // Variations ids from synthetic field trials.
  std::set<VariationIDEntry> synthetic_variation_ids_set_;

  std::string cached_variation_ids_header_;
  std::string cached_variation_ids_header_signed_in_;

  DISALLOW_COPY_AND_ASSIGN(VariationsHttpHeaderProvider);
};

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_VARIATIONS_HTTP_HEADER_PROVIDER_H_
