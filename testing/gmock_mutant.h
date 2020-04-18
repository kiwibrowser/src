// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_GMOCK_MUTANT_H_
#define TESTING_GMOCK_MUTANT_H_

// The intention of this file is to make possible using GMock actions in
// all of its syntactic beauty.
//
//
// Sample usage with gMock:
//
// struct Mock : public ObjectDelegate {
//   MOCK_METHOD2(string, OnRequest(int n, const string& request));
//   MOCK_METHOD1(void, OnQuit(int exit_code));
//   MOCK_METHOD2(void, LogMessage(int level, const string& message));
//
//   string HandleFlowers(const string& reply, int n, const string& request) {
//     string result = SStringPrintf("In request of %d %s ", n, request);
//     for (int i = 0; i < n; ++i) result.append(reply)
//     return result;
//   }
//
//   void DoLogMessage(int level, const string& message) {
//   }
//
//   void QuitMessageLoop(int seconds) {
//     base::MessageLoop* loop = base::MessageLoop::current();
//     loop->PostDelayedTask(FROM_HERE,
//                           base::MessageLoop::QuitWhenIdleClosure(),
//                           1000 * seconds);
//   }
// };
//
// Mock mock;
// // Will invoke mock.HandleFlowers("orchids", n, request)
// // "orchids" is a pre-bound argument, and <n> and <request> are call-time
// // arguments - they are not known until the OnRequest mock is invoked.
// EXPECT_CALL(mock, OnRequest(Ge(5), base::StartsWith("flower"))
//   .Times(1)
//   .WillOnce(Invoke(CreateFunctor(
//       &Mock::HandleFlowers, base::Unretained(&mock), string("orchids"))));
//
//
// // No pre-bound arguments, two call-time arguments passed
// // directly to DoLogMessage
// EXPECT_CALL(mock, OnLogMessage(_, _))
//   .Times(AnyNumber())
//   .WillAlways(Invoke(CreateFunctor(
//       &Mock::DoLogMessage, base::Unretained(&mock))));
//
//
// // In this case we have a single pre-bound argument - 3. We ignore
// // all of the arguments of OnQuit.
// EXCEPT_CALL(mock, OnQuit(_))
//   .Times(1)
//   .WillOnce(InvokeWithoutArgs(CreateFunctor(
//       &Mock::QuitMessageLoop, base::Unretained(&mock), 3)));
//
// MessageLoop loop;
// loop.Run();
//
//
//  // Here is another example of how we can set an action that invokes
//  // method of an object that is not yet created.
// struct Mock : public ObjectDelegate {
//   MOCK_METHOD1(void, DemiurgeCreated(Demiurge*));
//   MOCK_METHOD2(void, OnRequest(int count, const string&));
//
//   void StoreDemiurge(Demiurge* w) {
//     demiurge_ = w;
//   }
//
//   Demiurge* demiurge;
// }
//
// EXPECT_CALL(mock, DemiurgeCreated(_)).Times(1)
//    .WillOnce(Invoke(CreateFunctor(
//        &Mock::StoreDemiurge, base::Unretained(&mock))));
//
// EXPECT_CALL(mock, OnRequest(_, StrEq("Moby Dick")))
//    .Times(AnyNumber())
//    .WillAlways(WithArgs<0>(Invoke(CreateFunctor(
//        &Demiurge::DecreaseMonsters, base::Unretained(&mock->demiurge_)))));
//

#include "base/bind.h"

namespace testing {

template <typename Signature>
class CallbackToFunctorHelper;

template <typename R, typename... Args>
class CallbackToFunctorHelper<R(Args...)> {
 public:
  explicit CallbackToFunctorHelper(const base::Callback<R(Args...)>& cb)
      : cb_(cb) {}

  template <typename... RunArgs>
  R operator()(RunArgs&&... args) {
    return cb_.Run(std::forward<RunArgs>(args)...);
  }

 private:
  base::Callback<R(Args...)> cb_;
};

template <typename Signature>
CallbackToFunctorHelper<Signature>
CallbackToFunctor(const base::Callback<Signature>& cb) {
  return CallbackToFunctorHelper<Signature>(cb);
}

template <typename Functor>
CallbackToFunctorHelper<typename Functor::RunType> CreateFunctor(
    Functor functor) {
  return CallbackToFunctor(functor);
}

template <typename Functor, typename... BoundArgs>
CallbackToFunctorHelper<base::MakeUnboundRunType<Functor, BoundArgs...>>
CreateFunctor(Functor functor, const BoundArgs&... args) {
  return CallbackToFunctor(base::Bind(functor, args...));
}

}  // namespace testing

#endif  // TESTING_GMOCK_MUTANT_H_
