// Copyright (C) 2014 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef I18N_ADDRESSINPUT_ONDEMAND_SUPPLY_TASK_H_
#define I18N_ADDRESSINPUT_ONDEMAND_SUPPLY_TASK_H_

#include <libaddressinput/supplier.h>

#include <map>
#include <memory>
#include <set>
#include <string>

#include "retriever.h"

namespace i18n {
namespace addressinput {

class LookupKey;
class Rule;

// An OndemandSupplyTask object encapsulates the information necessary to
// retrieve the set of Rule objects corresponding to a LookupKey and call a
// callback when that has been done. Calling the Retrieve() method will load
// required metadata, then call the callback and delete the OndemandSupplyTask
// object itself.
class OndemandSupplyTask {
 public:
  OndemandSupplyTask(const OndemandSupplyTask&) = delete;
  OndemandSupplyTask& operator=(const OndemandSupplyTask&) = delete;

  OndemandSupplyTask(const LookupKey& lookup_key,
                     std::map<std::string, const Rule*>* rules,
                     const Supplier::Callback& supplied);
  ~OndemandSupplyTask();

  // Adds lookup key string |key| to the queue of data to be retrieved.
  void Queue(const std::string& key);

  // Retrieves and parses data for all queued keys, then calls |supplied_|.
  void Retrieve(const Retriever& retriever);

  Supplier::RuleHierarchy hierarchy_;

 private:
  void Load(bool success, const std::string& key, const std::string& data);
  void Loaded();

  std::set<std::string> pending_;
  const LookupKey& lookup_key_;
  std::map<std::string, const Rule*>* const rule_cache_;
  const Supplier::Callback& supplied_;
  const std::unique_ptr<const Retriever::Callback> retrieved_;
  bool success_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_ONDEMAND_SUPPLY_TASK_H_
