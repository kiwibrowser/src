// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_LOADER_WEB_URL_LOADER_IMPL_H_
#define CONTENT_RENDERER_LOADER_WEB_URL_LOADER_IMPL_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/common/frame.mojom.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "third_party/blink/public/platform/web_url_loader.h"
#include "third_party/blink/public/platform/web_url_loader_factory.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace network {
struct ResourceResponseInfo;
}

namespace content {

class ResourceDispatcher;

// Default implementation of WebURLLoaderFactory.
class CONTENT_EXPORT WebURLLoaderFactoryImpl
    : public blink::WebURLLoaderFactory {
 public:
  WebURLLoaderFactoryImpl(
      base::WeakPtr<ResourceDispatcher> resource_dispatcher,
      scoped_refptr<network::SharedURLLoaderFactory> loader_factory);
  ~WebURLLoaderFactoryImpl() override;

  // Creates a test-only factory which can be used only for data URLs.
  static std::unique_ptr<WebURLLoaderFactoryImpl> CreateTestOnlyFactory();

  std::unique_ptr<blink::WebURLLoader> CreateURLLoader(
      const blink::WebURLRequest& request,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;

 private:
  base::WeakPtr<ResourceDispatcher> resource_dispatcher_;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_;
  DISALLOW_COPY_AND_ASSIGN(WebURLLoaderFactoryImpl);
};

class CONTENT_EXPORT WebURLLoaderImpl : public blink::WebURLLoader {
 public:
  WebURLLoaderImpl(
      ResourceDispatcher* resource_dispatcher,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  // When non-null |keep_alive_handle| is specified, this loader prolongs
  // this render process's lifetime.
  WebURLLoaderImpl(
      ResourceDispatcher* resource_dispatcher,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      mojom::KeepAliveHandlePtr keep_alive_handle);
  ~WebURLLoaderImpl() override;

  static void PopulateURLResponse(const blink::WebURL& url,
                                  const network::ResourceResponseInfo& info,
                                  blink::WebURLResponse* response,
                                  bool report_security_info);
  // WebURLLoader methods:
  void LoadSynchronously(const blink::WebURLRequest& request,
                         blink::WebURLLoaderClient* client,
                         blink::WebURLResponse& response,
                         base::Optional<blink::WebURLError>& error,
                         blink::WebData& data,
                         int64_t& encoded_data_length,
                         int64_t& encoded_body_length,
                         base::Optional<int64_t>& downloaded_file_length,
                         blink::WebBlobInfo& downloaded_blob) override;
  void LoadAsynchronously(const blink::WebURLRequest& request,
                          blink::WebURLLoaderClient* client) override;
  void Cancel() override;
  void SetDefersLoading(bool value) override;
  void DidChangePriority(blink::WebURLRequest::Priority new_priority,
                         int intra_priority_value) override;
 private:
  class Context;
  class RequestPeerImpl;
  class SinkPeer;
  scoped_refptr<Context> context_;

  DISALLOW_COPY_AND_ASSIGN(WebURLLoaderImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_LOADER_WEB_URL_LOADER_IMPL_H_
