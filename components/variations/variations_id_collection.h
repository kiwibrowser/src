// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_VARIATIONS_ID_COLLECTION_H_
#define COMPONENTS_VARIATIONS_VARIATIONS_ID_COLLECTION_H_

#include <set>
#include <string>

#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "components/variations/variations_associated_data.h"

namespace variations {

// Watches finalization of trials that may have a variation id for the given
// key. Maintains a list of ids for the given key, and invokes a callback every
// time a new id is found. Does not invoke the callback for ids for trials that
// were finalized prior to the construction of a VariationsIdCollection
// instance. Is not thread safe.
class VariationsIdCollection : public base::FieldTrialList::Observer {
 public:
  // Will not invoke |new_id_callback| for ids that correspond to trials that
  // were finalized before construction of this object. Callers may want to
  // call |GetIds()| manually after construction to batch the initial ids.
  VariationsIdCollection(
      IDCollectionKey collection_key,
      base::RepeatingCallback<void(VariationID)> new_id_callback);

  ~VariationsIdCollection() override;

  // base::FieldTrialList::Observer implementation.
  void OnFieldTrialGroupFinalized(const std::string& trial_name,
                                  const std::string& group_name) override;

  // Returns a set of all variations ids for trials finalized that are part of
  // |collection_key_|.
  const std::set<VariationID>& GetIds();

 private:
  const IDCollectionKey collection_key_;

  // Will not be set until the end of initialization.
  base::RepeatingCallback<void(VariationID)> new_id_callback_;

  std::set<VariationID> id_set_;

  DISALLOW_COPY_AND_ASSIGN(VariationsIdCollection);
};

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_VARIATIONS_ID_COLLECTION_H_
