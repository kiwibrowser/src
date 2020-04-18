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
// An object to retrieve data.

#ifndef I18N_ADDRESSINPUT_RETRIEVER_H_
#define I18N_ADDRESSINPUT_RETRIEVER_H_

#include <libaddressinput/callback.h>

#include <memory>
#include <string>

namespace i18n {
namespace addressinput {

class Source;
class Storage;
class ValidatingStorage;

// Retrieves data. Sample usage:
//    Source* source = ...;
//    Storage* storage = ...;
//    Retriever retriever(source, storage);
//    const std::unique_ptr<const Retriever::Callback> retrieved(
//        BuildCallback(this, &MyClass::OnDataRetrieved));
//    retriever.Retrieve("data/CA/AB--fr", *retrieved);
class Retriever {
 public:
  typedef i18n::addressinput::Callback<const std::string&,
                                       const std::string&> Callback;

  Retriever(const Retriever&) = delete;
  Retriever& operator=(const Retriever&) = delete;

  // Takes ownership of |source| and |storage|.
  Retriever(const Source* source, Storage* storage);
  ~Retriever();

  // Retrieves the data for |key| and invokes the |retrieved| callback. Checks
  // for the data in |storage_| first. If storage does not have the data for
  // |key|, then gets the data from |source_| and places it in storage. If the
  // data in storage is corrupted, then it's discarded and requested anew. If
  // the data is stale, then it's requested anew. If the request fails, then
  // stale data will be returned this one time. Any subsequent call to
  // Retrieve() will attempt to get fresh data again.
  void Retrieve(const std::string& key, const Callback& retrieved) const;

 private:
  std::unique_ptr<const Source> source_;
  std::unique_ptr<ValidatingStorage> storage_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_RETRIEVER_H_
