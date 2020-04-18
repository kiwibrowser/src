// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Implementation of the Invalidation Client Library (Ticl).

#include "google/cacheinvalidation/impl/invalidation-client-impl.h"

namespace invalidation {

InvalidationClientImpl::InvalidationClientImpl(
    SystemResources* resources, Random* random, int client_type,
    const string& client_name, const ClientConfigP& config,
    const string& application_name, InvalidationListener* listener)
    : InvalidationClientCore(resources, random, client_type, client_name,
        config, application_name),
      listener_(new CheckingInvalidationListener(
            listener, GetStatistics(), resources->internal_scheduler(),
            resources->listener_scheduler(), resources->logger())) {
}

void InvalidationClientImpl::Start() {
    GetInternalScheduler()->Schedule(
        Scheduler::NoDelay(),
        NewPermanentCallback(this, &InvalidationClientImpl::DoStart));
}

void InvalidationClientImpl::Stop() {
    GetInternalScheduler()->Schedule(
        Scheduler::NoDelay(),
        NewPermanentCallback(this, &InvalidationClientImpl::DoStop));
}

void InvalidationClientImpl::Register(const ObjectId& object_id) {
    GetInternalScheduler()->Schedule(
        Scheduler::NoDelay(),
        NewPermanentCallback(this, &InvalidationClientImpl::DoRegister,
                             object_id));
}

void InvalidationClientImpl::Register(const vector<ObjectId>& object_ids) {
    GetInternalScheduler()->Schedule(
        Scheduler::NoDelay(),
        NewPermanentCallback(this, &InvalidationClientImpl::DoBulkRegister,
                             object_ids));
}

void InvalidationClientImpl::Unregister(const ObjectId& object_id) {
    GetInternalScheduler()->Schedule(
        Scheduler::NoDelay(),
        NewPermanentCallback(this, &InvalidationClientImpl::DoUnregister,
                             object_id));
}

void InvalidationClientImpl::Unregister(const vector<ObjectId>& object_ids) {
    GetInternalScheduler()->Schedule(
        Scheduler::NoDelay(),
        NewPermanentCallback(this, &InvalidationClientImpl::DoBulkUnregister,
                             object_ids));
}

void InvalidationClientImpl::Acknowledge(const AckHandle& acknowledge_handle) {
    GetInternalScheduler()->Schedule(
        Scheduler::NoDelay(),
        NewPermanentCallback(this, &InvalidationClientImpl::DoAcknowledge,
                             acknowledge_handle));
}

}  // namespace invalidation
