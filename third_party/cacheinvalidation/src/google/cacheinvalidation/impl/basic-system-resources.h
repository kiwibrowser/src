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

// A simple implementation of SystemResources that just takes the resource
// components and constructs a SystemResources object.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_BASIC_SYSTEM_RESOURCES_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_BASIC_SYSTEM_RESOURCES_H_

#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/deps/scoped_ptr.h"
#include "google/cacheinvalidation/impl/run-state.h"

namespace invalidation {

class BasicSystemResources : public SystemResources {
 public:
  // Constructs an instance from resource components.  Ownership of all
  // components is transferred to the BasicSystemResources object.
  BasicSystemResources(
      Logger* logger, Scheduler* internal_scheduler,
      Scheduler* listener_scheduler, NetworkChannel* network,
      Storage* storage, const string& platform);

  virtual ~BasicSystemResources();

  // Overrides from SystemResources.
  virtual void Start();
  virtual void Stop();
  virtual bool IsStarted() const;

  virtual Logger* logger();
  virtual Scheduler* internal_scheduler();
  virtual Scheduler* listener_scheduler();
  virtual NetworkChannel* network();
  virtual Storage* storage();
  virtual string platform() const;

 private:
  // Components comprising the system resources. We delegate calls to these as
  // appropriate.
  scoped_ptr<Logger> logger_;
  scoped_ptr<Scheduler> internal_scheduler_;
  scoped_ptr<Scheduler> listener_scheduler_;
  scoped_ptr<NetworkChannel> network_;
  scoped_ptr<Storage> storage_;

  // The state of the resources.
  RunState run_state_;

  // Information about the client operating system/platform.
  string platform_;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_BASIC_SYSTEM_RESOURCES_H_
