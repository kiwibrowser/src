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

// Validator for v2 protocol messages.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_TICL_MESSAGE_VALIDATOR_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_TICL_MESSAGE_VALIDATOR_H_

#include "google/cacheinvalidation/impl/client-protocol-namespace-fix.h"

namespace invalidation {

class Logger;

class TiclMessageValidator {
 public:
  TiclMessageValidator(Logger* logger) : logger_(logger) {}

  // Generic IsValid() method.  Delegates to the private |Validate| helper
  // method.
  template<typename T>
  bool IsValid(const T& message) {
    bool result = true;
    Validate(message, &result);
    return result;
  }

 private:
  // Validates a message.  For each type of message to be validated, there
  // should be a specialization of this method.  Instead of returning a boolean,
  // the method stores |false| in |*result| if the message is invalid.  Thus,
  // the caller must initialize |*result| to |true|.  Following this pattern
  // allows the specific validation methods to be simpler (i.e., a method that
  // accepts all messages has an empty body instead of having to return |true|).
  template<typename T>
  void Validate(const T& message, bool* result);

 private:
  Logger* logger_;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_TICL_MESSAGE_VALIDATOR_H_
