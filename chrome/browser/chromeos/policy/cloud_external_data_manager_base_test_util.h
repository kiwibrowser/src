// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_CLOUD_EXTERNAL_DATA_MANAGER_BASE_TEST_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_CLOUD_EXTERNAL_DATA_MANAGER_BASE_TEST_UTIL_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"

namespace base {
class DictionaryValue;
}

namespace policy {

class CloudPolicyCore;

namespace test {

// Passes |data| to |destination| and invokes |done_callback| to indicate that
// the |data| has been retrieved.
void ExternalDataFetchCallback(std::unique_ptr<std::string>* destination,
                               const base::Closure& done_callback,
                               std::unique_ptr<std::string> data);

// Constructs a value that points a policy referencing external data at |url|
// and sets the expected hash of the external data to that of |data|.
std::unique_ptr<base::DictionaryValue> ConstructExternalDataReference(
    const std::string& url,
    const std::string& data);

// TODO(bartfab): Makes an arbitrary |policy| in |core| reference external data
// as specified in |metadata|. This is only done because there are no policies
// that reference external data yet. Once the first such policy is added, it
// will be sufficient to set its value to |metadata| and this method should be
// removed.
void SetExternalDataReference(CloudPolicyCore* core,
                              const std::string& policy,
                              std::unique_ptr<base::DictionaryValue> metadata);

}  // namespace test
}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_CLOUD_EXTERNAL_DATA_MANAGER_BASE_TEST_UTIL_H_
