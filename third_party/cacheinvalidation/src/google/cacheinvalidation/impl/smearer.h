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

// An abstraction to "smear" values by a given percent. Useful for randomizing
// delays a little bit so that (say) processes do not get synchronized on time
// inadvertently, e.g., a heartbeat task that sends a message every few minutes
// is smeared so that all clients do not end up sending a message at the same
// time. In particular, given a |delay|, returns a value that is randomly
// distributed between
// [delay - smearPercent * delay, delay + smearPercent * delay]

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_SMEARER_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_SMEARER_H_

#include "google/cacheinvalidation/deps/logging.h"
#include "google/cacheinvalidation/deps/random.h"
#include "google/cacheinvalidation/deps/scoped_ptr.h"

namespace invalidation {

class Smearer {
 public:
  /* Creates a smearer with the given random number generator.
   * REQUIRES: 0 <= smear_percent <= 100
   * Caller continues to own space for random.
   */
  Smearer(Random* random, int smear_percent) : random_(random),
          smear_fraction_(smear_percent / 100.0) {
    CHECK((smear_percent >= 0) && (smear_percent <= 100));
  }

  /* Given a delay, returns a value that is randomly distributed between
   * (delay - smear_percent * delay, delay + smear_percent * delay)
   */
  TimeDelta GetSmearedDelay(TimeDelta delay) {
    // Get a random number between -1 and 1 and then multiply that by the
    // smear fraction.
    double smear_factor = (2 * random_->RandDouble() - 1.0) * smear_fraction_;
    return TimeDelta::FromMilliseconds(
        delay.InMilliseconds() * (1.0 + smear_factor));
  }

 private:
  Random* random_;

  /* The percentage (0, 1.0] for smearing the delay. */
  double smear_fraction_;
};
}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_SMEARER_H_
