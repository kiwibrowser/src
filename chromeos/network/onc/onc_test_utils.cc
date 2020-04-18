// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/onc/onc_test_utils.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/values.h"
#include "chromeos/chromeos_test_utils.h"

namespace chromeos {
namespace onc {
namespace test_utils {

namespace {

// The name of the component directory to get the test data from.
const char kNetworkComponentDirectory[] = "network";

}  // namespace

std::string ReadTestData(const std::string& filename) {
  base::FilePath path;
  if (!chromeos::test_utils::GetTestDataPath(kNetworkComponentDirectory,
                                             filename,
                                             &path)) {
    NOTREACHED() << "Unable to get test data path for "
                 << kNetworkComponentDirectory << "/" << filename;
    return "";
  }
  std::string result;
  base::ReadFileToString(path, &result);
  return result;
}

std::unique_ptr<base::DictionaryValue> ReadTestDictionary(
    const std::string& filename) {
  std::unique_ptr<base::DictionaryValue> dict;
  base::FilePath path;
  if (!chromeos::test_utils::GetTestDataPath(kNetworkComponentDirectory,
                                             filename,
                                             &path)) {
    NOTREACHED() << "Unable to get test dictionary path for "
                 << kNetworkComponentDirectory << "/" << filename;
    return dict;
  }

  JSONFileValueDeserializer deserializer(path,
                                         base::JSON_ALLOW_TRAILING_COMMAS);

  std::string error_message;
  std::unique_ptr<base::Value> content =
      deserializer.Deserialize(NULL, &error_message);
  CHECK(content != NULL) << "Couldn't json-deserialize file '"
                         << filename << "': " << error_message;

  dict = base::DictionaryValue::From(std::move(content));
  CHECK(dict) << "File '" << filename
              << "' does not contain a dictionary as expected, but type "
              << content->type();
  return dict;
}

::testing::AssertionResult Equals(const base::Value* expected,
                                  const base::Value* actual) {
  CHECK(expected != NULL);
  if (actual == NULL)
    return ::testing::AssertionFailure() << "Actual value pointer is NULL";

  if (*expected == *actual)
    return ::testing::AssertionSuccess() << "Values are equal";

  return ::testing::AssertionFailure() << "Values are unequal.\n"
                                       << "Expected value:\n" << *expected
                                       << "Actual value:\n" << *actual;
}

}  // namespace test_utils
}  // namespace onc
}  // namespace chromeos
