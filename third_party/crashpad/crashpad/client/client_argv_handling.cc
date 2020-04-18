// Copyright 2018 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "client/client_argv_handling.h"

#include "base/strings/stringprintf.h"

namespace crashpad {

namespace {

std::string FormatArgumentString(const std::string& name,
                                 const std::string& value) {
  return base::StringPrintf("--%s=%s", name.c_str(), value.c_str());
}

}  // namespace

void BuildHandlerArgvStrings(
    const base::FilePath& handler,
    const base::FilePath& database,
    const base::FilePath& metrics_dir,
    const std::string& url,
    const std::map<std::string, std::string>& annotations,
    const std::vector<std::string>& arguments,
    std::vector<std::string>* argv_strings) {
  argv_strings->clear();

  argv_strings->push_back(handler.value());
  for (const auto& argument : arguments) {
    argv_strings->push_back(argument);
  }

  if (!database.empty()) {
    argv_strings->push_back(FormatArgumentString("database", database.value()));
  }

  if (!metrics_dir.empty()) {
    argv_strings->push_back(
        FormatArgumentString("metrics-dir", metrics_dir.value()));
  }

  if (!url.empty()) {
    argv_strings->push_back(FormatArgumentString("url", url));
  }

  for (const auto& kv : annotations) {
    argv_strings->push_back(
        FormatArgumentString("annotation", kv.first + '=' + kv.second));
  }
}

void ConvertArgvStrings(const std::vector<std::string>& argv_strings,
                        std::vector<const char*>* argv) {
  argv->clear();
  argv->reserve(argv_strings.size() + 1);
  for (const auto& arg : argv_strings) {
    argv->push_back(arg.c_str());
  }
  argv->push_back(nullptr);
}

}  // namespace crashpad
