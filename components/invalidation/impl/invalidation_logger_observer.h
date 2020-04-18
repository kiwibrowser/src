// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_INVALIDATION_LOGGER_OBSERVER_H_
#define COMPONENTS_INVALIDATION_IMPL_INVALIDATION_LOGGER_OBSERVER_H_

#include "components/invalidation/public/invalidation_util.h"
#include "components/invalidation/public/invalidator_state.h"
#include "components/invalidation/public/object_id_invalidation_map.h"

namespace base {
class DictionaryValue;
}  // namespace base

namespace syncer {
class ObjectIdInvalidationMap;
}  // namespace syncer

namespace invalidation {
// This class provides the possibilty to register as an observer for the
// InvalidationLogger notifications whenever an InvalidatorService changes
// its internal state.
// (i.e. A new registration, a new invalidation, a TICL/GCM state change)
class InvalidationLoggerObserver {
 public:
  virtual void OnRegistrationChange(
      const std::multiset<std::string>& registered_handlers) = 0;
  virtual void OnStateChange(const syncer::InvalidatorState& new_state,
                             const base::Time& last_change_timestamp) = 0;
  virtual void OnUpdateIds(const std::string& handler_name,
                           const syncer::ObjectIdCountMap& details) = 0;
  virtual void OnDebugMessage(const base::DictionaryValue& details) = 0;
  virtual void OnInvalidation(
      const syncer::ObjectIdInvalidationMap& details) = 0;
  virtual void OnDetailedStatus(const base::DictionaryValue& details) = 0;

 protected:
  virtual ~InvalidationLoggerObserver() {}
};

}  // namespace invalidation

#endif  // COMPONENTS_INVALIDATION_IMPL_INVALIDATION_LOGGER_OBSERVER_H_
