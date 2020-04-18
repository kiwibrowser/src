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
// The interface to be implemented by the user of the library to enable storing
// address metadata (e.g. on disk).

#ifndef I18N_ADDRESSINPUT_STORAGE_H_
#define I18N_ADDRESSINPUT_STORAGE_H_

#include <libaddressinput/callback.h>

#include <string>

namespace i18n {
namespace addressinput {

// Stores address metadata. The data must be allocated on the heap, passing
// ownership to the called function. Sample usage:
//
//    class MyStorage : public Storage {
//     public:
//      virtual void Put(const std::string& key, std::string* data) {
//        ...
//        delete data;
//      }
//
//      virtual void Get(const std::string& key,
//                       const Callback& data_ready) const {
//        bool success = ...
//        std::string* data = new ...
//        data_ready(success, key, data);
//      }
//    };
class Storage {
 public:
  typedef i18n::addressinput::Callback<const std::string&,
                                       std::string*> Callback;

  virtual ~Storage() {}

  // Stores |data| for |key|, where |data| is an object allocated on the heap,
  // which Storage takes ownership of.
  virtual void Put(const std::string& key, std::string* data) = 0;

  // Retrieves the data for |key| and invokes the |data_ready| callback.
  virtual void Get(const std::string& key,
                   const Callback& data_ready) const = 0;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_STORAGE_H_
