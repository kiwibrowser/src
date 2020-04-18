/*
 * Copyright (C) 2011, 2012 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/websockets/worker_websocket_channel.h"

#include <memory>
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fileapi/blob.h"
#include "third_party/blink/renderer/core/loader/threadable_loading_context.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/core/workers/worker_thread_lifecycle_context.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/heap/safe_point.h"
#include "third_party/blink/renderer/platform/waitable_event.h"
#include "third_party/blink/renderer/platform/web_task_runner.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

typedef WorkerWebSocketChannel::Bridge Bridge;
typedef WorkerWebSocketChannel::MainChannelClient MainChannelClient;

// Created and destroyed on the worker thread. All setters of this class are
// called on the main thread, while all getters are called on the worker
// thread. signalWorkerThread() must be called before any getters are called.
class WebSocketChannelSyncHelper {
 public:
  WebSocketChannelSyncHelper() = default;
  ~WebSocketChannelSyncHelper() = default;

  // All setters are called on the main thread.
  void SetConnectRequestResult(bool connect_request_result) {
    DCHECK(IsMainThread());
    connect_request_result_ = connect_request_result;
  }

  // All getters are called on the worker thread.
  bool ConnectRequestResult() const {
    DCHECK(!IsMainThread());
    return connect_request_result_;
  }

  // This should be called after all setters are called and before any
  // getters are called.
  void SignalWorkerThread() {
    DCHECK(IsMainThread());
    event_.Signal();
  }

  void Wait() {
    DCHECK(!IsMainThread());
    event_.Wait();
  }

 private:
  WaitableEvent event_;
  bool connect_request_result_ = false;
};

WorkerWebSocketChannel::WorkerWebSocketChannel(
    WorkerGlobalScope& worker_global_scope,
    WebSocketChannelClient* client,
    std::unique_ptr<SourceLocation> location)
    : bridge_(new Bridge(client, worker_global_scope)),
      location_at_connection_(std::move(location)) {}

WorkerWebSocketChannel::~WorkerWebSocketChannel() {
  DCHECK(!bridge_);
}

bool WorkerWebSocketChannel::Connect(const KURL& url, const String& protocol) {
  DCHECK(bridge_);
  return bridge_->Connect(location_at_connection_->Clone(), url, protocol);
}

void WorkerWebSocketChannel::Send(const CString& message) {
  DCHECK(bridge_);
  bridge_->Send(message);
}

void WorkerWebSocketChannel::Send(const DOMArrayBuffer& binary_data,
                                  unsigned byte_offset,
                                  unsigned byte_length) {
  DCHECK(bridge_);
  bridge_->Send(binary_data, byte_offset, byte_length);
}

void WorkerWebSocketChannel::Send(scoped_refptr<BlobDataHandle> blob_data) {
  DCHECK(bridge_);
  bridge_->Send(std::move(blob_data));
}

void WorkerWebSocketChannel::Close(int code, const String& reason) {
  DCHECK(bridge_);
  bridge_->Close(code, reason);
}

void WorkerWebSocketChannel::Fail(const String& reason,
                                  MessageLevel level,
                                  std::unique_ptr<SourceLocation> location) {
  DCHECK(bridge_);

  std::unique_ptr<SourceLocation> captured_location = SourceLocation::Capture();
  if (!captured_location->IsUnknown()) {
    // If we are in JavaScript context, use the current location instead
    // of passed one - it's more precise.
    bridge_->Fail(reason, level, std::move(captured_location));
  } else if (location->IsUnknown()) {
    // No information is specified by the caller - use the url
    // and the line number at the connection.
    bridge_->Fail(reason, level, location_at_connection_->Clone());
  } else {
    // Use the specified information.
    bridge_->Fail(reason, level, std::move(location));
  }
}

void WorkerWebSocketChannel::Disconnect() {
  bridge_->Disconnect();
  bridge_.Clear();
}

void WorkerWebSocketChannel::Trace(blink::Visitor* visitor) {
  visitor->Trace(bridge_);
  WebSocketChannel::Trace(visitor);
}

MainChannelClient::MainChannelClient(
    Bridge* bridge,
    scoped_refptr<base::SingleThreadTaskRunner> worker_networking_task_runner,
    WorkerThreadLifecycleContext* worker_thread_lifecycle_context)
    : WorkerThreadLifecycleObserver(worker_thread_lifecycle_context),
      bridge_(bridge),
      worker_networking_task_runner_(std::move(worker_networking_task_runner)),
      main_channel_(nullptr) {
  DCHECK(IsMainThread());
}

MainChannelClient::~MainChannelClient() {
  DCHECK(IsMainThread());
}

bool MainChannelClient::Initialize(std::unique_ptr<SourceLocation> location,
                                   ThreadableLoadingContext* loading_context) {
  DCHECK(IsMainThread());
  if (WasContextDestroyedBeforeObserverCreation())
    return false;
  main_channel_ =
      WebSocketChannelImpl::Create(loading_context, this, std::move(location));
  return true;
}

bool MainChannelClient::Connect(
    const KURL& url,
    const String& protocol,
    network::mojom::blink::WebSocketPtr socket_ptr) {
  DCHECK(IsMainThread());
  if (!main_channel_)
    return false;
  if (socket_ptr)
    return main_channel_->Connect(url, protocol, std::move(socket_ptr));
  // See Bridge::Connect for an explanation of the case where |socket_ptr| is
  // null. In this case we allow |main_channel_| to connect the pipe for us.
  return main_channel_->Connect(url, protocol);
}

void MainChannelClient::SendTextAsCharVector(
    std::unique_ptr<Vector<char>> data) {
  DCHECK(IsMainThread());
  if (main_channel_)
    main_channel_->SendTextAsCharVector(std::move(data));
}

void MainChannelClient::SendBinaryAsCharVector(
    std::unique_ptr<Vector<char>> data) {
  DCHECK(IsMainThread());
  if (main_channel_)
    main_channel_->SendBinaryAsCharVector(std::move(data));
}

void MainChannelClient::SendBlob(scoped_refptr<BlobDataHandle> blob_data) {
  DCHECK(IsMainThread());
  if (main_channel_)
    main_channel_->Send(std::move(blob_data));
}

void MainChannelClient::Close(int code, const String& reason) {
  DCHECK(IsMainThread());
  if (main_channel_)
    main_channel_->Close(code, reason);
}

void MainChannelClient::Fail(const String& reason,
                             MessageLevel level,
                             std::unique_ptr<SourceLocation> location) {
  DCHECK(IsMainThread());
  if (main_channel_)
    main_channel_->Fail(reason, level, std::move(location));
}

void MainChannelClient::ReleaseMainChannel() {
  if (!main_channel_)
    return;

  main_channel_->Disconnect();
  main_channel_ = nullptr;
}

void MainChannelClient::Disconnect() {
  DCHECK(IsMainThread());

  ReleaseMainChannel();
}

static void WorkerGlobalScopeDidConnect(Bridge* bridge,
                                        const String& subprotocol,
                                        const String& extensions) {
  if (bridge && bridge->Client())
    bridge->Client()->DidConnect(subprotocol, extensions);
}

void MainChannelClient::DidConnect(const String& subprotocol,
                                   const String& extensions) {
  DCHECK(IsMainThread());
  PostCrossThreadTask(*worker_networking_task_runner_, FROM_HERE,
                      CrossThreadBind(&WorkerGlobalScopeDidConnect, bridge_,
                                      subprotocol, extensions));
}

static void WorkerGlobalScopeDidReceiveTextMessage(Bridge* bridge,
                                                   const String& payload) {
  if (bridge && bridge->Client())
    bridge->Client()->DidReceiveTextMessage(payload);
}

void MainChannelClient::DidReceiveTextMessage(const String& payload) {
  DCHECK(IsMainThread());
  PostCrossThreadTask(*worker_networking_task_runner_, FROM_HERE,
                      CrossThreadBind(&WorkerGlobalScopeDidReceiveTextMessage,
                                      bridge_, payload));
}

static void WorkerGlobalScopeDidReceiveBinaryMessage(
    Bridge* bridge,
    std::unique_ptr<Vector<char>> payload) {
  if (bridge && bridge->Client())
    bridge->Client()->DidReceiveBinaryMessage(std::move(payload));
}

void MainChannelClient::DidReceiveBinaryMessage(
    std::unique_ptr<Vector<char>> payload) {
  DCHECK(IsMainThread());
  PostCrossThreadTask(
      *worker_networking_task_runner_, FROM_HERE,
      CrossThreadBind(&WorkerGlobalScopeDidReceiveBinaryMessage, bridge_,
                      WTF::Passed(std::move(payload))));
}

static void WorkerGlobalScopeDidConsumeBufferedAmount(Bridge* bridge,
                                                      uint64_t consumed) {
  if (bridge && bridge->Client())
    bridge->Client()->DidConsumeBufferedAmount(consumed);
}

void MainChannelClient::DidConsumeBufferedAmount(uint64_t consumed) {
  DCHECK(IsMainThread());
  PostCrossThreadTask(
      *worker_networking_task_runner_, FROM_HERE,
      CrossThreadBind(&WorkerGlobalScopeDidConsumeBufferedAmount, bridge_,
                      consumed));
}

static void WorkerGlobalScopeDidStartClosingHandshake(Bridge* bridge) {
  if (bridge && bridge->Client())
    bridge->Client()->DidStartClosingHandshake();
}

void MainChannelClient::DidStartClosingHandshake() {
  DCHECK(IsMainThread());
  PostCrossThreadTask(
      *worker_networking_task_runner_, FROM_HERE,
      CrossThreadBind(&WorkerGlobalScopeDidStartClosingHandshake, bridge_));
}

static void WorkerGlobalScopeDidClose(
    Bridge* bridge,
    WebSocketChannelClient::ClosingHandshakeCompletionStatus
        closing_handshake_completion,
    unsigned short code,
    const String& reason) {
  if (bridge && bridge->Client())
    bridge->Client()->DidClose(closing_handshake_completion, code, reason);
}

void MainChannelClient::DidClose(
    ClosingHandshakeCompletionStatus closing_handshake_completion,
    unsigned short code,
    const String& reason) {
  DCHECK(IsMainThread());

  ReleaseMainChannel();

  PostCrossThreadTask(
      *worker_networking_task_runner_, FROM_HERE,
      CrossThreadBind(&WorkerGlobalScopeDidClose, bridge_,
                      closing_handshake_completion, code, reason));
}

static void WorkerGlobalScopeDidError(Bridge* bridge) {
  if (bridge && bridge->Client())
    bridge->Client()->DidError();
}

void MainChannelClient::DidError() {
  DCHECK(IsMainThread());
  PostCrossThreadTask(*worker_networking_task_runner_, FROM_HERE,
                      CrossThreadBind(&WorkerGlobalScopeDidError, bridge_));
}

void MainChannelClient::ContextDestroyed(WorkerThreadLifecycleContext*) {
  DCHECK(IsMainThread());

  ReleaseMainChannel();

  bridge_ = nullptr;
}

void MainChannelClient::Trace(blink::Visitor* visitor) {
  visitor->Trace(main_channel_);
  WebSocketChannelClient::Trace(visitor);
  WorkerThreadLifecycleObserver::Trace(visitor);
}

Bridge::Bridge(WebSocketChannelClient* client,
               WorkerGlobalScope& worker_global_scope)
    : client_(client),
      worker_global_scope_(worker_global_scope),
      parent_execution_context_task_runners_(
          worker_global_scope_->GetThread()
              ->GetParentExecutionContextTaskRunners()) {}

Bridge::~Bridge() {
  DCHECK(!main_channel_client_);
}

void Bridge::ConnectOnMainThread(
    std::unique_ptr<SourceLocation> location,
    ThreadableLoadingContext* loading_context,
    scoped_refptr<base::SingleThreadTaskRunner> worker_networking_task_runner,
    WorkerThreadLifecycleContext* worker_thread_lifecycle_context,
    const KURL& url,
    const String& protocol,
    network::mojom::blink::WebSocketPtrInfo socket_ptr_info,
    WebSocketChannelSyncHelper* sync_helper) {
  DCHECK(IsMainThread());
  DCHECK(!main_channel_client_);
  MainChannelClient* main_channel_client =
      new MainChannelClient(this, std::move(worker_networking_task_runner),
                            worker_thread_lifecycle_context);
  if (main_channel_client->Initialize(std::move(location), loading_context)) {
    main_channel_client_ = main_channel_client;
    sync_helper->SetConnectRequestResult(main_channel_client_->Connect(
        url, protocol, mojo::MakeProxy(std::move(socket_ptr_info))));
  }
  sync_helper->SignalWorkerThread();
}

bool Bridge::Connect(std::unique_ptr<SourceLocation> location,
                     const KURL& url,
                     const String& protocol) {
  DCHECK(worker_global_scope_->IsContextThread());
  // Wait for completion of the task on the main thread because the mixed
  // content check must synchronously be conducted.
  WebSocketChannelSyncHelper sync_helper;
  scoped_refptr<base::SingleThreadTaskRunner> worker_networking_task_runner =
      worker_global_scope_->GetTaskRunner(TaskType::kNetworking);
  WorkerThread* worker_thread = worker_global_scope_->GetThread();

  // Dedicated workers are a special case as they always have an associated
  // document and so can make requests using that context. In the case of
  // https://crbug.com/760708 for example this is necessary to apply the user's
  // SSL interstitial decision to WebSocket connections from the worker.
  network::mojom::blink::WebSocketPtrInfo socket_ptr_info;
  if (!worker_global_scope_->IsDedicatedWorkerGlobalScope()) {
    worker_global_scope_->GetInterfaceProvider()->GetInterface(
        mojo::MakeRequest(&socket_ptr_info));
  }

  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(TaskType::kNetworking),
      FROM_HERE,
      CrossThreadBind(
          &Bridge::ConnectOnMainThread, WrapCrossThreadPersistent(this),
          WTF::Passed(location->Clone()),
          WrapCrossThreadPersistent(worker_thread->GetLoadingContext()),
          std::move(worker_networking_task_runner),
          WrapCrossThreadPersistent(
              worker_thread->GetWorkerThreadLifecycleContext()),
          url, protocol, WTF::Passed(std::move(socket_ptr_info)),
          CrossThreadUnretained(&sync_helper)));
  sync_helper.Wait();
  return sync_helper.ConnectRequestResult();
}

void Bridge::Send(const CString& message) {
  DCHECK(main_channel_client_);
  std::unique_ptr<Vector<char>> data =
      std::make_unique<Vector<char>>(message.length());
  if (message.length()) {
    memcpy(data->data(), static_cast<const char*>(message.data()),
           message.length());
  }

  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(TaskType::kNetworking),
      FROM_HERE,
      CrossThreadBind(&MainChannelClient::SendTextAsCharVector,
                      main_channel_client_, WTF::Passed(std::move(data))));
}

void Bridge::Send(const DOMArrayBuffer& binary_data,
                  unsigned byte_offset,
                  unsigned byte_length) {
  DCHECK(main_channel_client_);
  // ArrayBuffer isn't thread-safe, hence the content of ArrayBuffer is copied
  // into Vector<char>.
  std::unique_ptr<Vector<char>> data =
      std::make_unique<Vector<char>>(byte_length);
  if (binary_data.ByteLength()) {
    memcpy(data->data(),
           static_cast<const char*>(binary_data.Data()) + byte_offset,
           byte_length);
  }

  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(TaskType::kNetworking),
      FROM_HERE,
      CrossThreadBind(&MainChannelClient::SendBinaryAsCharVector,
                      main_channel_client_, WTF::Passed(std::move(data))));
}

void Bridge::Send(scoped_refptr<BlobDataHandle> data) {
  DCHECK(main_channel_client_);
  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(TaskType::kNetworking),
      FROM_HERE,
      CrossThreadBind(&MainChannelClient::SendBlob, main_channel_client_,
                      std::move(data)));
}

void Bridge::Close(int code, const String& reason) {
  DCHECK(main_channel_client_);
  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(TaskType::kNetworking),
      FROM_HERE,
      CrossThreadBind(&MainChannelClient::Close, main_channel_client_, code,
                      reason));
}

void Bridge::Fail(const String& reason,
                  MessageLevel level,
                  std::unique_ptr<SourceLocation> location) {
  DCHECK(main_channel_client_);
  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(TaskType::kNetworking),
      FROM_HERE,
      CrossThreadBind(&MainChannelClient::Fail, main_channel_client_, reason,
                      level, WTF::Passed(location->Clone())));
}

void Bridge::Disconnect() {
  if (!main_channel_client_)
    return;

  PostCrossThreadTask(
      *parent_execution_context_task_runners_->Get(TaskType::kNetworking),
      FROM_HERE,
      CrossThreadBind(&MainChannelClient::Disconnect, main_channel_client_));

  client_ = nullptr;
  main_channel_client_ = nullptr;
  worker_global_scope_.Clear();
}

void Bridge::Trace(blink::Visitor* visitor) {
  visitor->Trace(client_);
  visitor->Trace(worker_global_scope_);
}

}  // namespace blink
