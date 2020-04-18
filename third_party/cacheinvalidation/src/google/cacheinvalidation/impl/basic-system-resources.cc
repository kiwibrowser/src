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

#include "google/cacheinvalidation/impl/basic-system-resources.h"

namespace invalidation {

BasicSystemResources::BasicSystemResources(
    Logger* logger, Scheduler* internal_scheduler,
    Scheduler* listener_scheduler, NetworkChannel* network,
    Storage* storage, const string& platform)
    : logger_(logger),
      internal_scheduler_(internal_scheduler),
      listener_scheduler_(listener_scheduler),
      network_(network),
      storage_(storage),
      platform_(platform) {
  logger_->SetSystemResources(this);
  internal_scheduler_->SetSystemResources(this);
  listener_scheduler_->SetSystemResources(this);
  network_->SetSystemResources(this);
  storage_->SetSystemResources(this);
}

BasicSystemResources::~BasicSystemResources() {
}

void BasicSystemResources::Start() {
  CHECK(!run_state_.IsStarted()) << "resources already started";

  // TODO(ghc): Investigate whether we should have Start() and Stop() methods
  // on components like the scheduler.  Otherwise, the resources can't start and
  // stop them ...
  run_state_.Start();
}

void BasicSystemResources::Stop() {
  CHECK(run_state_.IsStarted()) << "cannot stop resources that aren't started";
  CHECK(!run_state_.IsStopped()) << "resources already stopped";
  run_state_.Stop();
}

bool BasicSystemResources::IsStarted() const {
  return run_state_.IsStarted();
}

Logger* BasicSystemResources::logger() {
  return logger_.get();
}

Scheduler* BasicSystemResources::internal_scheduler() {
  return internal_scheduler_.get();
}

Scheduler* BasicSystemResources::listener_scheduler() {
  return listener_scheduler_.get();
}

NetworkChannel* BasicSystemResources::network() {
  return network_.get();
}

Storage* BasicSystemResources::storage() {
  return storage_.get();
}

string BasicSystemResources::platform() const {
  return platform_;
}

}  // namespace invalidation
