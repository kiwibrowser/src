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
//
// ValidatingStorage saves data with checksum and timestamp using
// ValidatingUtil.

#include "validating_storage.h"

#include <libaddressinput/callback.h>
#include <libaddressinput/storage.h>

#include <cassert>
#include <cstddef>
#include <ctime>
#include <memory>
#include <string>

#include "validating_util.h"

namespace i18n {
namespace addressinput {

namespace {

class Helper {
 public:
  Helper(const Helper&) = delete;
  Helper& operator=(const Helper&) = delete;

  Helper(const std::string& key,
         const ValidatingStorage::Callback& data_ready,
         const Storage& wrapped_storage)
      : data_ready_(data_ready),
        wrapped_data_ready_(BuildCallback(this, &Helper::OnWrappedDataReady)) {
    wrapped_storage.Get(key, *wrapped_data_ready_);
  }

 private:
  ~Helper() {}

  void OnWrappedDataReady(bool success,
                          const std::string& key,
                          std::string* data) {
    if (success) {
      assert(data != nullptr);
      bool is_stale =
          !ValidatingUtil::UnwrapTimestamp(data, std::time(nullptr));
      bool is_corrupted = !ValidatingUtil::UnwrapChecksum(data);
      success = !is_corrupted && !is_stale;
      if (is_corrupted) {
        delete data;
        data = nullptr;
      }
    } else {
      delete data;
      data = nullptr;
    }
    data_ready_(success, key, data);
    delete this;
  }

  const Storage::Callback& data_ready_;
  const std::unique_ptr<const Storage::Callback> wrapped_data_ready_;
};

}  // namespace

ValidatingStorage::ValidatingStorage(Storage* storage)
    : wrapped_storage_(storage) {
  assert(wrapped_storage_ != nullptr);
}

ValidatingStorage::~ValidatingStorage() {}

void ValidatingStorage::Put(const std::string& key, std::string* data) {
  assert(data != nullptr);
  ValidatingUtil::Wrap(std::time(nullptr), data);
  wrapped_storage_->Put(key, data);
}

void ValidatingStorage::Get(const std::string& key,
                            const Callback& data_ready) const {
  new Helper(key, data_ready, *wrapped_storage_);
}

}  // namespace addressinput
}  // namespace i18n
