/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBSOCKETS_WORKER_WEBSOCKET_CHANNEL_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBSOCKETS_WORKER_WEBSOCKET_CHANNEL_H_

#include <stdint.h>
#include <memory>
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "services/network/public/mojom/websocket.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/core/workers/parent_execution_context_task_runners.h"
#include "third_party/blink/renderer/core/workers/worker_thread_lifecycle_observer.h"
#include "third_party/blink/renderer/modules/websockets/websocket_channel.h"
#include "third_party/blink/renderer/modules/websockets/websocket_channel_client.h"
#include "third_party/blink/renderer/modules/websockets/websocket_channel_impl.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class BlobDataHandle;
class KURL;
class ThreadableLoadingContext;
class WebSocketChannelSyncHelper;
class WorkerGlobalScope;
class WorkerThreadLifecycleContext;

class WorkerWebSocketChannel final : public WebSocketChannel {
 public:
  static WebSocketChannel* Create(WorkerGlobalScope& worker_global_scope,
                                  WebSocketChannelClient* client,
                                  std::unique_ptr<SourceLocation> location) {
    return new WorkerWebSocketChannel(worker_global_scope, client,
                                      std::move(location));
  }
  ~WorkerWebSocketChannel() override;

  // WebSocketChannel functions.
  bool Connect(const KURL&, const String& protocol) override;
  void Send(const CString&) override;
  void Send(const DOMArrayBuffer&,
            unsigned byte_offset,
            unsigned byte_length) override;
  void Send(scoped_refptr<BlobDataHandle>) override;
  void SendTextAsCharVector(std::unique_ptr<Vector<char>>) override {
    NOTREACHED();
  }
  void SendBinaryAsCharVector(std::unique_ptr<Vector<char>>) override {
    NOTREACHED();
  }
  void Close(int code, const String& reason) override;
  void Fail(const String& reason,
            MessageLevel,
            std::unique_ptr<SourceLocation>) override;
  void Disconnect() override;  // Will suppress didClose().

  void Trace(blink::Visitor*) override;

  class Bridge;

  // A WebSocketChannelClient to pass to |main_channel_|. It forwards
  // method incovactions to the worker thread, and re-invokes them on the
  // WebSocketChannelClient given to the WorkerWebSocketChannel.
  //
  // Allocated and used in the main thread.
  class MainChannelClient final
      : public GarbageCollectedFinalized<MainChannelClient>,
        public WebSocketChannelClient,
        public WorkerThreadLifecycleObserver {
    USING_GARBAGE_COLLECTED_MIXIN(MainChannelClient);

   public:
    MainChannelClient(Bridge*,
                      scoped_refptr<base::SingleThreadTaskRunner>,
                      WorkerThreadLifecycleContext*);
    ~MainChannelClient() override;

    // SourceLocation parameter may be shown when the connection fails.
    bool Initialize(std::unique_ptr<SourceLocation>, ThreadableLoadingContext*);

    bool Connect(const KURL&,
                 const String& protocol,
                 network::mojom::blink::WebSocketPtr);
    void SendTextAsCharVector(std::unique_ptr<Vector<char>>);
    void SendBinaryAsCharVector(std::unique_ptr<Vector<char>>);
    void SendBlob(scoped_refptr<BlobDataHandle>);
    void Close(int code, const String& reason);
    void Fail(const String& reason,
              MessageLevel,
              std::unique_ptr<SourceLocation>);
    void Disconnect();

    void Trace(blink::Visitor*) override;
    // Promptly clear connection to bridge + loader proxy.
    EAGERLY_FINALIZE();

    // WebSocketChannelClient functions.
    void DidConnect(const String& subprotocol,
                    const String& extensions) override;
    void DidReceiveTextMessage(const String& payload) override;
    void DidReceiveBinaryMessage(std::unique_ptr<Vector<char>>) override;
    void DidConsumeBufferedAmount(uint64_t) override;
    void DidStartClosingHandshake() override;
    void DidClose(ClosingHandshakeCompletionStatus,
                  unsigned short code,
                  const String& reason) override;
    void DidError() override;

    // WorkerThreadLifecycleObserver function.
    void ContextDestroyed(WorkerThreadLifecycleContext*) override;

   private:
    void ReleaseMainChannel();

    CrossThreadWeakPersistent<Bridge> bridge_;
    scoped_refptr<base::SingleThreadTaskRunner> worker_networking_task_runner_;
    Member<WebSocketChannelImpl> main_channel_;

    DISALLOW_COPY_AND_ASSIGN(MainChannelClient);
  };

  // Bridge for MainChannelClient. Running on the worker thread.
  class Bridge final : public GarbageCollectedFinalized<Bridge> {
   public:
    Bridge(WebSocketChannelClient*, WorkerGlobalScope&);
    ~Bridge();

    // SourceLocation parameter may be shown when the connection fails.
    bool Connect(std::unique_ptr<SourceLocation>,
                 const KURL&,
                 const String& protocol);

    void Send(const CString& message);
    void Send(const DOMArrayBuffer&,
              unsigned byte_offset,
              unsigned byte_length);
    void Send(scoped_refptr<BlobDataHandle>);
    void Close(int code, const String& reason);
    void Fail(const String& reason,
              MessageLevel,
              std::unique_ptr<SourceLocation>);
    void Disconnect();

    void ConnectOnMainThread(std::unique_ptr<SourceLocation>,
                             ThreadableLoadingContext*,
                             scoped_refptr<base::SingleThreadTaskRunner>,
                             WorkerThreadLifecycleContext*,
                             const KURL&,
                             const String& protocol,
                             network::mojom::blink::WebSocketPtrInfo,
                             WebSocketChannelSyncHelper*);

    // Returns null when |disconnect| has already been called.
    WebSocketChannelClient* Client() { return client_; }

    void Trace(blink::Visitor*);
    // Promptly clear connection to peer + loader proxy.
    EAGERLY_FINALIZE();

   private:
    Member<WebSocketChannelClient> client_;
    Member<WorkerGlobalScope> worker_global_scope_;
    CrossThreadPersistent<ParentExecutionContextTaskRunners>
        parent_execution_context_task_runners_;
    CrossThreadPersistent<MainChannelClient> main_channel_client_;

    DISALLOW_COPY_AND_ASSIGN(Bridge);
  };

 private:
  WorkerWebSocketChannel(WorkerGlobalScope&,
                         WebSocketChannelClient*,
                         std::unique_ptr<SourceLocation>);

  Member<Bridge> bridge_;
  std::unique_ptr<SourceLocation> location_at_connection_;

  DISALLOW_COPY_AND_ASSIGN(WorkerWebSocketChannel);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBSOCKETS_WORKER_WEBSOCKET_CHANNEL_H_
