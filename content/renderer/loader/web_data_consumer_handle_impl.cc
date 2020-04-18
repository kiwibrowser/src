// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/loader/web_data_consumer_handle_impl.h"

#include <stdint.h>

#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "mojo/public/c/system/types.h"

namespace content {

using Result = blink::WebDataConsumerHandle::Result;

class WebDataConsumerHandleImpl::Context
    : public base::RefCountedThreadSafe<Context> {
 public:
  explicit Context(Handle handle) : handle_(std::move(handle)) {}

  const Handle& handle() { return handle_; }

 private:
  friend class base::RefCountedThreadSafe<Context>;
  ~Context() {}
  Handle handle_;

  DISALLOW_COPY_AND_ASSIGN(Context);
};

WebDataConsumerHandleImpl::ReaderImpl::ReaderImpl(
    scoped_refptr<Context> context,
    Client* client,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : context_(context),
      handle_watcher_(FROM_HERE,
                      mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                      std::move(task_runner)),
      client_(client) {
  if (client_)
    StartWatching();
}

WebDataConsumerHandleImpl::ReaderImpl::~ReaderImpl() {
}

Result WebDataConsumerHandleImpl::ReaderImpl::Read(void* data,
                                                   size_t size,
                                                   Flags flags,
                                                   size_t* read_size) {
  // We need this variable definition to avoid a link error.
  const Flags kNone = kFlagNone;
  DCHECK_EQ(flags, kNone);
  DCHECK_LE(size, std::numeric_limits<uint32_t>::max());

  *read_size = 0;

  if (!size) {
    // Even if there is unread data available, ReadData() returns
    // FAILED_PRECONDITION when |size| is 0 and the producer handle was closed.
    // But in this case, WebDataConsumerHandle::Reader::read() must return Ok.
    // So we query the signals state directly.
    mojo::HandleSignalsState state = context_->handle()->QuerySignalsState();
    if (state.readable())
      return kOk;
    if (state.never_readable())
      return kDone;
    return kShouldWait;
  }

  uint32_t size_to_pass = size;
  MojoReadDataFlags flags_to_pass = MOJO_READ_DATA_FLAG_NONE;
  MojoResult rv =
      context_->handle()->ReadData(data, &size_to_pass, flags_to_pass);
  if (rv == MOJO_RESULT_OK)
    *read_size = size_to_pass;
  if (rv == MOJO_RESULT_SHOULD_WAIT)
    handle_watcher_.ArmOrNotify();

  return HandleReadResult(rv);
}

Result WebDataConsumerHandleImpl::ReaderImpl::BeginRead(const void** buffer,
                                                        Flags flags,
                                                        size_t* available) {
  // We need this variable definition to avoid a link error.
  const Flags kNone = kFlagNone;
  DCHECK_EQ(flags, kNone);

  *buffer = nullptr;
  *available = 0;

  uint32_t size_to_pass = 0;
  MojoReadDataFlags flags_to_pass = MOJO_READ_DATA_FLAG_NONE;

  MojoResult rv =
      context_->handle()->BeginReadData(buffer, &size_to_pass, flags_to_pass);
  if (rv == MOJO_RESULT_OK)
    *available = size_to_pass;
  if (rv == MOJO_RESULT_SHOULD_WAIT)
    handle_watcher_.ArmOrNotify();
  return HandleReadResult(rv);
}

Result WebDataConsumerHandleImpl::ReaderImpl::EndRead(size_t read_size) {
  MojoResult rv = context_->handle()->EndReadData(read_size);
  return rv == MOJO_RESULT_OK ? kOk : kUnexpectedError;
}

Result WebDataConsumerHandleImpl::ReaderImpl::HandleReadResult(
    MojoResult mojo_result) {
  switch (mojo_result) {
    case MOJO_RESULT_OK:
      return kOk;
    case MOJO_RESULT_FAILED_PRECONDITION:
      return kDone;
    case MOJO_RESULT_BUSY:
      return kBusy;
    case MOJO_RESULT_SHOULD_WAIT:
      return kShouldWait;
    case MOJO_RESULT_RESOURCE_EXHAUSTED:
      return kResourceExhausted;
    default:
      return kUnexpectedError;
  }
}

void WebDataConsumerHandleImpl::ReaderImpl::StartWatching() {
  handle_watcher_.Watch(
      context_->handle().get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&ReaderImpl::OnHandleGotReadable, base::Unretained(this)));
  handle_watcher_.ArmOrNotify();
}

void WebDataConsumerHandleImpl::ReaderImpl::OnHandleGotReadable(MojoResult) {
  DCHECK(client_);
  client_->DidGetReadable();
}

WebDataConsumerHandleImpl::WebDataConsumerHandleImpl(Handle handle)
    : context_(new Context(std::move(handle))) {}

WebDataConsumerHandleImpl::~WebDataConsumerHandleImpl() {
}

std::unique_ptr<blink::WebDataConsumerHandle::Reader>
WebDataConsumerHandleImpl::ObtainReader(
    Client* client,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  return base::WrapUnique(
      new ReaderImpl(context_, client, std::move(task_runner)));
}

const char* WebDataConsumerHandleImpl::DebugName() const {
  return "WebDataConsumerHandleImpl";
}

}  // namespace content
