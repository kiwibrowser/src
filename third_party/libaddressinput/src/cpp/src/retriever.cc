// Copyright (C) 2013 Google Inc.
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

#include "retriever.h"

#include <libaddressinput/callback.h>
#include <libaddressinput/source.h>
#include <libaddressinput/storage.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

#include "validating_storage.h"

namespace i18n {
namespace addressinput {

namespace {

class Helper {
 public:
  Helper(const Helper&) = delete;
  Helper& operator=(const Helper&) = delete;

  // Does not take ownership of its parameters.
  Helper(const std::string& key,
         const Retriever::Callback& retrieved,
         const Source& source,
         ValidatingStorage* storage)
      : retrieved_(retrieved),
        source_(source),
        storage_(storage),
        fresh_data_ready_(BuildCallback(this, &Helper::OnFreshDataReady)),
        validated_data_ready_(
            BuildCallback(this, &Helper::OnValidatedDataReady)),
        stale_data_() {
    assert(storage_ != nullptr);
    storage_->Get(key, *validated_data_ready_);
  }

 private:
  ~Helper() {}

  void OnValidatedDataReady(bool success,
                            const std::string& key,
                            std::string* data) {
    if (success) {
      assert(data != nullptr);
      retrieved_(success, key, *data);
      delete this;
    } else {
      // Validating storage returns (false, key, stale-data) for valid but stale
      // data. If |data| is empty, however, then it's either missing or invalid.
      if (data != nullptr && !data->empty()) {
        stale_data_ = *data;
      }
      source_.Get(key, *fresh_data_ready_);
    }
    delete data;
  }

  void OnFreshDataReady(bool success,
                        const std::string& key,
                        std::string* data) {
    if (success) {
      assert(data != nullptr);
      retrieved_(true, key, *data);
      storage_->Put(key, data);
      data = nullptr;  // Deleted by Storage::Put().
    } else if (!stale_data_.empty()) {
      // Reuse the stale data if a download fails. It's better to have slightly
      // outdated validation rules than to suddenly lose validation ability.
      retrieved_(true, key, stale_data_);
    } else {
      retrieved_(false, key, std::string());
    }
    delete data;
    delete this;
  }

  const Retriever::Callback& retrieved_;
  const Source& source_;
  ValidatingStorage* storage_;
  const std::unique_ptr<const Source::Callback> fresh_data_ready_;
  const std::unique_ptr<const Storage::Callback> validated_data_ready_;
  std::string stale_data_;
};

}  // namespace

Retriever::Retriever(const Source* source, Storage* storage)
    : source_(source), storage_(new ValidatingStorage(storage)) {
  assert(source_ != nullptr);
  assert(storage_ != nullptr);
}

Retriever::~Retriever() {}

void Retriever::Retrieve(const std::string& key,
                         const Callback& retrieved) const {
  new Helper(key, retrieved, *source_, storage_.get());
}

}  // namespace addressinput
}  // namespace i18n
