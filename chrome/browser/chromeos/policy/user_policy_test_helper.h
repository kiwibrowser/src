// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_USER_POLICY_TEST_HELPER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_USER_POLICY_TEST_HELPER_H_

#include <memory>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"

class Profile;

namespace base {
class CommandLine;
class FilePath;
class DictionaryValue;
}

namespace policy {

class LocalPolicyTestServer;

// This class can be used to apply a user policy to the profile in a
// BrowserTest.
class UserPolicyTestHelper {
 public:
  explicit UserPolicyTestHelper(const std::string& account_id);
  virtual ~UserPolicyTestHelper();

  // Must be called after construction to start the policy test server.
  void Init(const base::DictionaryValue& mandatory_policy,
            const base::DictionaryValue& recommended_policy);

  // Must be used during BrowserTestBase::SetUpCommandLine to direct Chrome to
  // the policy test server.
  void UpdateCommandLine(base::CommandLine* command_line) const;

  // Can be optionally used to wait for the initial policy to be applied to the
  // profile. Alternatively, a login can be simulated, which makes it
  // unnecessary to call this function.
  void WaitForInitialPolicy(Profile* profile);

  // Update the policy test server with the given policy. Then refresh and wait
  // for the new policy being applied to |profile|.
  void UpdatePolicy(const base::DictionaryValue& mandatory_policy,
                    const base::DictionaryValue& recommended_policy,
                    Profile* profile);

  void DeletePolicyFile();

 private:
  void WritePolicyFile(const base::DictionaryValue& mandatory,
                       const base::DictionaryValue& recommended);
  base::FilePath PolicyFilePath() const;

  const std::string account_id_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<LocalPolicyTestServer> test_server_;

  DISALLOW_COPY_AND_ASSIGN(UserPolicyTestHelper);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_USER_POLICY_TEST_HELPER_H_
