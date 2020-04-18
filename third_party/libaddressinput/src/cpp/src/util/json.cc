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

#include "json.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/reader.h>

namespace i18n {
namespace addressinput {

using rapidjson::Document;
using rapidjson::kParseValidateEncodingFlag;
using rapidjson::Value;

class Json::JsonImpl {
 public:
  JsonImpl(const JsonImpl&) = delete;
  JsonImpl& operator=(const JsonImpl&) = delete;

  explicit JsonImpl(const std::string& json)
      : document_(new Document),
        value_(document_.get()),
        dictionaries_(),
        valid_(false) {
    document_->Parse<kParseValidateEncodingFlag>(json.c_str());
    valid_ = !document_->HasParseError() && document_->IsObject();
  }

  ~JsonImpl() {
    for (std::vector<const Json*>::const_iterator it = dictionaries_.begin();
         it != dictionaries_.end(); ++it) {
      delete *it;
    }
  }

  bool valid() const { return valid_; }

  const std::vector<const Json*>& GetSubDictionaries() {
    if (dictionaries_.empty()) {
      for (Value::ConstMemberIterator member = value_->MemberBegin();
           member != value_->MemberEnd(); ++member) {
        if (member->value.IsObject()) {
          dictionaries_.push_back(new Json(new JsonImpl(&member->value)));
        }
      }
    }
    return dictionaries_;
  }

  bool GetStringValueForKey(const std::string& key, std::string* value) const {
    assert(value != nullptr);

    Value::ConstMemberIterator member = value_->FindMember(key.c_str());
    if (member == value_->MemberEnd() || !member->value.IsString()) {
      return false;
    }

    value->assign(member->value.GetString(),
                  member->value.GetStringLength());
    return true;
  }

 private:
  // Does not take ownership of |value|.
  explicit JsonImpl(const Value* value)
      : document_(),
        value_(value),
        dictionaries_(),
        valid_(true) {
    assert(value_ != nullptr);
    assert(value_->IsObject());
  }

  // An owned JSON document. Can be nullptr if the JSON document is not owned.
  const std::unique_ptr<Document> document_;

  // A JSON document that is not owned. Cannot be nullptr. Can point to
  // document_.
  const Value* const value_;

  // Owned JSON objects of sub-dictionaries.
  std::vector<const Json*> dictionaries_;

  // True if the JSON object was parsed successfully.
  bool valid_;
};

Json::Json() : impl_() {}

Json::~Json() {}

bool Json::ParseObject(const std::string& json) {
  assert(impl_ == nullptr);
  impl_.reset(new JsonImpl(json));
  if (!impl_->valid()) {
    impl_.reset();
  }
  return impl_ != nullptr;
}

const std::vector<const Json*>& Json::GetSubDictionaries() const {
  assert(impl_ != nullptr);
  return impl_->GetSubDictionaries();
}

bool Json::GetStringValueForKey(const std::string& key,
                                std::string* value) const {
  assert(impl_ != nullptr);
  return impl_->GetStringValueForKey(key, value);
}

Json::Json(JsonImpl* impl) : impl_(impl) {}

}  // namespace addressinput
}  // namespace i18n
