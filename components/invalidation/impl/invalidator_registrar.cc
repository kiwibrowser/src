// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/invalidator_registrar.h"

#include <cstddef>
#include <iterator>
#include <utility>

#include "base/logging.h"
#include "components/invalidation/public/object_id_invalidation_map.h"

namespace syncer {

InvalidatorRegistrar::InvalidatorRegistrar()
    : state_(DEFAULT_INVALIDATION_ERROR) {}

InvalidatorRegistrar::~InvalidatorRegistrar() {
  DCHECK(thread_checker_.CalledOnValidThread());
  CHECK(handler_to_ids_map_.empty());
}

void InvalidatorRegistrar::RegisterHandler(InvalidationHandler* handler) {
  DCHECK(thread_checker_.CalledOnValidThread());
  CHECK(handler);
  CHECK(!handlers_.HasObserver(handler));
  handlers_.AddObserver(handler);
}

bool InvalidatorRegistrar::UpdateRegisteredIds(InvalidationHandler* handler,
                                               const ObjectIdSet& ids) {
  DCHECK(thread_checker_.CalledOnValidThread());
  CHECK(handler);
  CHECK(handlers_.HasObserver(handler));

  for (HandlerIdsMap::const_iterator it = handler_to_ids_map_.begin();
       it != handler_to_ids_map_.end(); ++it) {
    if (it->first == handler) {
      continue;
    }

    std::vector<invalidation::ObjectId> intersection;
    std::set_intersection(
        it->second.begin(), it->second.end(),
        ids.begin(), ids.end(),
        std::inserter(intersection, intersection.end()),
        ObjectIdLessThan());
    if (!intersection.empty()) {
      LOG(ERROR) << "Duplicate registration: trying to register "
                 << ObjectIdToString(*intersection.begin()) << " for "
                 << handler << " when it's already registered for "
                 << it->first;
      return false;
    }
  }

  if (ids.empty()) {
    handler_to_ids_map_.erase(handler);
  } else {
    handler_to_ids_map_[handler] = ids;
  }
  return true;
}

void InvalidatorRegistrar::UnregisterHandler(InvalidationHandler* handler) {
  DCHECK(thread_checker_.CalledOnValidThread());
  CHECK(handler);
  CHECK(handlers_.HasObserver(handler));
  handlers_.RemoveObserver(handler);
  handler_to_ids_map_.erase(handler);
}

ObjectIdSet InvalidatorRegistrar::GetRegisteredIds(
    InvalidationHandler* handler) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  HandlerIdsMap::const_iterator lookup = handler_to_ids_map_.find(handler);
  return lookup != handler_to_ids_map_.end() ? lookup->second : ObjectIdSet();
}

ObjectIdSet InvalidatorRegistrar::GetAllRegisteredIds() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  ObjectIdSet registered_ids;
  for (HandlerIdsMap::const_iterator it = handler_to_ids_map_.begin();
       it != handler_to_ids_map_.end(); ++it) {
    registered_ids.insert(it->second.begin(), it->second.end());
  }
  return registered_ids;
}

void InvalidatorRegistrar::DispatchInvalidationsToHandlers(
    const ObjectIdInvalidationMap& invalidation_map) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // If we have no handlers, there's nothing to do.
  if (!handlers_.might_have_observers()) {
    return;
  }

  for (HandlerIdsMap::iterator it = handler_to_ids_map_.begin();
       it != handler_to_ids_map_.end(); ++it) {
    ObjectIdInvalidationMap to_emit =
        invalidation_map.GetSubsetWithObjectIds(it->second);
    if (!to_emit.Empty()) {
      it->first->OnIncomingInvalidation(to_emit);
    }
  }
}

void InvalidatorRegistrar::UpdateInvalidatorState(InvalidatorState state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "New invalidator state: " << InvalidatorStateToString(state_)
      << " -> " << InvalidatorStateToString(state);
  state_ = state;
  for (auto& observer : handlers_)
    observer.OnInvalidatorStateChange(state);
}

InvalidatorState InvalidatorRegistrar::GetInvalidatorState() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return state_;
}

std::map<std::string, ObjectIdSet>
InvalidatorRegistrar::GetSanitizedHandlersIdsMap() {
  DCHECK(thread_checker_.CalledOnValidThread());
  std::map<std::string, ObjectIdSet> clean_handlers_to_ids;
  for (HandlerIdsMap::const_iterator it = handler_to_ids_map_.begin();
       it != handler_to_ids_map_.end();
       ++it) {
    clean_handlers_to_ids[it->first->GetOwnerName()] = ObjectIdSet(it->second);
  }
  return clean_handlers_to_ids;
}

bool InvalidatorRegistrar::IsHandlerRegisteredForTest(
    const InvalidationHandler* handler) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return handlers_.HasObserver(handler);
}

void InvalidatorRegistrar::DetachFromThreadForTest() {
  DCHECK(thread_checker_.CalledOnValidThread());
  thread_checker_.DetachFromThread();
}

}  // namespace syncer
