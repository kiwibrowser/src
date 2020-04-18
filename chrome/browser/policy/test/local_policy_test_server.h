// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_TEST_LOCAL_POLICY_TEST_SERVER_H_
#define CHROME_BROWSER_POLICY_TEST_LOCAL_POLICY_TEST_SERVER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/values.h"
#include "net/test/spawned_test_server/local_test_server.h"
#include "url/gurl.h"

namespace crypto {
class RSAPrivateKey;
}

namespace policy {

// Runs a python implementation of the cloud policy server on the local machine.
class LocalPolicyTestServer : public net::LocalTestServer {
 public:
  // Initializes the test server to serve its policy from a temporary directory,
  // the contents of which can be updated via UpdatePolicy().
  LocalPolicyTestServer();

  // Initializes a test server configured by the configuration file
  // |config_file|.
  explicit LocalPolicyTestServer(const base::FilePath& config_file);

  // Initializes the test server with the configuration read from
  // chrome/test/data/policy/policy_|test_name|.json.
  explicit LocalPolicyTestServer(const std::string& test_name);

  ~LocalPolicyTestServer() override;

  // Sets the policy signing key and verification signature used by the server.
  // This must be called before starting the server, and only works when the
  // server serves from a temporary directory.
  bool SetSigningKeyAndSignature(const crypto::RSAPrivateKey* key,
                                 const std::string& signature);

  // Enables the automatic rotation of the policy signing keys with each policy
  // fetch request. This must be called before starting the server, and only
  // works when the server serves from a temporary directory.
  void EnableAutomaticRotationOfSigningKeys();

  // Pre-configures a registered client so the server returns policy without the
  // client having to make a registration call. This must be called before
  // starting the server, and only works when the server serves from a temporary
  // directory.
  void RegisterClient(const std::string& dm_token,
                      const std::string& device_id);

  // Updates policy served by the server for a given (type, entity_id) pair.
  // This only works when the server serves from a temporary directory.
  //
  // |type| is the policy type as requested in the protocol via
  // |PolicyFetchRequest.policy_type|. |policy| is the payload data in the
  // format appropriate for |type|, which is usually a serialized protobuf (for
  // example, CloudPolicySettings or ChromeDeviceSettingsProto).
  bool UpdatePolicy(const std::string& type,
                    const std::string& entity_id,
                    const std::string& policy);

  // Updates the external policy data served by the server for a given
  // (type, entity_id) pair, at the /externalpolicydata path. Requests to that
  // URL must include a 'key' parameter, whose value is the |type| and
  // |entity_id| values joined by a '/'.
  //
  // If this data is set but no policy is set for the (type, entity_id) pair,
  // then an ExternalPolicyData protobuf is automatically served that points to
  // this data.
  //
  // This only works when the server serves from a temporary directory.
  bool UpdatePolicyData(const std::string& type,
                        const std::string& entity_id,
                        const std::string& data);

  // Gets the service URL.
  GURL GetServiceURL() const;

  // net::LocalTestServer:
  bool SetPythonPath() const override;
  bool GetTestServerPath(base::FilePath* testserver_path) const override;
  bool GenerateAdditionalArguments(
      base::DictionaryValue* arguments) const override;

 private:
  std::string GetSelector(const std::string& type,
                          const std::string& entity_id);

  base::FilePath config_file_;
  base::FilePath policy_key_;
  base::DictionaryValue clients_;
  base::ScopedTempDir server_data_dir_;
  bool automatic_rotation_of_signing_keys_enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(LocalPolicyTestServer);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_TEST_LOCAL_POLICY_TEST_SERVER_H_
