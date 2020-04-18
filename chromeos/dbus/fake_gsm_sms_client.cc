// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/dbus/fake_gsm_sms_client.h"

namespace chromeos {

FakeGsmSMSClient::FakeGsmSMSClient()
    : test_index_(-1),
      sms_test_message_switch_present_(false),
      weak_ptr_factory_(this) {
  test_messages_.push_back("Test Message 0");
  test_messages_.push_back("Test Message 1");
  test_messages_.push_back("Test a relatively long message 2");
  test_messages_.push_back("Test a very, the quick brown fox jumped"
                           " over the lazy dog, long message 3");
  test_messages_.push_back("Test Message 4");
  test_messages_.push_back("Test Message 5");
  test_messages_.push_back("Test Message 6");
}

FakeGsmSMSClient::~FakeGsmSMSClient() = default;

void FakeGsmSMSClient::Init(dbus::Bus* bus) {
}

void FakeGsmSMSClient::SetSmsReceivedHandler(
    const std::string& service_name,
    const dbus::ObjectPath& object_path,
    const SmsReceivedHandler& handler) {
  handler_ = handler;
}

void FakeGsmSMSClient::ResetSmsReceivedHandler(
    const std::string& service_name,
    const dbus::ObjectPath& object_path) {
  handler_.Reset();
}

void FakeGsmSMSClient::Delete(const std::string& service_name,
                              const dbus::ObjectPath& object_path,
                              uint32_t index,
                              VoidDBusMethodCallback callback) {
  message_list_.Remove(index, nullptr);
  std::move(callback).Run(true);
}

void FakeGsmSMSClient::Get(const std::string& service_name,
                           const dbus::ObjectPath& object_path,
                           uint32_t index,
                           DBusMethodCallback<base::DictionaryValue> callback) {
  base::DictionaryValue* dictionary = nullptr;
  if (!message_list_.GetDictionary(index, &dictionary)) {
    std::move(callback).Run(base::nullopt);
    return;
  }

  // TODO(crbug.com/646113): Once migration is done, this can be simplified.
  base::DictionaryValue copy;
  copy.MergeDictionary(dictionary);
  std::move(callback).Run(std::move(copy));
}

void FakeGsmSMSClient::List(const std::string& service_name,
                            const dbus::ObjectPath& object_path,
                            DBusMethodCallback<base::ListValue> callback) {
  std::move(callback).Run(base::ListValue(message_list_.GetList()));
}

void FakeGsmSMSClient::RequestUpdate(const std::string& service_name,
                                     const dbus::ObjectPath& object_path) {
  if (!sms_test_message_switch_present_)
    return;

  if (test_index_ >= 0)
    return;
  test_index_ = 0;
  // Call PushTestMessageChain asynchronously so that the handler_ callback
  // does not get called from the update request.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&FakeGsmSMSClient::PushTestMessageChain,
                                weak_ptr_factory_.GetWeakPtr()));
}

void FakeGsmSMSClient::PushTestMessageChain() {
  if (PushTestMessage())
    PushTestMessageDelayed();
}

void FakeGsmSMSClient::PushTestMessageDelayed() {
  const int kSmsMessageDelaySeconds = 5;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&FakeGsmSMSClient::PushTestMessageChain,
                     weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(kSmsMessageDelaySeconds));
}

bool FakeGsmSMSClient::PushTestMessage() {
  if (test_index_ >= static_cast<int>(test_messages_.size()))
    return false;
  auto message = std::make_unique<base::DictionaryValue>();
  message->SetString("number", "000-000-0000");
  message->SetString("text", test_messages_[test_index_]);
  message->SetInteger("index", test_index_);
  int msg_index = message_list_.GetSize();
  message_list_.Append(std::move(message));
  if (!handler_.is_null())
    handler_.Run(msg_index, true);
  ++test_index_;
  return true;
}

}  // namespace chromeos
