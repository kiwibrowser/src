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

//
// Factory for the invalidation client library.

#ifndef GOOGLE_CACHEINVALIDATION_INCLUDE_INVALIDATION_CLIENT_FACTORY_H_
#define GOOGLE_CACHEINVALIDATION_INCLUDE_INVALIDATION_CLIENT_FACTORY_H_

#include <stdint.h>

#include <string>

#include "google/cacheinvalidation/include/types.h"
#include "google/cacheinvalidation/include/invalidation-listener.h"
#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/deps/stl-namespace.h"

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::string;

/* Application-provided configuration for an invalidation client. */
class InvalidationClientConfig {
 public:
  /* Constructs an InvalidationClientConfig instance.
   *
   * Arguments:
   *   client_type Client type code as assigned by the notification system's
   *      backend.
   *   client_name Id/name of the client in the application's own naming
   *      scheme.
   *   application_name Name of the application using the library (for
   *      debugging/monitoring)
   *   allow_suppression If false, invalidateUnknownVersion() is called
   *      whenever suppression occurs.
   */
  InvalidationClientConfig(int client_type,
                           const string& client_name,
                           const string& application_name,
                           bool allow_suppression) :
      client_type_(client_type), client_name_(client_name),
      application_name_(application_name),
      allow_suppression_(allow_suppression) {
  }

  int32_t client_type() const {
    return client_type_;
  }

  const string& client_name() const {
    return client_name_;
  }

  const string& application_name() const {
    return application_name_;
  }

  bool allow_suppression() const {
    return allow_suppression_;
  }

 private:
  const int32_t client_type_;
  const string client_name_;
  const string application_name_;
  const bool allow_suppression_;
};

// A class for new factory methods.  These methods will be static, so this class
// is essentially just a namespace.  This is more consistent with how the
// factory works in other languages, and it avoids overload issues with the old
// methods defined below.
class ClientFactory {
 public:
  /* Constructs an invalidation client library instance with a default
   * configuration. Caller owns returned space.
   *
   * Arguments:
   *   resources SystemResources to use for logging, scheduling, persistence,
   *       and network connectivity
   *   config configuration provided by the application
   *   listener callback object for invalidation events
   */
  static InvalidationClient* Create(
      SystemResources* resources,
      const InvalidationClientConfig& config,
      InvalidationListener* listener);

  /* Constructs an invalidation client library instance with a configuration
   * initialized for testing. Caller owns returned space.
   *
   * Arguments:
   *   resources SystemResources to use for logging, scheduling, persistence,
   *       and network connectivity
   *   client_type client type code as assigned by the notification system's
   *       backend
   *   client_name id/name of the client in the application's own naming scheme
   *   application_name name of the application using the library (for
   *       debugging/monitoring)
   *   listener callback object for invalidation events
   */
  static InvalidationClient* CreateForTest(
      SystemResources* resources,
      const InvalidationClientConfig& config,
      InvalidationListener* listener);
};

/* Constructs an invalidation client library instance with a default
 * configuration. Deprecated, please use the version which takes an
 * InvalidationClientConfig. Caller owns returned space.
 *
 * Arguments:
 *   resources SystemResources to use for logging, scheduling, persistence,
 *       and network connectivity
 *   client_type client type code as assigned by the notification system's
 *       backend
 *   client_name id/name of the client in the application's own naming scheme
 *   application_name name of the application using the library (for
 *       debugging/monitoring)
 *   listener callback object for invalidation events
 */
InvalidationClient* CreateInvalidationClient(
    SystemResources* resources,
    int client_type,
    const string& client_name,
    const string& application_name,
    InvalidationListener* listener);

/* Constructs an invalidation client library instance with a configuration
 * initialized for testing. Deprecated, please use the version which takes an
 * InvalidationClientConfig. Caller owns returned space.
 *
 * Arguments:
 *   resources SystemResources to use for logging, scheduling, persistence,
 *       and network connectivity
 *   client_type client type code as assigned by the notification system's
 *       backend
 *   client_name id/name of the client in the application's own naming scheme
 *   application_name name of the application using the library (for
 *       debugging/monitoring)
 *   listener callback object for invalidation events
 */
InvalidationClient* CreateInvalidationClientForTest(
    SystemResources* resources,
    int client_type,
    const string& client_name,
    const string& application_name,
    InvalidationListener* listener);

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_INCLUDE_INVALIDATION_CLIENT_FACTORY_H_
