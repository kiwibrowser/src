// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/tests/rect_blink.h"
#include "mojo/public/cpp/bindings/tests/rect_chromium.h"
#include "mojo/public/cpp/bindings/tests/struct_with_traits_impl.h"
#include "mojo/public/cpp/bindings/tests/struct_with_traits_impl_traits.h"
#include "mojo/public/cpp/bindings/tests/variant_test_util.h"
#include "mojo/public/cpp/system/wait.h"
#include "mojo/public/interfaces/bindings/tests/struct_with_traits.mojom.h"
#include "mojo/public/interfaces/bindings/tests/test_native_types.mojom-blink.h"
#include "mojo/public/interfaces/bindings/tests/test_native_types.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

template <typename T>
void DoExpectResult(const T& expected,
                    const base::Closure& callback,
                    const T& actual) {
  EXPECT_EQ(expected.x(), actual.x());
  EXPECT_EQ(expected.y(), actual.y());
  EXPECT_EQ(expected.width(), actual.width());
  EXPECT_EQ(expected.height(), actual.height());
  callback.Run();
}

template <typename T>
base::Callback<void(const T&)> ExpectResult(const T& r,
                                            const base::Closure& callback) {
  return base::Bind(&DoExpectResult<T>, r, callback);
}

template <typename T>
void DoFail(const std::string& reason, const T&) {
  EXPECT_TRUE(false) << reason;
}

template <typename T>
base::Callback<void(const T&)> Fail(const std::string& reason) {
  return base::Bind(&DoFail<T>, reason);
}

template <typename T>
void ExpectError(InterfacePtr<T> *proxy, const base::Closure& callback) {
  proxy->set_connection_error_handler(callback);
}

// This implements the generated Chromium variant of RectService.
class ChromiumRectServiceImpl : public RectService {
 public:
  ChromiumRectServiceImpl() {}

  // mojo::test::RectService:
  void AddRect(const RectChromium& r) override {
    if (r.GetArea() > largest_rect_.GetArea())
      largest_rect_ = r;
  }

  void GetLargestRect(const GetLargestRectCallback& callback) override {
    callback.Run(largest_rect_);
  }

  void PassSharedRect(const SharedRect& r,
                      const PassSharedRectCallback& callback) override {
    callback.Run(r);
  }

 private:
  RectChromium largest_rect_;
};

// This implements the generated Blink variant of RectService.
class BlinkRectServiceImpl : public blink::RectService {
 public:
  BlinkRectServiceImpl() {}

  // mojo::test::blink::RectService:
  void AddRect(const RectBlink& r) override {
    if (r.computeArea() > largest_rect_.computeArea()) {
      largest_rect_.setX(r.x());
      largest_rect_.setY(r.y());
      largest_rect_.setWidth(r.width());
      largest_rect_.setHeight(r.height());
    }
  }

  void GetLargestRect(const GetLargestRectCallback& callback) override {
    callback.Run(largest_rect_);
  }

  void PassSharedRect(const SharedRect& r,
                      const PassSharedRectCallback& callback) override {
    callback.Run(r);
  }

 private:
  RectBlink largest_rect_;
};

// A test which runs both Chromium and Blink implementations of a RectService.
class StructTraitsTest : public testing::Test,
                         public TraitsTestService {
 public:
  StructTraitsTest() {}

 protected:
  void BindToChromiumService(RectServiceRequest request) {
    chromium_bindings_.AddBinding(&chromium_service_, std::move(request));
  }
  void BindToChromiumService(blink::RectServiceRequest request) {
    chromium_bindings_.AddBinding(
        &chromium_service_,
        ConvertInterfaceRequest<RectService>(std::move(request)));
  }

  void BindToBlinkService(blink::RectServiceRequest request) {
    blink_bindings_.AddBinding(&blink_service_, std::move(request));
  }
  void BindToBlinkService(RectServiceRequest request) {
    blink_bindings_.AddBinding(
        &blink_service_,
        ConvertInterfaceRequest<blink::RectService>(std::move(request)));
  }

  TraitsTestServicePtr GetTraitsTestProxy() {
    TraitsTestServicePtr proxy;
    traits_test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // TraitsTestService:
  void EchoStructWithTraits(
      const StructWithTraitsImpl& s,
      const EchoStructWithTraitsCallback& callback) override {
    callback.Run(s);
  }

  void EchoTrivialStructWithTraits(
      TrivialStructWithTraitsImpl s,
      const EchoTrivialStructWithTraitsCallback& callback) override {
    callback.Run(s);
  }

  void EchoMoveOnlyStructWithTraits(
      MoveOnlyStructWithTraitsImpl s,
      const EchoMoveOnlyStructWithTraitsCallback& callback) override {
    callback.Run(std::move(s));
  }

  void EchoNullableMoveOnlyStructWithTraits(
      base::Optional<MoveOnlyStructWithTraitsImpl> s,
      const EchoNullableMoveOnlyStructWithTraitsCallback& callback) override {
    callback.Run(std::move(s));
  }

  void EchoEnumWithTraits(EnumWithTraitsImpl e,
                          const EchoEnumWithTraitsCallback& callback) override {
    callback.Run(e);
  }

  void EchoStructWithTraitsForUniquePtr(
      std::unique_ptr<int> e,
      const EchoStructWithTraitsForUniquePtrCallback& callback) override {
    callback.Run(std::move(e));
  }

  void EchoNullableStructWithTraitsForUniquePtr(
      std::unique_ptr<int> e,
      const EchoNullableStructWithTraitsForUniquePtrCallback& callback)
      override {
    callback.Run(std::move(e));
  }

  void EchoUnionWithTraits(
      std::unique_ptr<test::UnionWithTraitsBase> u,
      const EchoUnionWithTraitsCallback& callback) override {
    callback.Run(std::move(u));
  }

  base::MessageLoop loop_;

  ChromiumRectServiceImpl chromium_service_;
  BindingSet<RectService> chromium_bindings_;

  BlinkRectServiceImpl blink_service_;
  BindingSet<blink::RectService> blink_bindings_;

  BindingSet<TraitsTestService> traits_test_bindings_;
};

}  // namespace

TEST_F(StructTraitsTest, ChromiumProxyToChromiumService) {
  RectServicePtr chromium_proxy;
  BindToChromiumService(MakeRequest(&chromium_proxy));
  {
    base::RunLoop loop;
    chromium_proxy->AddRect(RectChromium(1, 1, 4, 5));
    chromium_proxy->AddRect(RectChromium(-1, -1, 2, 2));
    chromium_proxy->GetLargestRect(
        ExpectResult(RectChromium(1, 1, 4, 5), loop.QuitClosure()));
    loop.Run();
  }
  {
    base::RunLoop loop;
    chromium_proxy->PassSharedRect(
        {1, 2, 3, 4},
        ExpectResult(SharedRect({1, 2, 3, 4}), loop.QuitClosure()));
    loop.Run();
  }
}

TEST_F(StructTraitsTest, ChromiumToBlinkService) {
  RectServicePtr chromium_proxy;
  BindToBlinkService(MakeRequest(&chromium_proxy));
  {
    base::RunLoop loop;
    chromium_proxy->AddRect(RectChromium(1, 1, 4, 5));
    chromium_proxy->AddRect(RectChromium(2, 2, 5, 5));
    chromium_proxy->GetLargestRect(
        ExpectResult(RectChromium(2, 2, 5, 5), loop.QuitClosure()));
    loop.Run();
  }
  {
    base::RunLoop loop;
    chromium_proxy->PassSharedRect(
        {1, 2, 3, 4},
        ExpectResult(SharedRect({1, 2, 3, 4}), loop.QuitClosure()));
    loop.Run();
  }
  // The Blink service should drop our connection because RectBlink's
  // deserializer rejects negative origins.
  {
    base::RunLoop loop;
    ExpectError(&chromium_proxy, loop.QuitClosure());
    chromium_proxy->AddRect(RectChromium(-1, -1, 2, 2));
    chromium_proxy->GetLargestRect(
        Fail<RectChromium>("The pipe should have been closed."));
    loop.Run();
  }
}

TEST_F(StructTraitsTest, BlinkProxyToBlinkService) {
  blink::RectServicePtr blink_proxy;
  BindToBlinkService(MakeRequest(&blink_proxy));
  {
    base::RunLoop loop;
    blink_proxy->AddRect(RectBlink(1, 1, 4, 5));
    blink_proxy->AddRect(RectBlink(10, 10, 20, 20));
    blink_proxy->GetLargestRect(
        ExpectResult(RectBlink(10, 10, 20, 20), loop.QuitClosure()));
    loop.Run();
  }
  {
    base::RunLoop loop;
    blink_proxy->PassSharedRect(
        {4, 3, 2, 1},
        ExpectResult(SharedRect({4, 3, 2, 1}), loop.QuitClosure()));
    loop.Run();
  }
}

TEST_F(StructTraitsTest, BlinkProxyToChromiumService) {
  blink::RectServicePtr blink_proxy;
  BindToChromiumService(MakeRequest(&blink_proxy));
  {
    base::RunLoop loop;
    blink_proxy->AddRect(RectBlink(1, 1, 4, 5));
    blink_proxy->AddRect(RectBlink(10, 10, 2, 2));
    blink_proxy->GetLargestRect(
        ExpectResult(RectBlink(1, 1, 4, 5), loop.QuitClosure()));
    loop.Run();
  }
  {
    base::RunLoop loop;
    blink_proxy->PassSharedRect(
        {4, 3, 2, 1},
        ExpectResult(SharedRect({4, 3, 2, 1}), loop.QuitClosure()));
    loop.Run();
  }
}

void ExpectStructWithTraits(const StructWithTraitsImpl& expected,
                            const base::Closure& closure,
                            const StructWithTraitsImpl& passed) {
  EXPECT_EQ(expected.get_enum(), passed.get_enum());
  EXPECT_EQ(expected.get_bool(), passed.get_bool());
  EXPECT_EQ(expected.get_uint32(), passed.get_uint32());
  EXPECT_EQ(expected.get_uint64(), passed.get_uint64());
  EXPECT_EQ(expected.get_string(), passed.get_string());
  EXPECT_EQ(expected.get_string_array(), passed.get_string_array());
  EXPECT_EQ(expected.get_struct(), passed.get_struct());
  EXPECT_EQ(expected.get_struct_array(), passed.get_struct_array());
  EXPECT_EQ(expected.get_struct_map(), passed.get_struct_map());
  closure.Run();
}

TEST_F(StructTraitsTest, EchoStructWithTraits) {
  StructWithTraitsImpl input;
  input.set_enum(EnumWithTraitsImpl::CUSTOM_VALUE_1);
  input.set_bool(true);
  input.set_uint32(7);
  input.set_uint64(42);
  input.set_string("hello world!");
  input.get_mutable_string_array().assign({"hello", "world!"});
  input.get_mutable_string_set().insert("hello");
  input.get_mutable_string_set().insert("world!");
  input.get_mutable_struct().value = 42;
  input.get_mutable_struct_array().resize(2);
  input.get_mutable_struct_array()[0].value = 1;
  input.get_mutable_struct_array()[1].value = 2;
  input.get_mutable_struct_map()["hello"] = NestedStructWithTraitsImpl(1024);
  input.get_mutable_struct_map()["world"] = NestedStructWithTraitsImpl(2048);

  base::RunLoop loop;
  TraitsTestServicePtr proxy = GetTraitsTestProxy();

  proxy->EchoStructWithTraits(
      input,
      base::Bind(&ExpectStructWithTraits, input, loop.QuitClosure()));
  loop.Run();
}

TEST_F(StructTraitsTest, CloneStructWithTraitsContainer) {
  StructWithTraitsContainerPtr container = StructWithTraitsContainer::New();
  container->f_struct.set_uint32(7);
  container->f_struct.set_uint64(42);
  StructWithTraitsContainerPtr cloned_container = container.Clone();
  EXPECT_EQ(7u, cloned_container->f_struct.get_uint32());
  EXPECT_EQ(42u, cloned_container->f_struct.get_uint64());
}

void ExpectTrivialStructWithTraits(TrivialStructWithTraitsImpl expected,
                                   const base::Closure& closure,
                                   TrivialStructWithTraitsImpl passed) {
  EXPECT_EQ(expected.value, passed.value);
  closure.Run();
}

TEST_F(StructTraitsTest, EchoTrivialStructWithTraits) {
  TrivialStructWithTraitsImpl input;
  input.value = 42;

  base::RunLoop loop;
  TraitsTestServicePtr proxy = GetTraitsTestProxy();

  proxy->EchoTrivialStructWithTraits(
      input,
      base::Bind(&ExpectTrivialStructWithTraits, input, loop.QuitClosure()));
  loop.Run();
}

void CaptureMessagePipe(ScopedMessagePipeHandle* storage,
                        const base::Closure& closure,
                        MoveOnlyStructWithTraitsImpl passed) {
  storage->reset(MessagePipeHandle(
      passed.get_mutable_handle().release().value()));
  closure.Run();
}

TEST_F(StructTraitsTest, EchoMoveOnlyStructWithTraits) {
  MessagePipe mp;
  MoveOnlyStructWithTraitsImpl input;
  input.get_mutable_handle().reset(mp.handle0.release());

  base::RunLoop loop;
  TraitsTestServicePtr proxy = GetTraitsTestProxy();

  ScopedMessagePipeHandle received;
  proxy->EchoMoveOnlyStructWithTraits(
      std::move(input),
      base::Bind(&CaptureMessagePipe, &received, loop.QuitClosure()));
  loop.Run();

  ASSERT_TRUE(received.is_valid());

  // Verify that the message pipe handle is correctly passed.
  const char kHello[] = "hello";
  const uint32_t kHelloSize = static_cast<uint32_t>(sizeof(kHello));
  EXPECT_EQ(MOJO_RESULT_OK,
            WriteMessageRaw(mp.handle1.get(), kHello, kHelloSize, nullptr, 0,
                            MOJO_WRITE_MESSAGE_FLAG_NONE));

  EXPECT_EQ(MOJO_RESULT_OK, Wait(received.get(), MOJO_HANDLE_SIGNAL_READABLE));

  std::vector<uint8_t> bytes;
  std::vector<ScopedHandle> handles;
  EXPECT_EQ(MOJO_RESULT_OK, ReadMessageRaw(received.get(), &bytes, &handles,
                                           MOJO_READ_MESSAGE_FLAG_NONE));
  EXPECT_EQ(kHelloSize, bytes.size());
  EXPECT_STREQ(kHello, reinterpret_cast<char*>(bytes.data()));
}

void CaptureNullableMoveOnlyStructWithTraitsImpl(
    base::Optional<MoveOnlyStructWithTraitsImpl>* storage,
    const base::Closure& closure,
    base::Optional<MoveOnlyStructWithTraitsImpl> passed) {
  *storage = std::move(passed);
  closure.Run();
}

TEST_F(StructTraitsTest, EchoNullableMoveOnlyStructWithTraits) {
  base::RunLoop loop;
  TraitsTestServicePtr proxy = GetTraitsTestProxy();

  base::Optional<MoveOnlyStructWithTraitsImpl> received;
  proxy->EchoNullableMoveOnlyStructWithTraits(
      base::nullopt, base::Bind(&CaptureNullableMoveOnlyStructWithTraitsImpl,
                                &received, loop.QuitClosure()));
  loop.Run();

  EXPECT_FALSE(received);
}

void ExpectEnumWithTraits(EnumWithTraitsImpl expected_value,
                          const base::Closure& closure,
                          EnumWithTraitsImpl value) {
  EXPECT_EQ(expected_value, value);
  closure.Run();
}

TEST_F(StructTraitsTest, EchoEnumWithTraits) {
  base::RunLoop loop;
  TraitsTestServicePtr proxy = GetTraitsTestProxy();

  proxy->EchoEnumWithTraits(
      EnumWithTraitsImpl::CUSTOM_VALUE_1,
      base::Bind(&ExpectEnumWithTraits, EnumWithTraitsImpl::CUSTOM_VALUE_1,
                 loop.QuitClosure()));
  loop.Run();
}

TEST_F(StructTraitsTest, SerializeStructWithTraits) {
  StructWithTraitsImpl input;
  input.set_enum(EnumWithTraitsImpl::CUSTOM_VALUE_1);
  input.set_bool(true);
  input.set_uint32(7);
  input.set_uint64(42);
  input.set_string("hello world!");
  input.get_mutable_string_array().assign({ "hello", "world!" });
  input.get_mutable_string_set().insert("hello");
  input.get_mutable_string_set().insert("world!");
  input.get_mutable_struct().value = 42;
  input.get_mutable_struct_array().resize(2);
  input.get_mutable_struct_array()[0].value = 1;
  input.get_mutable_struct_array()[1].value = 2;
  input.get_mutable_struct_map()["hello"] = NestedStructWithTraitsImpl(1024);
  input.get_mutable_struct_map()["world"] = NestedStructWithTraitsImpl(2048);

  auto data = StructWithTraits::Serialize(&input);
  StructWithTraitsImpl output;
  ASSERT_TRUE(StructWithTraits::Deserialize(std::move(data), &output));

  EXPECT_EQ(input.get_enum(), output.get_enum());
  EXPECT_EQ(input.get_bool(), output.get_bool());
  EXPECT_EQ(input.get_uint32(), output.get_uint32());
  EXPECT_EQ(input.get_uint64(), output.get_uint64());
  EXPECT_EQ(input.get_string(), output.get_string());
  EXPECT_EQ(input.get_string_array(), output.get_string_array());
  EXPECT_EQ(input.get_string_set(), output.get_string_set());
  EXPECT_EQ(input.get_struct(), output.get_struct());
  EXPECT_EQ(input.get_struct_array(), output.get_struct_array());
  EXPECT_EQ(input.get_struct_map(), output.get_struct_map());
}

void ExpectUniquePtr(std::unique_ptr<int> expected,
                     const base::Closure& closure,
                     std::unique_ptr<int> value) {
  ASSERT_EQ(!expected, !value);
  if (expected)
    EXPECT_EQ(*expected, *value);
  closure.Run();
}

TEST_F(StructTraitsTest, TypemapUniquePtr) {
  TraitsTestServicePtr proxy = GetTraitsTestProxy();

  {
    base::RunLoop loop;
    proxy->EchoStructWithTraitsForUniquePtr(
        std::make_unique<int>(12345),
        base::Bind(&ExpectUniquePtr, base::Passed(std::make_unique<int>(12345)),
                   loop.QuitClosure()));
    loop.Run();
  }
  {
    base::RunLoop loop;
    proxy->EchoNullableStructWithTraitsForUniquePtr(
        nullptr, base::Bind(&ExpectUniquePtr, nullptr, loop.QuitClosure()));
    loop.Run();
  }
}

TEST_F(StructTraitsTest, EchoUnionWithTraits) {
  TraitsTestServicePtr proxy = GetTraitsTestProxy();

  {
    std::unique_ptr<test::UnionWithTraitsBase> input(
        new test::UnionWithTraitsInt32(1234));
    base::RunLoop loop;
    proxy->EchoUnionWithTraits(
        std::move(input),
        base::Bind(
            [](const base::Closure& quit_closure,
               std::unique_ptr<test::UnionWithTraitsBase> passed) {
              ASSERT_EQ(test::UnionWithTraitsBase::Type::INT32, passed->type());
              EXPECT_EQ(1234,
                        static_cast<test::UnionWithTraitsInt32*>(passed.get())
                            ->value());
              quit_closure.Run();

            },
            loop.QuitClosure()));
    loop.Run();
  }

  {
    std::unique_ptr<test::UnionWithTraitsBase> input(
        new test::UnionWithTraitsStruct(4321));
    base::RunLoop loop;
    proxy->EchoUnionWithTraits(
        std::move(input),
        base::Bind(
            [](const base::Closure& quit_closure,
               std::unique_ptr<test::UnionWithTraitsBase> passed) {
              ASSERT_EQ(test::UnionWithTraitsBase::Type::STRUCT,
                        passed->type());
              EXPECT_EQ(4321,
                        static_cast<test::UnionWithTraitsStruct*>(passed.get())
                            ->get_struct()
                            .value);
              quit_closure.Run();

            },
            loop.QuitClosure()));
    loop.Run();
  }
}

TEST_F(StructTraitsTest, DefaultValueOfEnumWithTraits) {
  auto container = EnumWithTraitsContainer::New();
  EXPECT_EQ(EnumWithTraitsImpl::CUSTOM_VALUE_1, container->f_field);
}

}  // namespace test
}  // namespace mojo
