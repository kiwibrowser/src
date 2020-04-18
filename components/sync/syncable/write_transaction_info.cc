// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/write_transaction_info.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"

namespace syncer {
namespace syncable {

WriteTransactionInfo::WriteTransactionInfo(
    int64_t id,
    base::Location location,
    WriterTag writer,
    ImmutableEntryKernelMutationMap mutations)
    : id(id), location_(location), writer(writer), mutations(mutations) {}

WriteTransactionInfo::WriteTransactionInfo() : id(-1), writer(INVALID) {}

WriteTransactionInfo::WriteTransactionInfo(const WriteTransactionInfo& other) =
    default;

WriteTransactionInfo::~WriteTransactionInfo() {}

std::unique_ptr<base::DictionaryValue> WriteTransactionInfo::ToValue(
    size_t max_mutations_size) const {
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetString("id", base::Int64ToString(id));
  dict->SetString("location", location_.ToString());
  dict->SetString("writer", WriterTagToString(writer));
  std::unique_ptr<base::Value> mutations_value;
  const size_t mutations_size = mutations.Get().size();
  if (mutations_size <= max_mutations_size) {
    mutations_value = EntryKernelMutationMapToValue(mutations.Get());
  } else {
    mutations_value = std::make_unique<base::Value>(
        base::NumberToString(mutations_size) + " mutations");
  }
  dict->Set("mutations", std::move(mutations_value));
  return dict;
}

}  // namespace syncable
}  // namespace syncer
