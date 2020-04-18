// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/tests/pickled_types_blink.h"
#include "mojo/public/cpp/bindings/tests/pickled_types_chromium.h"
#include "mojo/public/cpp/bindings/tests/variant_test_util.h"
#include "mojo/public/interfaces/bindings/tests/test_native_types.mojom-blink.h"
#include "mojo/public/interfaces/bindings/tests/test_native_types.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

template <typename T>
void DoExpectResult(int foo, int bar, const base::Closure& callback, T actual) {
  EXPECT_EQ(foo, actual.foo());
  EXPECT_EQ(bar, actual.bar());
  callback.Run();
}

template <typename T>
base::Callback<void(T)> ExpectResult(const T& t,
                                     const base::Closure& callback) {
  return base::Bind(&DoExpectResult<T>, t.foo(), t.bar(), callback);
}

template <typename T>
void DoFail(const std::string& reason, T) {
  EXPECT_TRUE(false) << reason;
}

template <typename T>
base::Callback<void(T)> Fail(const std::string& reason) {
  return base::Bind(&DoFail<T>, reason);
}

template <typename T>
void DoExpectEnumResult(T expected, const base::Closure& callback, T actual) {
  EXPECT_EQ(expected, actual);
  callback.Run();
}

template <typename T>
base::Callback<void(T)> ExpectEnumResult(T t, const base::Closure& callback) {
  return base::Bind(&DoExpectEnumResult<T>, t, callback);
}

template <typename T>
void DoEnumFail(const std::string& reason, T) {
  EXPECT_TRUE(false) << reason;
}

template <typename T>
base::Callback<void(T)> EnumFail(const std::string& reason) {
  return base::Bind(&DoEnumFail<T>, reason);
}

template <typename T>
void ExpectError(InterfacePtr<T>* proxy, const base::Closure& callback) {
  proxy->set_connection_error_handler(callback);
}

template <typename Func, typename Arg>
void RunSimpleLambda(Func func, Arg arg) { func(std::move(arg)); }

template <typename Arg, typename Func>
base::Callback<void(Arg)> BindSimpleLambda(Func func) {
  return base::Bind(&RunSimpleLambda<Func, Arg>, func);
}

// This implements the generated Chromium variant of PicklePasser.
class ChromiumPicklePasserImpl : public PicklePasser {
 public:
  ChromiumPicklePasserImpl() {}

  // mojo::test::PicklePasser:
  void PassPickledStruct(PickledStructChromium pickle,
                         const PassPickledStructCallback& callback) override {
    callback.Run(std::move(pickle));
  }

  void PassPickledEnum(PickledEnumChromium pickle,
                       const PassPickledEnumCallback& callback) override {
    callback.Run(pickle);
  }

  void PassPickleContainer(
      PickleContainerPtr container,
      const PassPickleContainerCallback& callback) override {
    callback.Run(std::move(container));
  }

  void PassPickles(std::vector<PickledStructChromium> pickles,
                   const PassPicklesCallback& callback) override {
    callback.Run(std::move(pickles));
  }

  void PassPickleArrays(
      std::vector<std::vector<PickledStructChromium>> pickle_arrays,
      const PassPickleArraysCallback& callback) override {
    callback.Run(std::move(pickle_arrays));
  }
};

// This implements the generated Blink variant of PicklePasser.
class BlinkPicklePasserImpl : public blink::PicklePasser {
 public:
  BlinkPicklePasserImpl() {}

  // mojo::test::blink::PicklePasser:
  void PassPickledStruct(PickledStructBlink pickle,
                         const PassPickledStructCallback& callback) override {
    callback.Run(std::move(pickle));
  }

  void PassPickledEnum(PickledEnumBlink pickle,
                       const PassPickledEnumCallback& callback) override {
    callback.Run(pickle);
  }

  void PassPickleContainer(
      blink::PickleContainerPtr container,
      const PassPickleContainerCallback& callback) override {
    callback.Run(std::move(container));
  }

  void PassPickles(WTF::Vector<PickledStructBlink> pickles,
                   const PassPicklesCallback& callback) override {
    callback.Run(std::move(pickles));
  }

  void PassPickleArrays(
      WTF::Vector<WTF::Vector<PickledStructBlink>> pickle_arrays,
      const PassPickleArraysCallback& callback) override {
    callback.Run(std::move(pickle_arrays));
  }
};

// A test which runs both Chromium and Blink implementations of the
// PicklePasser service.
class PickleTest : public testing::Test {
 public:
  PickleTest() {}

  template <typename ProxyType = PicklePasser>
  InterfacePtr<ProxyType> ConnectToChromiumService() {
    InterfacePtr<ProxyType> proxy;
    chromium_bindings_.AddBinding(
        &chromium_service_,
        ConvertInterfaceRequest<PicklePasser>(mojo::MakeRequest(&proxy)));
    return proxy;
  }

  template <typename ProxyType = blink::PicklePasser>
  InterfacePtr<ProxyType> ConnectToBlinkService() {
    InterfacePtr<ProxyType> proxy;
    blink_bindings_.AddBinding(&blink_service_,
                               ConvertInterfaceRequest<blink::PicklePasser>(
                                   mojo::MakeRequest(&proxy)));
    return proxy;
  }

 protected:
  static void ForceMessageSerialization(bool forced) {
    // Force messages to be serialized in this test since it intentionally
    // exercises StructTraits logic.
    Connector::OverrideDefaultSerializationBehaviorForTesting(
        forced ? Connector::OutgoingSerializationMode::kEager
               : Connector::OutgoingSerializationMode::kLazy,
        Connector::IncomingSerializationMode::kDispatchAsIs);
  }

  class ScopedForceMessageSerialization {
   public:
    ScopedForceMessageSerialization() { ForceMessageSerialization(true); }
    ~ScopedForceMessageSerialization() { ForceMessageSerialization(false); }

   private:
    DISALLOW_COPY_AND_ASSIGN(ScopedForceMessageSerialization);
  };

 private:
  base::MessageLoop loop_;
  ChromiumPicklePasserImpl chromium_service_;
  BindingSet<PicklePasser> chromium_bindings_;
  BlinkPicklePasserImpl blink_service_;
  BindingSet<blink::PicklePasser> blink_bindings_;
};

}  // namespace

TEST_F(PickleTest, ChromiumProxyToChromiumService) {
  auto chromium_proxy = ConnectToChromiumService();
  {
    base::RunLoop loop;
    chromium_proxy->PassPickledStruct(
        PickledStructChromium(1, 2),
        ExpectResult(PickledStructChromium(1, 2), loop.QuitClosure()));
    loop.Run();
  }
  {
    base::RunLoop loop;
    chromium_proxy->PassPickledStruct(
        PickledStructChromium(4, 5),
        ExpectResult(PickledStructChromium(4, 5), loop.QuitClosure()));
    loop.Run();
  }

  {
    base::RunLoop loop;
    chromium_proxy->PassPickledEnum(
        PickledEnumChromium::VALUE_1,
        ExpectEnumResult(PickledEnumChromium::VALUE_1, loop.QuitClosure()));
    loop.Run();
  }
}

TEST_F(PickleTest, ChromiumProxyToBlinkService) {
  auto chromium_proxy = ConnectToBlinkService<PicklePasser>();
  {
    base::RunLoop loop;
    chromium_proxy->PassPickledStruct(
        PickledStructChromium(1, 2),
        ExpectResult(PickledStructChromium(1, 2), loop.QuitClosure()));
    loop.Run();
  }
  {
    base::RunLoop loop;
    chromium_proxy->PassPickledStruct(
        PickledStructChromium(4, 5),
        ExpectResult(PickledStructChromium(4, 5), loop.QuitClosure()));
    loop.Run();
  }
  // The Blink service should drop our connection because the
  // PickledStructBlink ParamTraits deserializer rejects negative values.
  {
    base::RunLoop loop;
    chromium_proxy->PassPickledStruct(
        PickledStructChromium(-1, -1),
        Fail<PickledStructChromium>("Blink service should reject this."));
    ExpectError(&chromium_proxy, loop.QuitClosure());
    loop.Run();
  }

  chromium_proxy = ConnectToBlinkService<PicklePasser>();
  {
    base::RunLoop loop;
    chromium_proxy->PassPickledEnum(
        PickledEnumChromium::VALUE_0,
        ExpectEnumResult(PickledEnumChromium::VALUE_0, loop.QuitClosure()));
    loop.Run();
  }

  // The Blink service should drop our connection because the
  // PickledEnumBlink ParamTraits deserializer rejects this value.
  {
    base::RunLoop loop;
    chromium_proxy->PassPickledEnum(
        PickledEnumChromium::VALUE_2,
        EnumFail<PickledEnumChromium>("Blink service should reject this."));
    ExpectError(&chromium_proxy, loop.QuitClosure());
    loop.Run();
  }
}

TEST_F(PickleTest, BlinkProxyToBlinkService) {
  auto blink_proxy = ConnectToBlinkService();
  {
    base::RunLoop loop;
    blink_proxy->PassPickledStruct(
        PickledStructBlink(1, 1),
        ExpectResult(PickledStructBlink(1, 1), loop.QuitClosure()));
    loop.Run();
  }

  {
    base::RunLoop loop;
    blink_proxy->PassPickledEnum(
        PickledEnumBlink::VALUE_0,
        ExpectEnumResult(PickledEnumBlink::VALUE_0, loop.QuitClosure()));
    loop.Run();
  }
}

TEST_F(PickleTest, BlinkProxyToChromiumService) {
  auto blink_proxy = ConnectToChromiumService<blink::PicklePasser>();
  {
    base::RunLoop loop;
    blink_proxy->PassPickledStruct(
        PickledStructBlink(1, 1),
        ExpectResult(PickledStructBlink(1, 1), loop.QuitClosure()));
    loop.Run();
  }

  {
    base::RunLoop loop;
    blink_proxy->PassPickledEnum(
        PickledEnumBlink::VALUE_1,
        ExpectEnumResult(PickledEnumBlink::VALUE_1, loop.QuitClosure()));
    loop.Run();
  }
}

TEST_F(PickleTest, PickleArray) {
  ScopedForceMessageSerialization force_serialization;
  auto proxy = ConnectToChromiumService();
  auto pickles = std::vector<PickledStructChromium>(2);
  pickles[0].set_foo(1);
  pickles[0].set_bar(2);
  pickles[0].set_baz(100);
  pickles[1].set_foo(3);
  pickles[1].set_bar(4);
  pickles[1].set_baz(100);
  {
    base::RunLoop run_loop;
    // Verify that the array of pickled structs can be serialized and
    // deserialized intact. This ensures that the ParamTraits are actually used
    // rather than doing a byte-for-byte copy of the element data, beacuse the
    // |baz| field should never be serialized.
    proxy->PassPickles(std::move(pickles),
                       BindSimpleLambda<std::vector<PickledStructChromium>>(
                           [&](std::vector<PickledStructChromium> passed) {
                             ASSERT_EQ(2u, passed.size());
                             EXPECT_EQ(1, passed[0].foo());
                             EXPECT_EQ(2, passed[0].bar());
                             EXPECT_EQ(0, passed[0].baz());
                             EXPECT_EQ(3, passed[1].foo());
                             EXPECT_EQ(4, passed[1].bar());
                             EXPECT_EQ(0, passed[1].baz());
                             run_loop.Quit();
                           }));
    run_loop.Run();
  }
}

TEST_F(PickleTest, PickleArrayArray) {
  ScopedForceMessageSerialization force_serialization;
  auto proxy = ConnectToChromiumService();
  auto pickle_arrays = std::vector<std::vector<PickledStructChromium>>(2);
  for (size_t i = 0; i < 2; ++i)
    pickle_arrays[i] = std::vector<PickledStructChromium>(2);

  pickle_arrays[0][0].set_foo(1);
  pickle_arrays[0][0].set_bar(2);
  pickle_arrays[0][0].set_baz(100);
  pickle_arrays[0][1].set_foo(3);
  pickle_arrays[0][1].set_bar(4);
  pickle_arrays[0][1].set_baz(100);
  pickle_arrays[1][0].set_foo(5);
  pickle_arrays[1][0].set_bar(6);
  pickle_arrays[1][0].set_baz(100);
  pickle_arrays[1][1].set_foo(7);
  pickle_arrays[1][1].set_bar(8);
  pickle_arrays[1][1].set_baz(100);
  {
    base::RunLoop run_loop;
    // Verify that the array-of-arrays serializes and deserializes properly.
    proxy->PassPickleArrays(
        std::move(pickle_arrays),
        BindSimpleLambda<std::vector<std::vector<PickledStructChromium>>>(
            [&](std::vector<std::vector<PickledStructChromium>> passed) {
              ASSERT_EQ(2u, passed.size());
              ASSERT_EQ(2u, passed[0].size());
              ASSERT_EQ(2u, passed[1].size());
              EXPECT_EQ(1, passed[0][0].foo());
              EXPECT_EQ(2, passed[0][0].bar());
              EXPECT_EQ(0, passed[0][0].baz());
              EXPECT_EQ(3, passed[0][1].foo());
              EXPECT_EQ(4, passed[0][1].bar());
              EXPECT_EQ(0, passed[0][1].baz());
              EXPECT_EQ(5, passed[1][0].foo());
              EXPECT_EQ(6, passed[1][0].bar());
              EXPECT_EQ(0, passed[1][0].baz());
              EXPECT_EQ(7, passed[1][1].foo());
              EXPECT_EQ(8, passed[1][1].bar());
              EXPECT_EQ(0, passed[1][1].baz());
              run_loop.Quit();
            }));
    run_loop.Run();
  }
}

TEST_F(PickleTest, PickleContainer) {
  ScopedForceMessageSerialization force_serialization;
  auto proxy = ConnectToChromiumService();
  PickleContainerPtr pickle_container = PickleContainer::New();
  pickle_container->f_struct.set_foo(42);
  pickle_container->f_struct.set_bar(43);
  pickle_container->f_struct.set_baz(44);
  pickle_container->f_enum = PickledEnumChromium::VALUE_1;
  EXPECT_TRUE(pickle_container.Equals(pickle_container));
  EXPECT_FALSE(pickle_container.Equals(PickleContainer::New()));
  {
    base::RunLoop run_loop;
    proxy->PassPickleContainer(std::move(pickle_container),
                               BindSimpleLambda<PickleContainerPtr>(
                                   [&](PickleContainerPtr passed) {
                                     ASSERT_FALSE(passed.is_null());
                                     EXPECT_EQ(42, passed->f_struct.foo());
                                     EXPECT_EQ(43, passed->f_struct.bar());
                                     EXPECT_EQ(0, passed->f_struct.baz());
                                     EXPECT_EQ(PickledEnumChromium::VALUE_1,
                                               passed->f_enum);
                                     run_loop.Quit();
                                   }));
    run_loop.Run();
  }
}

}  // namespace test
}  // namespace mojo
