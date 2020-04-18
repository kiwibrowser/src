// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/cycle/commit_counters.h"

#include "base/json/json_string_value_serializer.h"

namespace syncer {

CommitCounters::CommitCounters()
    : num_commits_attempted(0),
      num_commits_success(0),
      num_commits_conflict(0),
      num_commits_error(0) {}

CommitCounters::~CommitCounters() {}

std::unique_ptr<base::DictionaryValue> CommitCounters::ToValue() const {
  std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
  value->SetInteger("numCommitsAttempted", num_commits_attempted);
  value->SetInteger("numCommitsSuccess", num_commits_success);
  value->SetInteger("numCommitsConflict", num_commits_conflict);
  value->SetInteger("numCommitsError", num_commits_error);
  return value;
}

std::string CommitCounters::ToString() const {
  std::string result;
  std::unique_ptr<base::DictionaryValue> value = ToValue();
  JSONStringValueSerializer serializer(&result);
  serializer.Serialize(*value);
  return result;
}

}  // namespace syncer
