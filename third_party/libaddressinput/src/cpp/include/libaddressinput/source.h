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
// The interface to be implemented by the user of the library to access address
// metadata, typically by downloading this from the address metadata server or
// by linking the metadata into the binary.

#ifndef I18N_ADDRESSINPUT_SOURCE_H_
#define I18N_ADDRESSINPUT_SOURCE_H_

#include <libaddressinput/callback.h>

#include <string>

namespace i18n {
namespace addressinput {

// Gets address metadata. The callback data must be allocated on the heap,
// passing ownership to the callback. Sample usage:
//
//    class MySource : public Source {
//     public:
//      virtual void Get(const std::string& key,
//                       const Callback& data_ready) const {
//        bool success = ...
//        std::string* data = new ...
//        data_ready(success, key, data);
//      }
//    };
class Source {
 public:
  typedef i18n::addressinput::Callback<const std::string&,
                                       std::string*> Callback;

  virtual ~Source() {}

  // Gets metadata for |key| and invokes the |data_ready| callback.
  virtual void Get(const std::string& key,
                   const Callback& data_ready) const = 0;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_SOURCE_H_
