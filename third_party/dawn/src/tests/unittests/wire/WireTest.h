// Copyright 2019 The Dawn Authors
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

#include "gtest/gtest.h"
#include "mock/mock_dawn.h"

#include <memory>

// Definition of a "Lambda predicate matcher" for GMock to allow checking deep structures
// are passed correctly by the wire.

// Helper templates to extract the argument type of a lambda.
template <typename T>
struct MatcherMethodArgument;

template <typename Lambda, typename Arg>
struct MatcherMethodArgument<bool (Lambda::*)(Arg) const> {
    using Type = Arg;
};

template <typename Lambda>
using MatcherLambdaArgument = typename MatcherMethodArgument<decltype(&Lambda::operator())>::Type;

// The matcher itself, unfortunately it isn't able to return detailed information like other
// matchers do.
template <typename Lambda, typename Arg>
class LambdaMatcherImpl : public testing::MatcherInterface<Arg> {
  public:
    explicit LambdaMatcherImpl(Lambda lambda) : mLambda(lambda) {
    }

    void DescribeTo(std::ostream* os) const override {
        *os << "with a custom matcher";
    }

    bool MatchAndExplain(Arg value, testing::MatchResultListener* listener) const override {
        if (!mLambda(value)) {
            *listener << "which doesn't satisfy the custom predicate";
            return false;
        }
        return true;
    }

  private:
    Lambda mLambda;
};

// Use the MatchesLambda as follows:
//
//   EXPECT_CALL(foo, Bar(MatchesLambda([](ArgType arg) -> bool {
//       return CheckPredicateOnArg(arg);
//   })));
template <typename Lambda>
inline testing::Matcher<MatcherLambdaArgument<Lambda>> MatchesLambda(Lambda lambda) {
    return MakeMatcher(new LambdaMatcherImpl<Lambda, MatcherLambdaArgument<Lambda>>(lambda));
}

namespace dawn_wire {
    class WireClient;
    class WireServer;
}  // namespace dawn_wire

namespace utils {
    class TerribleCommandBuffer;
}

class WireTest : public testing::Test {
  protected:
    WireTest();
    ~WireTest() override;

    void SetUp() override;
    void TearDown() override;
    void FlushClient();
    void FlushServer();

    testing::StrictMock<MockProcTable> api;
    DawnDevice apiDevice;
    DawnDevice device;

    dawn_wire::WireServer* GetWireServer();
    dawn_wire::WireClient* GetWireClient();

    void DeleteServer();

  private:
    void SetupIgnoredCallExpectations();

    std::unique_ptr<dawn_wire::WireServer> mWireServer;
    std::unique_ptr<dawn_wire::WireClient> mWireClient;
    std::unique_ptr<utils::TerribleCommandBuffer> mS2cBuf;
    std::unique_ptr<utils::TerribleCommandBuffer> mC2sBuf;
};
