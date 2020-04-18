// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/test/local_policy_test_server.h"

#include <ctype.h>
#include <stdint.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/numerics/safe_conversions.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "crypto/rsa_private_key.h"
#include "net/test/python_utils.h"

namespace policy {

namespace {

// Filename in the temporary directory storing the policy data.
const base::FilePath::CharType kPolicyFileName[] = FILE_PATH_LITERAL("policy");

// Private signing key file within the temporary directory.
const base::FilePath::CharType kSigningKeyFileName[] =
    FILE_PATH_LITERAL("signing_key");

// Private signing key signature file within the temporary directory.
const base::FilePath::CharType kSigningKeySignatureFileName[] =
    FILE_PATH_LITERAL("signing_key.sig");

// The file containing client definitions to be passed to the server.
const base::FilePath::CharType kClientStateFileName[] =
    FILE_PATH_LITERAL("clients");

// Dictionary keys for the client state file. Needs to be kept in sync with
// policy_testserver.py.
const char kClientStateKeyAllowedPolicyTypes[] = "allowed_policy_types";
const char kClientStateKeyDeviceId[] = "device_id";
const char kClientStateKeyDeviceToken[] = "device_token";
const char kClientStateKeyMachineName[] = "machine_name";
const char kClientStateKeyMachineId[] = "machine_id";

// Checks whether a given character should be replaced when constructing a file
// name. To keep things simple, this is a bit over-aggressive. Needs to be kept
// in sync with policy_testserver.py.
bool IsUnsafeCharacter(char c) {
  return !(isalnum(c) || c == '.' || c == '@' || c == '-');
}

}  // namespace

LocalPolicyTestServer::LocalPolicyTestServer()
    : net::LocalTestServer(net::BaseTestServer::TYPE_HTTP, base::FilePath()) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  CHECK(server_data_dir_.CreateUniqueTempDir());
  config_file_ = server_data_dir_.GetPath().Append(kPolicyFileName);
}

LocalPolicyTestServer::LocalPolicyTestServer(const base::FilePath& config_file)
    : net::LocalTestServer(net::BaseTestServer::TYPE_HTTP, base::FilePath()),
      config_file_(config_file) {}

LocalPolicyTestServer::LocalPolicyTestServer(const std::string& test_name)
    : net::LocalTestServer(net::BaseTestServer::TYPE_HTTP, base::FilePath()) {
  // Read configuration from a file in chrome/test/data/policy.
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath source_root;
  CHECK(base::PathService::Get(base::DIR_SOURCE_ROOT, &source_root));
  config_file_ = source_root
      .AppendASCII("chrome")
      .AppendASCII("test")
      .AppendASCII("data")
      .AppendASCII("policy")
      .AppendASCII(base::StringPrintf("policy_%s.json", test_name.c_str()));
}

LocalPolicyTestServer::~LocalPolicyTestServer() {}

bool LocalPolicyTestServer::SetSigningKeyAndSignature(
    const crypto::RSAPrivateKey* key, const std::string& signature) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  CHECK(server_data_dir_.IsValid());

  std::vector<uint8_t> signing_key_bits;
  if (!key->ExportPrivateKey(&signing_key_bits))
    return false;

  policy_key_ = server_data_dir_.GetPath().Append(kSigningKeyFileName);
  int bytes_written = base::WriteFile(
      policy_key_, reinterpret_cast<const char*>(signing_key_bits.data()),
      signing_key_bits.size());

  if (bytes_written != base::checked_cast<int>(signing_key_bits.size()))
    return false;

  // Write the signature data.
  base::FilePath signature_file =
      server_data_dir_.GetPath().Append(kSigningKeySignatureFileName);
  bytes_written = base::WriteFile(
      signature_file,
      signature.c_str(),
      signature.size());

  return bytes_written == base::checked_cast<int>(signature.size());
}

void LocalPolicyTestServer::EnableAutomaticRotationOfSigningKeys() {
  automatic_rotation_of_signing_keys_enabled_ = true;
}

void LocalPolicyTestServer::RegisterClient(const std::string& dm_token,
                                           const std::string& device_id) {
  CHECK(server_data_dir_.IsValid());

  std::unique_ptr<base::DictionaryValue> client_dict(
      new base::DictionaryValue());
  client_dict->SetString(kClientStateKeyDeviceId, device_id);
  client_dict->SetString(kClientStateKeyDeviceToken, dm_token);
  client_dict->SetString(kClientStateKeyMachineName, std::string());
  client_dict->SetString(kClientStateKeyMachineId, std::string());

  // Allow all policy types for now.
  std::unique_ptr<base::ListValue> types(new base::ListValue());
  types->AppendString(dm_protocol::kChromeDevicePolicyType);
  types->AppendString(dm_protocol::kChromeUserPolicyType);
  types->AppendString(dm_protocol::kChromePublicAccountPolicyType);
  types->AppendString(dm_protocol::kChromeExtensionPolicyType);
  types->AppendString(dm_protocol::kChromeSigninExtensionPolicyType);

  client_dict->Set(kClientStateKeyAllowedPolicyTypes, std::move(types));
  clients_.Set(dm_token, std::move(client_dict));
}

bool LocalPolicyTestServer::UpdatePolicy(const std::string& type,
                                         const std::string& entity_id,
                                         const std::string& policy) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  CHECK(server_data_dir_.IsValid());

  std::string selector = GetSelector(type, entity_id);
  base::FilePath policy_file = server_data_dir_.GetPath().AppendASCII(
      base::StringPrintf("policy_%s.bin", selector.c_str()));

  return base::WriteFile(policy_file, policy.c_str(), policy.size()) ==
         base::checked_cast<int>(policy.size());
}

bool LocalPolicyTestServer::UpdatePolicyData(const std::string& type,
                                             const std::string& entity_id,
                                             const std::string& data) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  CHECK(server_data_dir_.IsValid());

  std::string selector = GetSelector(type, entity_id);
  base::FilePath data_file = server_data_dir_.GetPath().AppendASCII(
      base::StringPrintf("policy_%s.data", selector.c_str()));

  return base::WriteFile(data_file, data.c_str(), data.size()) ==
         base::checked_cast<int>(data.size());
}

GURL LocalPolicyTestServer::GetServiceURL() const {
  return GetURL("device_management");
}

bool LocalPolicyTestServer::SetPythonPath() const {
  base::ScopedAllowBlockingForTesting allow_blocking;
  if (!net::LocalTestServer::SetPythonPath())
    return false;

  // Add the net/tools/testserver directory to the path.
  base::FilePath net_testserver_path;
  if (!LocalTestServer::GetTestServerPath(&net_testserver_path)) {
    LOG(ERROR) << "Failed to get net testserver path.";
    return false;
  }
  AppendToPythonPath(net_testserver_path.DirName());

  // We need protobuf python bindings.
  base::FilePath third_party_dir;
  if (!base::PathService::Get(base::DIR_SOURCE_ROOT, &third_party_dir)) {
    LOG(ERROR) << "Failed to get DIR_SOURCE_ROOT";
    return false;
  }
  AppendToPythonPath(third_party_dir
                     .AppendASCII("third_party")
                     .AppendASCII("protobuf")
                     .AppendASCII("python"));

  // Add the generated python protocol buffer bindings.
  base::FilePath pyproto_dir;
  if (!GetPyProtoPath(&pyproto_dir)) {
    LOG(ERROR) << "Cannot find pyproto dir for generated code.";
    return false;
  }

  AppendToPythonPath(pyproto_dir
                     .AppendASCII("components")
                     .AppendASCII("policy")
                     .AppendASCII("proto"));
#if defined(OS_CHROMEOS)
  AppendToPythonPath(pyproto_dir
                     .AppendASCII("chrome")
                     .AppendASCII("browser")
                     .AppendASCII("chromeos")
                     .AppendASCII("policy")
                     .AppendASCII("proto"));
#endif

  return true;
}

bool LocalPolicyTestServer::GetTestServerPath(
    base::FilePath* testserver_path) const {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath source_root;
  if (!base::PathService::Get(base::DIR_SOURCE_ROOT, &source_root)) {
    LOG(ERROR) << "Failed to get DIR_SOURCE_ROOT";
    return false;
  }
  *testserver_path = source_root
      .AppendASCII("chrome")
      .AppendASCII("browser")
      .AppendASCII("policy")
      .AppendASCII("test")
      .AppendASCII("policy_testserver.py");
  return true;
}

bool LocalPolicyTestServer::GenerateAdditionalArguments(
    base::DictionaryValue* arguments) const {
  base::ScopedAllowBlockingForTesting allow_blocking;
  if (!net::LocalTestServer::GenerateAdditionalArguments(arguments))
    return false;

  arguments->SetString("config-file", config_file_.AsUTF8Unsafe());
  if (!policy_key_.empty())
    arguments->SetString("policy-key", policy_key_.AsUTF8Unsafe());
  if (automatic_rotation_of_signing_keys_enabled_) {
    arguments->Set("rotate-policy-keys-automatically",
                   std::make_unique<base::Value>());
  }
  if (server_data_dir_.IsValid()) {
    arguments->SetString("data-dir", server_data_dir_.GetPath().AsUTF8Unsafe());

    if (!clients_.empty()) {
      std::string json;
      base::JSONWriter::Write(clients_, &json);
      base::FilePath client_state_file =
          server_data_dir_.GetPath().Append(kClientStateFileName);
      if (base::WriteFile(client_state_file, json.c_str(), json.size()) !=
          base::checked_cast<int>(json.size())) {
        return false;
      }
      arguments->SetString("client-state", client_state_file.AsUTF8Unsafe());
    }
  }

  return true;
}

std::string LocalPolicyTestServer::GetSelector(const std::string& type,
                                               const std::string& entity_id) {
  std::string selector = type;
  if (!entity_id.empty())
    selector = base::StringPrintf("%s/%s", type.c_str(), entity_id.c_str());
  std::replace_if(selector.begin(), selector.end(), IsUnsafeCharacter, '_');
  return selector;
}

}  // namespace policy
