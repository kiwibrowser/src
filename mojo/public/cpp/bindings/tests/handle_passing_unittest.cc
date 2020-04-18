// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/bindings/tests/bindings_test_base.h"
#include "mojo/public/cpp/system/wait.h"
#include "mojo/public/cpp/test_support/test_utils.h"
#include "mojo/public/interfaces/bindings/tests/sample_factory.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

const char kText1[] = "hello";
const char kText2[] = "world";

void RecordString(std::string* storage,
                  const base::Closure& closure,
                  const std::string& str) {
  *storage = str;
  closure.Run();
}

base::Callback<void(const std::string&)> MakeStringRecorder(
    std::string* storage,
    const base::Closure& closure) {
  return base::Bind(&RecordString, storage, closure);
}

class ImportedInterfaceImpl : public imported::ImportedInterface {
 public:
  ImportedInterfaceImpl(
      InterfaceRequest<imported::ImportedInterface> request,
      const base::Closure& closure)
      : binding_(this, std::move(request)), closure_(closure) {}

  void DoSomething() override {
    do_something_count_++;
    closure_.Run();
  }

  static int do_something_count() { return do_something_count_; }

 private:
  static int do_something_count_;
  Binding<ImportedInterface> binding_;
  base::Closure closure_;
};
int ImportedInterfaceImpl::do_something_count_ = 0;

class SampleNamedObjectImpl : public sample::NamedObject {
 public:
  SampleNamedObjectImpl() {}

  void SetName(const std::string& name) override { name_ = name; }

  void GetName(const GetNameCallback& callback) override {
    callback.Run(name_);
  }

 private:
  std::string name_;
};

class SampleFactoryImpl : public sample::Factory {
 public:
  explicit SampleFactoryImpl(InterfaceRequest<sample::Factory> request)
      : binding_(this, std::move(request)) {}

  void DoStuff(sample::RequestPtr request,
               ScopedMessagePipeHandle pipe,
               const DoStuffCallback& callback) override {
    std::string text1;
    if (pipe.is_valid())
      EXPECT_TRUE(ReadTextMessage(pipe.get(), &text1));

    std::string text2;
    if (request->pipe.is_valid()) {
      EXPECT_TRUE(ReadTextMessage(request->pipe.get(), &text2));

      // Ensure that simply accessing request->pipe does not close it.
      EXPECT_TRUE(request->pipe.is_valid());
    }

    ScopedMessagePipeHandle pipe0;
    if (!text2.empty()) {
      CreateMessagePipe(nullptr, &pipe0, &pipe1_);
      EXPECT_TRUE(WriteTextMessage(pipe1_.get(), text2));
    }

    sample::ResponsePtr response(sample::Response::New(2, std::move(pipe0)));
    callback.Run(std::move(response), text1);

    if (request->obj) {
      imported::ImportedInterfacePtr proxy(std::move(request->obj));
      proxy->DoSomething();
    }
  }

  void DoStuff2(ScopedDataPipeConsumerHandle pipe,
                const DoStuff2Callback& callback) override {
    // Read the data from the pipe, writing the response (as a string) to
    // DidStuff2().
    ASSERT_TRUE(pipe.is_valid());
    uint32_t data_size = 0;

    MojoHandleSignalsState state;
    ASSERT_EQ(MOJO_RESULT_OK,
              mojo::Wait(pipe.get(), MOJO_HANDLE_SIGNAL_READABLE, &state));
    ASSERT_TRUE(state.satisfied_signals & MOJO_HANDLE_SIGNAL_READABLE);
    ASSERT_EQ(MOJO_RESULT_OK,
              pipe->ReadData(nullptr, &data_size, MOJO_READ_DATA_FLAG_QUERY));
    ASSERT_NE(0, static_cast<int>(data_size));
    char data[64];
    ASSERT_LT(static_cast<int>(data_size), 64);
    ASSERT_EQ(MOJO_RESULT_OK, pipe->ReadData(data, &data_size,
                                             MOJO_READ_DATA_FLAG_ALL_OR_NONE));

    callback.Run(data);
  }

  void CreateNamedObject(
      InterfaceRequest<sample::NamedObject> object_request) override {
    EXPECT_TRUE(object_request.is_pending());
    MakeStrongBinding(std::make_unique<SampleNamedObjectImpl>(),
                      std::move(object_request));
  }

  // These aren't called or implemented, but exist here to test that the
  // methods are generated with the correct argument types for imported
  // interfaces.
  void RequestImportedInterface(
      InterfaceRequest<imported::ImportedInterface> imported,
      const RequestImportedInterfaceCallback& callback) override {}
  void TakeImportedInterface(
      imported::ImportedInterfacePtr imported,
      const TakeImportedInterfaceCallback& callback) override {}

 private:
  ScopedMessagePipeHandle pipe1_;
  Binding<sample::Factory> binding_;
};

class HandlePassingTest : public BindingsTestBase {
 public:
  HandlePassingTest() {}

  void TearDown() override { PumpMessages(); }

  void PumpMessages() { base::RunLoop().RunUntilIdle(); }

};

void DoStuff(bool* got_response,
             std::string* got_text_reply,
             const base::Closure& closure,
             sample::ResponsePtr response,
             const std::string& text_reply) {
  *got_text_reply = text_reply;

  if (response->pipe.is_valid()) {
    std::string text2;
    EXPECT_TRUE(ReadTextMessage(response->pipe.get(), &text2));

    // Ensure that simply accessing response.pipe does not close it.
    EXPECT_TRUE(response->pipe.is_valid());

    EXPECT_EQ(std::string(kText2), text2);

    // Do some more tests of handle passing:
    ScopedMessagePipeHandle p = std::move(response->pipe);
    EXPECT_TRUE(p.is_valid());
    EXPECT_FALSE(response->pipe.is_valid());
  }

  *got_response = true;
  closure.Run();
}

void DoStuff2(bool* got_response,
              std::string* got_text_reply,
              const base::Closure& closure,
              const std::string& text_reply) {
  *got_response = true;
  *got_text_reply = text_reply;
  closure.Run();
}

TEST_P(HandlePassingTest, Basic) {
  sample::FactoryPtr factory;
  SampleFactoryImpl factory_impl(MakeRequest(&factory));

  MessagePipe pipe0;
  EXPECT_TRUE(WriteTextMessage(pipe0.handle1.get(), kText1));

  MessagePipe pipe1;
  EXPECT_TRUE(WriteTextMessage(pipe1.handle1.get(), kText2));

  imported::ImportedInterfacePtrInfo imported;
  base::RunLoop run_loop;
  ImportedInterfaceImpl imported_impl(MakeRequest(&imported),
                                      run_loop.QuitClosure());

  sample::RequestPtr request(sample::Request::New(
      1, std::move(pipe1.handle0), base::nullopt, std::move(imported)));
  bool got_response = false;
  std::string got_text_reply;
  base::RunLoop run_loop2;
  factory->DoStuff(std::move(request), std::move(pipe0.handle0),
                   base::Bind(&DoStuff, &got_response, &got_text_reply,
                              run_loop2.QuitClosure()));

  EXPECT_FALSE(got_response);
  int count_before = ImportedInterfaceImpl::do_something_count();

  run_loop.Run();
  run_loop2.Run();

  EXPECT_TRUE(got_response);
  EXPECT_EQ(kText1, got_text_reply);
  EXPECT_EQ(1, ImportedInterfaceImpl::do_something_count() - count_before);
}

TEST_P(HandlePassingTest, PassInvalid) {
  sample::FactoryPtr factory;
  SampleFactoryImpl factory_impl(MakeRequest(&factory));

  sample::RequestPtr request(sample::Request::New(1, ScopedMessagePipeHandle(),
                                                  base::nullopt, nullptr));

  bool got_response = false;
  std::string got_text_reply;
  base::RunLoop run_loop;
  factory->DoStuff(std::move(request), ScopedMessagePipeHandle(),
                   base::Bind(&DoStuff, &got_response, &got_text_reply,
                              run_loop.QuitClosure()));

  EXPECT_FALSE(got_response);

  run_loop.Run();

  EXPECT_TRUE(got_response);
}

// Verifies DataPipeConsumer can be passed and read from.
TEST_P(HandlePassingTest, DataPipe) {
  sample::FactoryPtr factory;
  SampleFactoryImpl factory_impl(MakeRequest(&factory));

  // Writes a string to a data pipe and passes the data pipe (consumer) to the
  // factory.
  ScopedDataPipeProducerHandle producer_handle;
  ScopedDataPipeConsumerHandle consumer_handle;
  MojoCreateDataPipeOptions options = {sizeof(MojoCreateDataPipeOptions),
                                       MOJO_CREATE_DATA_PIPE_FLAG_NONE, 1,
                                       1024};
  ASSERT_EQ(MOJO_RESULT_OK,
            CreateDataPipe(&options, &producer_handle, &consumer_handle));
  std::string expected_text_reply = "got it";
  // +1 for \0.
  uint32_t data_size = static_cast<uint32_t>(expected_text_reply.size() + 1);
  ASSERT_EQ(MOJO_RESULT_OK,
            producer_handle->WriteData(expected_text_reply.c_str(), &data_size,
                                       MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));

  bool got_response = false;
  std::string got_text_reply;
  base::RunLoop run_loop;
  factory->DoStuff2(std::move(consumer_handle),
                    base::Bind(&DoStuff2, &got_response, &got_text_reply,
                               run_loop.QuitClosure()));

  EXPECT_FALSE(got_response);

  run_loop.Run();

  EXPECT_TRUE(got_response);
  EXPECT_EQ(expected_text_reply, got_text_reply);
}

TEST_P(HandlePassingTest, CreateNamedObject) {
  sample::FactoryPtr factory;
  SampleFactoryImpl factory_impl(MakeRequest(&factory));

  sample::NamedObjectPtr object1;
  EXPECT_FALSE(object1);

  auto object1_request = mojo::MakeRequest(&object1);
  EXPECT_TRUE(object1_request.is_pending());
  factory->CreateNamedObject(std::move(object1_request));
  EXPECT_FALSE(object1_request.is_pending());  // We've passed the request.

  ASSERT_TRUE(object1);
  object1->SetName("object1");

  sample::NamedObjectPtr object2;
  factory->CreateNamedObject(MakeRequest(&object2));
  object2->SetName("object2");

  base::RunLoop run_loop, run_loop2;
  std::string name1;
  object1->GetName(MakeStringRecorder(&name1, run_loop.QuitClosure()));

  std::string name2;
  object2->GetName(MakeStringRecorder(&name2, run_loop2.QuitClosure()));

  run_loop.Run();
  run_loop2.Run();

  EXPECT_EQ(std::string("object1"), name1);
  EXPECT_EQ(std::string("object2"), name2);
}

INSTANTIATE_MOJO_BINDINGS_TEST_CASE_P(HandlePassingTest);

}  // namespace
}  // namespace test
}  // namespace mojo
