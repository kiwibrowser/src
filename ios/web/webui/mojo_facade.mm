// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/webui/mojo_facade.h"

#include <stdint.h>

#include <utility>
#include <vector>

#import <Foundation/Foundation.h>

#import "base/ios/block_types.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#import "base/mac/bind_objc_block.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#import "ios/web/public/web_state/js/crw_js_injection_evaluator.h"
#include "ios/web/public/web_thread.h"
#include "mojo/public/cpp/system/core.h"
#include "services/service_manager/public/mojom/interface_provider.mojom.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

namespace {

// Wraps an integer into |base::Value| as |Type::INTEGER|.
template <typename IntegerT>
std::unique_ptr<base::Value> ValueFromInteger(IntegerT handle) {
  return std::make_unique<base::Value>(static_cast<int>(handle));
}

}  // namespace

MojoFacade::MojoFacade(
    service_manager::mojom::InterfaceProvider* interface_provider,
    id<CRWJSInjectionEvaluator> script_evaluator)
    : interface_provider_(interface_provider),
      script_evaluator_(script_evaluator) {
  DCHECK_CURRENTLY_ON(WebThread::UI);
  DCHECK(interface_provider_);
  DCHECK(script_evaluator_);
}

MojoFacade::~MojoFacade() {
  DCHECK_CURRENTLY_ON(WebThread::UI);
}

std::string MojoFacade::HandleMojoMessage(
    const std::string& mojo_message_as_json) {
  DCHECK_CURRENTLY_ON(WebThread::UI);
  std::string name;
  std::unique_ptr<base::DictionaryValue> args;
  GetMessageNameAndArguments(mojo_message_as_json, &name, &args);

  std::unique_ptr<base::Value> result;
  if (name == "Mojo.bindInterface") {
    result = HandleMojoBindInterface(args.get());
  } else if (name == "MojoHandle.close") {
    result = HandleMojoHandleClose(args.get());
  } else if (name == "Mojo.createMessagePipe") {
    result = HandleMojoCreateMessagePipe(args.get());
  } else if (name == "MojoHandle.writeMessage") {
    result = HandleMojoHandleWriteMessage(args.get());
  } else if (name == "MojoHandle.readMessage") {
    result = HandleMojoHandleReadMessage(args.get());
  } else if (name == "MojoHandle.watch") {
    result = HandleMojoHandleWatch(args.get());
  } else if (name == "MojoWatcher.cancel") {
    result = HandleMojoWatcherCancel(args.get());
  }

  if (!result) {
    return "";
  }

  std::string json_result;
  base::JSONWriter::Write(*result, &json_result);
  return json_result;
}

void MojoFacade::GetMessageNameAndArguments(
    const std::string& mojo_message_as_json,
    std::string* out_name,
    std::unique_ptr<base::DictionaryValue>* out_args) {
  int error_code = 0;
  std::string error_message;
  std::unique_ptr<base::Value> mojo_message_as_value(
      base::JSONReader::ReadAndReturnError(mojo_message_as_json, false,
                                           &error_code, &error_message));
  CHECK(!error_code);
  base::DictionaryValue* mojo_message = nullptr;
  CHECK(mojo_message_as_value->GetAsDictionary(&mojo_message));

  std::string name;
  CHECK(mojo_message->GetString("name", &name));

  base::DictionaryValue* args = nullptr;
  CHECK(mojo_message->GetDictionary("args", &args));

  *out_name = name;
  *out_args = args->CreateDeepCopy();
}

std::unique_ptr<base::Value> MojoFacade::HandleMojoBindInterface(
    const base::DictionaryValue* args) {
  const base::Value* interface_name_as_value = nullptr;
  CHECK(args->Get("interfaceName", &interface_name_as_value));
  int raw_handle = 0;
  CHECK(args->GetInteger("requestHandle", &raw_handle));

  mojo::ScopedMessagePipeHandle handle(
      static_cast<mojo::MessagePipeHandle>(raw_handle));

  // By design interface_provider.getInterface either succeeds or crashes, so
  // check if interface name is a valid string is intentionally omitted.
  std::string interface_name_as_string;
  interface_name_as_value->GetAsString(&interface_name_as_string);

  interface_provider_->GetInterface(interface_name_as_string,
                                    std::move(handle));
  return nullptr;
}

std::unique_ptr<base::Value> MojoFacade::HandleMojoHandleClose(
    const base::DictionaryValue* args) {
  int handle = 0;
  CHECK(args->GetInteger("handle", &handle));

  mojo::Handle(handle).Close();
  return nullptr;
}

std::unique_ptr<base::Value> MojoFacade::HandleMojoCreateMessagePipe(
    base::DictionaryValue* args) {
  mojo::ScopedMessagePipeHandle handle0, handle1;
  MojoResult mojo_result = mojo::CreateMessagePipe(nullptr, &handle0, &handle1);
  auto result = std::make_unique<base::DictionaryValue>();
  result->SetInteger("result", mojo_result);
  if (mojo_result == MOJO_RESULT_OK) {
    result->SetInteger("handle0", handle0.release().value());
    result->SetInteger("handle1", handle1.release().value());
  }
  return std::unique_ptr<base::Value>(result.release());
}

std::unique_ptr<base::Value> MojoFacade::HandleMojoHandleWriteMessage(
    base::DictionaryValue* args) {
  int handle = 0;
  CHECK(args->GetInteger("handle", &handle));

  base::ListValue* handles_list = nullptr;
  CHECK(args->GetList("handles", &handles_list));

  base::DictionaryValue* buffer = nullptr;
  CHECK(args->GetDictionary("buffer", &buffer));

  int flags = MOJO_WRITE_MESSAGE_FLAG_NONE;

  std::vector<MojoHandle> handles(handles_list->GetSize());
  for (size_t i = 0; i < handles_list->GetSize(); i++) {
    int one_handle = 0;
    handles_list->GetInteger(i, &one_handle);
    handles[i] = one_handle;
  }

  std::vector<uint8_t> bytes(buffer->size());
  for (size_t i = 0; i < buffer->size(); i++) {
    int one_byte = 0;
    buffer->GetInteger(base::IntToString(i), &one_byte);
    bytes[i] = one_byte;
  }

  mojo::MessagePipeHandle message_pipe(static_cast<MojoHandle>(handle));
  MojoResult result =
      mojo::WriteMessageRaw(message_pipe, bytes.data(), bytes.size(),
                            handles.data(), handles.size(), flags);

  return ValueFromInteger(result);
}

std::unique_ptr<base::Value> MojoFacade::HandleMojoHandleReadMessage(
    const base::DictionaryValue* args) {
  const base::Value* handle_as_value = nullptr;
  CHECK(args->Get("handle", &handle_as_value));
  int handle_as_int = 0;
  if (!handle_as_value->GetAsInteger(&handle_as_int)) {
    handle_as_int = 0;
  }

  int flags = MOJO_READ_MESSAGE_FLAG_NONE;

  std::vector<uint8_t> bytes;
  std::vector<mojo::ScopedHandle> handles;
  mojo::MessagePipeHandle handle(static_cast<MojoHandle>(handle_as_int));
  MojoResult mojo_result =
      mojo::ReadMessageRaw(handle, &bytes, &handles, flags);
  auto result = std::make_unique<base::DictionaryValue>();
  if (mojo_result == MOJO_RESULT_OK) {
    auto handles_list = std::make_unique<base::ListValue>();
    for (uint32_t i = 0; i < handles.size(); i++) {
      handles_list->AppendInteger(handles[i].release().value());
    }
    result->Set("handles", std::move(handles_list));

    auto buffer = std::make_unique<base::ListValue>();
    for (uint32_t i = 0; i < bytes.size(); i++) {
      buffer->AppendInteger(bytes[i]);
    }
    result->Set("buffer", std::move(buffer));
  }
  result->SetInteger("result", mojo_result);

  return std::unique_ptr<base::Value>(result.release());
}

std::unique_ptr<base::Value> MojoFacade::HandleMojoHandleWatch(
    const base::DictionaryValue* args) {
  int handle = 0;
  CHECK(args->GetInteger("handle", &handle));
  int signals = 0;
  CHECK(args->GetInteger("signals", &signals));
  int callback_id;
  CHECK(args->GetInteger("callbackId", &callback_id));

  mojo::SimpleWatcher::ReadyCallback callback =
      base::BindBlockArc(^(MojoResult result) {
        NSString* script = [NSString
            stringWithFormat:
                @"Mojo.internal.watchCallbacksHolder.callCallback(%d, %d)",
                callback_id, result];
        [script_evaluator_ executeJavaScript:script completionHandler:nil];
      });
  auto watcher = std::make_unique<mojo::SimpleWatcher>(
      FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC);
  watcher->Watch(static_cast<mojo::Handle>(handle), signals, callback);
  watchers_.insert(std::make_pair(++last_watch_id_, std::move(watcher)));
  return ValueFromInteger(last_watch_id_);
}

std::unique_ptr<base::Value> MojoFacade::HandleMojoWatcherCancel(
    const base::DictionaryValue* args) {
  int watch_id = 0;
  CHECK(args->GetInteger("watchId", &watch_id));
  watchers_.erase(watch_id);
  return nullptr;
}

}  // namespace web
