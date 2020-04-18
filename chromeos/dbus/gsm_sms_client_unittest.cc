// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/gsm_sms_client.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/values.h"
#include "dbus/message.h"
#include "dbus/mock_bus.h"
#include "dbus/mock_object_proxy.h"
#include "dbus/object_path.h"
#include "dbus/values_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace chromeos {

namespace {

// D-Bus service name used by test.
const char kServiceName[] = "service.name";
// D-Bus object path used by test.
const char kObjectPath[] = "/object/path";

// Keys of SMS dictionary.
const char kNumberKey[] = "number";
const char kTextKey[] = "text";

// Example values of SMS dictionary.
const char kExampleNumber[] = "00012345678";
const char kExampleText[] = "Hello.";

}  // namespace

class GsmSMSClientTest : public testing::Test {
 public:
  GsmSMSClientTest() : expected_index_(0), response_(NULL) {}

  void SetUp() override {
    // Create a mock bus.
    dbus::Bus::Options options;
    options.bus_type = dbus::Bus::SYSTEM;
    mock_bus_ = new dbus::MockBus(options);

    // Create a mock proxy.
    mock_proxy_ = new dbus::MockObjectProxy(mock_bus_.get(),
                                            kServiceName,
                                            dbus::ObjectPath(kObjectPath));

    // Set an expectation so mock_proxy's ConnectToSignal() will use
    // OnConnectToSignal() to run the callback.
    EXPECT_CALL(*mock_proxy_.get(),
                DoConnectToSignal(modemmanager::kModemManagerSMSInterface,
                                  modemmanager::kSMSReceivedSignal, _, _))
        .WillRepeatedly(Invoke(this, &GsmSMSClientTest::OnConnectToSignal));

    // Set an expectation so mock_bus's GetObjectProxy() for the given
    // service name and the object path will return mock_proxy_.
    EXPECT_CALL(*mock_bus_.get(),
                GetObjectProxy(kServiceName, dbus::ObjectPath(kObjectPath)))
        .WillOnce(Return(mock_proxy_.get()));

    // ShutdownAndBlock() will be called in TearDown().
    EXPECT_CALL(*mock_bus_.get(), ShutdownAndBlock()).WillOnce(Return());

    // Create a client with the mock bus.
    client_.reset(GsmSMSClient::Create());
    client_->Init(mock_bus_.get());
  }

  void TearDown() override { mock_bus_->ShutdownAndBlock(); }

  // Handles Delete method call.
  void OnDelete(dbus::MethodCall* method_call,
                int timeout_ms,
                dbus::ObjectProxy::ResponseCallback* callback) {
    EXPECT_EQ(modemmanager::kModemManagerSMSInterface,
              method_call->GetInterface());
    EXPECT_EQ(modemmanager::kSMSDeleteFunction, method_call->GetMember());
    uint32_t index = 0;
    dbus::MessageReader reader(method_call);
    EXPECT_TRUE(reader.PopUint32(&index));
    EXPECT_EQ(expected_index_, index);
    EXPECT_FALSE(reader.HasMoreData());

    message_loop_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(std::move(*callback), response_));
  }

  // Handles Get method call.
  void OnGet(dbus::MethodCall* method_call,
             int timeout_ms,
             dbus::ObjectProxy::ResponseCallback* callback) {
    EXPECT_EQ(modemmanager::kModemManagerSMSInterface,
              method_call->GetInterface());
    EXPECT_EQ(modemmanager::kSMSGetFunction, method_call->GetMember());
    uint32_t index = 0;
    dbus::MessageReader reader(method_call);
    EXPECT_TRUE(reader.PopUint32(&index));
    EXPECT_EQ(expected_index_, index);
    EXPECT_FALSE(reader.HasMoreData());

    message_loop_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(std::move(*callback), response_));
  }

  // Handles List method call.
  void OnList(dbus::MethodCall* method_call,
              int timeout_ms,
              dbus::ObjectProxy::ResponseCallback* callback) {
    EXPECT_EQ(modemmanager::kModemManagerSMSInterface,
              method_call->GetInterface());
    EXPECT_EQ(modemmanager::kSMSListFunction, method_call->GetMember());
    dbus::MessageReader reader(method_call);
    EXPECT_FALSE(reader.HasMoreData());

    message_loop_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(std::move(*callback), response_));
  }

 protected:
  // The client to be tested.
  std::unique_ptr<GsmSMSClient> client_;
  // A message loop to emulate asynchronous behavior.
  base::MessageLoop message_loop_;
  // The mock bus.
  scoped_refptr<dbus::MockBus> mock_bus_;
  // The mock object proxy.
  scoped_refptr<dbus::MockObjectProxy> mock_proxy_;
  // The SmsReceived signal handler given by the tested client.
  dbus::ObjectProxy::SignalCallback sms_received_callback_;
  // Expected argument for Delete and Get methods.
  uint32_t expected_index_;
  // Response returned by mock methods.
  dbus::Response* response_;

 private:
  // Used to implement the mock proxy.
  void OnConnectToSignal(
      const std::string& interface_name,
      const std::string& signal_name,
      const dbus::ObjectProxy::SignalCallback& signal_callback,
      dbus::ObjectProxy::OnConnectedCallback* on_connected_callback) {
    sms_received_callback_ = signal_callback;
    const bool success = true;
    message_loop_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(std::move(*on_connected_callback),
                                  interface_name, signal_name, success));
  }
};

TEST_F(GsmSMSClientTest, SmsReceived) {
  // Set expectations.
  const uint32_t kIndex = 42;
  const bool kComplete = true;

  // Set handler.
  int num_called = 0;
  uint32_t index = 0;
  bool completed = false;
  client_->SetSmsReceivedHandler(
      kServiceName, dbus::ObjectPath(kObjectPath),
      base::BindRepeating(
          [](int* num_called, uint32_t* index_out, bool* completed_out,
             uint32_t index, bool completed) {
            ++*num_called;
            *index_out = index;
            *completed_out = completed;
          },
          &num_called, &index, &completed));

  // Run the message loop to run the signal connection result callback.
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0, num_called);

  // Send signal.
  dbus::Signal signal(modemmanager::kModemManagerSMSInterface,
                      modemmanager::kSMSReceivedSignal);
  dbus::MessageWriter writer(&signal);
  writer.AppendUint32(kIndex);
  writer.AppendBool(kComplete);
  ASSERT_FALSE(sms_received_callback_.is_null());
  sms_received_callback_.Run(&signal);

  EXPECT_EQ(1, num_called);
  EXPECT_EQ(kIndex, index);
  EXPECT_TRUE(completed);

  // Reset handler.
  client_->ResetSmsReceivedHandler(kServiceName, dbus::ObjectPath(kObjectPath));

  // Send signal again.
  sms_received_callback_.Run(&signal);
  EXPECT_EQ(1, num_called);
}

TEST_F(GsmSMSClientTest, Delete) {
  // Set expectations.
  const uint32_t kIndex = 42;
  expected_index_ = kIndex;
  EXPECT_CALL(*mock_proxy_.get(), DoCallMethod(_, _, _))
      .WillOnce(Invoke(this, &GsmSMSClientTest::OnDelete));

  // Create response.
  std::unique_ptr<dbus::Response> response(dbus::Response::CreateEmpty());
  response_ = response.get();

  // Call Delete.
  base::Optional<bool> success;
  client_->Delete(
      kServiceName, dbus::ObjectPath(kObjectPath), kIndex,
      base::BindOnce([](base::Optional<bool>* success_out,
                        bool success) { success_out->emplace(success); },
                     &success));

  // Run the message loop.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(true, success);
}

TEST_F(GsmSMSClientTest, Get) {
  // Set expectations.
  const uint32_t kIndex = 42;
  expected_index_ = kIndex;
  EXPECT_CALL(*mock_proxy_.get(), DoCallMethod(_, _, _))
      .WillOnce(Invoke(this, &GsmSMSClientTest::OnGet));
  // Create response.
  std::unique_ptr<dbus::Response> response(dbus::Response::CreateEmpty());
  dbus::MessageWriter writer(response.get());
  dbus::MessageWriter array_writer(NULL);
  writer.OpenArray("{sv}", &array_writer);
  dbus::MessageWriter entry_writer(NULL);
  array_writer.OpenDictEntry(&entry_writer);
  entry_writer.AppendString(kNumberKey);
  entry_writer.AppendVariantOfString(kExampleNumber);
  array_writer.CloseContainer(&entry_writer);
  array_writer.OpenDictEntry(&entry_writer);
  entry_writer.AppendString(kTextKey);
  entry_writer.AppendVariantOfString(kExampleText);
  array_writer.CloseContainer(&entry_writer);
  writer.CloseContainer(&array_writer);
  response_ = response.get();

  // Call Get.
  base::Optional<base::DictionaryValue> result;
  client_->Get(kServiceName, dbus::ObjectPath(kObjectPath), kIndex,
               base::BindOnce(
                   [](base::Optional<base::DictionaryValue>* result_out,
                      base::Optional<base::DictionaryValue> result) {
                     *result_out = std::move(result);
                   },
                   &result));

  // Run the message loop.
  base::RunLoop().RunUntilIdle();

  // Verify the result.
  base::DictionaryValue expected_result;
  expected_result.SetKey(kNumberKey, base::Value(kExampleNumber));
  expected_result.SetKey(kTextKey, base::Value(kExampleText));
  EXPECT_EQ(expected_result, result);
}

TEST_F(GsmSMSClientTest, List) {
  // Set expectations.
  EXPECT_CALL(*mock_proxy_.get(), DoCallMethod(_, _, _))
      .WillOnce(Invoke(this, &GsmSMSClientTest::OnList));
  // Create response.
  std::unique_ptr<dbus::Response> response(dbus::Response::CreateEmpty());
  dbus::MessageWriter writer(response.get());
  dbus::MessageWriter array_writer(NULL);
  writer.OpenArray("a{sv}", &array_writer);
  dbus::MessageWriter sub_array_writer(NULL);
  array_writer.OpenArray("{sv}", &sub_array_writer);
  dbus::MessageWriter entry_writer(NULL);
  sub_array_writer.OpenDictEntry(&entry_writer);
  entry_writer.AppendString(kNumberKey);
  entry_writer.AppendVariantOfString(kExampleNumber);
  sub_array_writer.CloseContainer(&entry_writer);
  sub_array_writer.OpenDictEntry(&entry_writer);
  entry_writer.AppendString(kTextKey);
  entry_writer.AppendVariantOfString(kExampleText);
  sub_array_writer.CloseContainer(&entry_writer);
  array_writer.CloseContainer(&sub_array_writer);
  writer.CloseContainer(&array_writer);
  response_ = response.get();

  // Call List.
  base::Optional<base::ListValue> result;
  client_->List(kServiceName, dbus::ObjectPath(kObjectPath),
                base::BindOnce(
                    [](base::Optional<base::ListValue>* result_out,
                       base::Optional<base::ListValue> result) {
                      *result_out = std::move(result);
                    },
                    &result));

  // Run the message loop.
  base::RunLoop().RunUntilIdle();

  // Create expected result.
  base::ListValue expected_result;
  auto sms = std::make_unique<base::DictionaryValue>();
  sms->SetKey(kNumberKey, base::Value(kExampleNumber));
  sms->SetKey(kTextKey, base::Value(kExampleText));
  expected_result.Append(std::move(sms));
  EXPECT_EQ(expected_result, result);
}

}  // namespace chromeos
