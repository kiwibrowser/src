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
//
// A mock implementation of the Source interface to be used in tests.

#ifndef I18N_ADDRESSINPUT_TEST_MOCK_SOURCE_H_
#define I18N_ADDRESSINPUT_TEST_MOCK_SOURCE_H_

#include <libaddressinput/source.h>

#include <map>
#include <string>

namespace i18n {
namespace addressinput {

// Gets address metadata from a key-value map. Sample usage:
//    class MyClass {
//     public:
//      MyClass(const MyClass&) = delete;
//      MyClass& operator=(const MyClass&) = delete;
//
//      MyClass() : source_(),
//                  data_ready_(BuildCallback(this, &MyClass::OnDataReady)) {
//        source_.data_.insert(
//            std::make_pair("data/XA", "{\"id\":\"data/XA\"}"));
//      }
//
//      ~MyClass() {}
//
//      void GetData(const std::string& key) {
//        source_.Get(key, *data_ready_);
//      }
//
//     private:
//      void OnDataReady(bool success,
//                       const std::string& key,
//                       std::string* data) {
//        ...
//        delete data;
//      }
//
//      MockSource source_;
//      const std::unique_ptr<const Source::Callback> data_ready_;
//    };
class MockSource : public Source {
 public:
  MockSource(const MockSource&) = delete;
  MockSource& operator=(const MockSource&) = delete;

  MockSource();
  ~MockSource() override;

  // Source implementation.
  void Get(const std::string& key, const Callback& data_ready) const override;

  std::map<std::string, std::string> data_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_TEST_MOCK_SOURCE_H_
