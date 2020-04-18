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
// An object to store a pointer to a method in an object with the following
// signature:
//
//    void Observer::ObserveEvent(bool success, Key key, Data data);

#ifndef I18N_ADDRESSINPUT_CALLBACK_H_
#define I18N_ADDRESSINPUT_CALLBACK_H_

#include <cassert>
#include <cstddef>

namespace i18n {
namespace addressinput {

// Stores a pointer to a method in an object. Sample usage:
//    class MyClass {
//     public:
//      typedef Callback<const MyType&, const MyDataType&> MyCallback;
//
//      void GetDataAsynchronously() {
//        std::unique_ptr<MyCallback> callback(BuildCallback(
//            this, &MyClass::OnDataReady));
//        bool success = ...
//        MyKeyType key = ...
//        MyDataType data = ...
//        (*callback)(success, key, data);
//      }
//
//      void OnDataReady(bool success,
//                       const MyKeyType& key,
//                       const MyDataType& data) {
//        ...
//      }
//    };
template <typename Key, typename Data>
class Callback {
 public:
  virtual ~Callback() {}
  virtual void operator()(bool success, Key key, Data data) const = 0;
};

namespace {

template <typename Observer, typename Key, typename Data>
class CallbackImpl : public Callback<Key, Data> {
 public:
  typedef void (Observer::*ObserveEvent)(bool, Key, Data);

  CallbackImpl(Observer* observer, ObserveEvent observe_event)
      : observer_(observer),
        observe_event_(observe_event) {
    assert(observer_ != nullptr);
    assert(observe_event_ != nullptr);
  }

  ~CallbackImpl() override {}

  void operator()(bool success, Key key, Data data) const override {
    (observer_->*observe_event_)(success, key, data);
  }

 private:
  Observer* observer_;
  ObserveEvent observe_event_;
};

}  // namespace

// Returns a callback to |observer->observe_event| method. The caller owns the
// result.
template <typename Observer, typename Key, typename Data>
Callback<Key, Data>* BuildCallback(
    Observer* observer,
    void (Observer::*observe_event)(bool, Key, Data)) {
  return new CallbackImpl<Observer, Key, Data>(observer, observe_event);
}

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_CALLBACK_H_
