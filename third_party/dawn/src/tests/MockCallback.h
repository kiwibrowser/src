// Copyright 2020 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gmock/gmock.h>

#include "common/Assert.h"

#include <memory>
#include <set>

namespace testing {

    template <typename F>
    class MockCallback;

    // Helper class for mocking callbacks used for Dawn callbacks with |void* userdata|
    // as the last callback argument.
    //
    // Example Usage:
    //   MockCallback<WGPUDeviceLostCallback> mock;
    //
    //   void* foo = XYZ; // this is the callback userdata
    //
    //   wgpuDeviceSetDeviceLostCallback(device, mock.Callback(), mock.MakeUserdata(foo));
    //   EXPECT_CALL(mock, Call(_, foo));
    template <typename R, typename... Args>
    class MockCallback<R (*)(Args...)> : public ::testing::MockFunction<R(Args...)> {
        using CallbackType = R (*)(Args...);

      public:
        // Helper function makes it easier to get the callback using |foo.Callback()|
        // unstead of MockCallback<CallbackType>::Callback.
        static CallbackType Callback() {
            return CallUnboundCallback;
        }

        void* MakeUserdata(void* userdata) {
            auto mockAndUserdata =
                std::unique_ptr<MockAndUserdata>(new MockAndUserdata{this, userdata});

            // Add the userdata to a set of userdata for this mock. We never
            // remove from this set even if a callback should only be called once so that
            // repeated calls to the callback still forward the userdata correctly.
            // Userdata will be destroyed when the mock is destroyed.
            auto it = mUserdatas.insert(std::move(mockAndUserdata));
            ASSERT(it.second);
            return it.first->get();
        }

      private:
        struct MockAndUserdata {
            MockCallback* mock;
            void* userdata;
        };

        static R CallUnboundCallback(Args... args) {
            std::tuple<Args...> tuple = std::make_tuple(args...);

            constexpr size_t ArgC = sizeof...(Args);
            static_assert(ArgC >= 1, "Mock callback requires at least one argument (the userdata)");

            // Get the userdata. It should be the last argument.
            auto userdata = std::get<ArgC - 1>(tuple);
            static_assert(std::is_same<decltype(userdata), void*>::value,
                          "Last callback argument must be void* userdata");

            // Extract the mock.
            ASSERT(userdata != nullptr);
            auto* mockAndUserdata = reinterpret_cast<MockAndUserdata*>(userdata);
            MockCallback* mock = mockAndUserdata->mock;
            ASSERT(mock != nullptr);

            // Replace the userdata
            std::get<ArgC - 1>(tuple) = mockAndUserdata->userdata;

            // Forward the callback to the mock.
            return mock->CallImpl(std::make_index_sequence<ArgC>{}, std::move(tuple));
        }

        // This helper cannot be inlined because we dependent on the templated index sequence
        // to unpack the tuple arguments.
        template <size_t... Is>
        R CallImpl(const std::index_sequence<Is...>&, std::tuple<Args...> args) {
            return this->Call(std::get<Is>(args)...);
        }

        std::set<std::unique_ptr<MockAndUserdata>> mUserdatas;
    };

}  // namespace testing
