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

#include "google/cacheinvalidation/include/invalidation-client-factory.h"

#include <stdint.h>

#include "google/cacheinvalidation/impl/invalidation-client-impl.h"

namespace invalidation {

InvalidationClient* ClientFactory::Create(
    SystemResources* resources,
    const InvalidationClientConfig& config,
    InvalidationListener* listener) {
  ClientConfigP client_config;
  InvalidationClientCore::InitConfig(&client_config);
  client_config.set_allow_suppression(config.allow_suppression());
  Random* random = new Random(InvalidationClientUtil::GetCurrentTimeMs(
              resources->internal_scheduler()));
  return new InvalidationClientImpl(
      resources, random, config.client_type(), config.client_name(),
      client_config, config.application_name(), listener);
}

// Deprecated, please the factory function that takes an
// InvalidationClientConfig instead.
InvalidationClient* CreateInvalidationClient(
    SystemResources* resources,
    int32_t client_type,
    const string& client_name,
    const string& application_name,
    InvalidationListener* listener) {
  InvalidationClientConfig config(
      client_type, client_name, application_name, true /* allowSuppression*/);
  return ClientFactory::Create(resources, config, listener);
}

InvalidationClient* ClientFactory::CreateForTest(
    SystemResources* resources,
    const InvalidationClientConfig& config,
    InvalidationListener* listener) {
  // Make a config with test params and construct an instance to return.
  ClientConfigP client_config;
  InvalidationClientCore::InitConfigForTest(&client_config);
  client_config.set_allow_suppression(config.allow_suppression());
  Random* random = new Random(InvalidationClientUtil::GetCurrentTimeMs(
              resources->internal_scheduler()));
  return new InvalidationClientImpl(
      resources, random, config.client_type(), config.client_name(),
      client_config, config.application_name(), listener);
}

InvalidationClient* CreateInvalidationClientForTest(
    SystemResources* resources,
    int32_t client_type,
    const string& client_name,
    const string& application_name,
    InvalidationListener* listener) {
  return ClientFactory::CreateForTest(
      resources,
      InvalidationClientConfig(client_type, client_name, application_name,
                               true /* allowSuppression */),
      listener);
}

}  // namespace invalidation
