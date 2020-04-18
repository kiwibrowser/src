// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WEB_ASSOCIATED_URL_LOADER_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WEB_ASSOCIATED_URL_LOADER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/web/web_associated_url_loader.h"
#include "third_party/blink/public/web/web_associated_url_loader_options.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class DocumentThreadableLoader;
class WebAssociatedURLLoaderClient;
class Document;

// This class is used to implement WebFrame::createAssociatedURLLoader.
class CORE_EXPORT WebAssociatedURLLoaderImpl final
    : public WebAssociatedURLLoader {
 public:
  WebAssociatedURLLoaderImpl(Document*, const WebAssociatedURLLoaderOptions&);
  ~WebAssociatedURLLoaderImpl() override;

  void LoadAsynchronously(const WebURLRequest&,
                          WebAssociatedURLLoaderClient*) override;
  void Cancel() override;
  void SetDefersLoading(bool) override;
  void SetLoadingTaskRunner(base::SingleThreadTaskRunner*) override;

  // Called by |m_observer| to handle destruction of the Document associated
  // with the frame given to the constructor.
  void DocumentDestroyed();

  // Called by ClientAdapter to handle completion of loading.
  void ClientAdapterDone();

 private:
  class ClientAdapter;
  class Observer;

  void CancelLoader();
  void DisposeObserver();

  WebAssociatedURLLoaderClient* ReleaseClient() {
    WebAssociatedURLLoaderClient* client = client_;
    client_ = nullptr;
    return client;
  }

  WebAssociatedURLLoaderClient* client_;
  WebAssociatedURLLoaderOptions options_;

  // An adapter which converts the DocumentThreadableLoaderClient method
  // calls into the WebURLLoaderClient method calls.
  std::unique_ptr<ClientAdapter> client_adapter_;
  Persistent<DocumentThreadableLoader> loader_;

  // A ContextLifecycleObserver for cancelling |m_loader| when the Document
  // is detached.
  Persistent<Observer> observer_;

  DISALLOW_COPY_AND_ASSIGN(WebAssociatedURLLoaderImpl);
};

}  // namespace blink

#endif
