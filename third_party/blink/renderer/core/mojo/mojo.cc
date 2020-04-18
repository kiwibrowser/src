// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/mojo/mojo.h"

#include <string>

#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/mojo/mojo_create_data_pipe_options.h"
#include "third_party/blink/renderer/core/mojo/mojo_create_data_pipe_result.h"
#include "third_party/blink/renderer/core/mojo/mojo_create_message_pipe_result.h"
#include "third_party/blink/renderer/core/mojo/mojo_create_shared_buffer_result.h"
#include "third_party/blink/renderer/core/mojo/mojo_handle.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/wtf/text/string_utf8_adaptor.h"

namespace blink {

// static
void Mojo::createMessagePipe(MojoCreateMessagePipeResult& result_dict) {
  MojoCreateMessagePipeOptions options = {0};
  options.struct_size = sizeof(::MojoCreateMessagePipeOptions);
  options.flags = MOJO_CREATE_MESSAGE_PIPE_FLAG_NONE;

  mojo::ScopedMessagePipeHandle handle0, handle1;
  MojoResult result = mojo::CreateMessagePipe(&options, &handle0, &handle1);

  result_dict.setResult(result);
  if (result == MOJO_RESULT_OK) {
    result_dict.setHandle0(
        MojoHandle::Create(mojo::ScopedHandle::From(std::move(handle0))));
    result_dict.setHandle1(
        MojoHandle::Create(mojo::ScopedHandle::From(std::move(handle1))));
  }
}

// static
void Mojo::createDataPipe(const MojoCreateDataPipeOptions& options_dict,
                          MojoCreateDataPipeResult& result_dict) {
  if (!options_dict.hasElementNumBytes() ||
      !options_dict.hasCapacityNumBytes()) {
    result_dict.setResult(MOJO_RESULT_INVALID_ARGUMENT);
    return;
  }

  ::MojoCreateDataPipeOptions options = {0};
  options.struct_size = sizeof(options);
  options.flags = MOJO_CREATE_DATA_PIPE_FLAG_NONE;
  options.element_num_bytes = options_dict.elementNumBytes();
  options.capacity_num_bytes = options_dict.capacityNumBytes();

  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  MojoResult result = mojo::CreateDataPipe(&options, &producer, &consumer);
  result_dict.setResult(result);
  if (result == MOJO_RESULT_OK) {
    result_dict.setProducer(
        MojoHandle::Create(mojo::ScopedHandle::From(std::move(producer))));
    result_dict.setConsumer(
        MojoHandle::Create(mojo::ScopedHandle::From(std::move(consumer))));
  }
}

// static
void Mojo::createSharedBuffer(unsigned num_bytes,
                              MojoCreateSharedBufferResult& result_dict) {
  MojoCreateSharedBufferOptions* options = nullptr;
  mojo::Handle handle;
  MojoResult result =
      MojoCreateSharedBuffer(num_bytes, options, handle.mutable_value());

  result_dict.setResult(result);
  if (result == MOJO_RESULT_OK) {
    result_dict.setHandle(MojoHandle::Create(mojo::MakeScopedHandle(handle)));
  }
}

// static
void Mojo::bindInterface(ScriptState* script_state,
                         const String& interface_name,
                         MojoHandle* request_handle,
                         const String& scope) {
  std::string name =
      StringUTF8Adaptor(interface_name).AsStringPiece().as_string();
  auto handle =
      mojo::ScopedMessagePipeHandle::From(request_handle->TakeHandle());

  if (scope == "process") {
    Platform::Current()->GetInterfaceProvider()->GetInterface(
        name.c_str(), std::move(handle));
    return;
  }

  if (auto* interface_provider =
          ExecutionContext::From(script_state)->GetInterfaceProvider()) {
    interface_provider->GetInterfaceByName(name, std::move(handle));
  }
}

}  // namespace blink
