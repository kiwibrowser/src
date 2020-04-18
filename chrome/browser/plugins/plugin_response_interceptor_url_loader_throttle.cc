// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/plugin_response_interceptor_url_loader_throttle.h"

#include "base/guid.h"
#include "chrome/browser/extensions/api/streams_private/streams_private_api.h"
#include "chrome/browser/plugins/plugin_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/stream_info.h"
#include "content/public/common/resource_type.h"
#include "content/public/common/transferrable_url_loader.mojom.h"
#include "mojo/public/cpp/system/data_pipe.h"

PluginResponseInterceptorURLLoaderThrottle::
    PluginResponseInterceptorURLLoaderThrottle(
        content::ResourceContext* resource_context,
        int resource_type,
        int frame_tree_node_id)
    : resource_context_(resource_context),
      resource_type_(resource_type),
      frame_tree_node_id_(frame_tree_node_id) {}

PluginResponseInterceptorURLLoaderThrottle::
    ~PluginResponseInterceptorURLLoaderThrottle() = default;

void PluginResponseInterceptorURLLoaderThrottle::WillProcessResponse(
    const GURL& response_url,
    const network::ResourceResponseHead& response_head,
    bool* defer) {
  std::string extension_id = PluginUtils::GetExtensionIdForMimeType(
      resource_context_, response_head.mime_type);
  if (extension_id.empty())
    return;

  std::string view_id = base::GenerateGUID();

  network::mojom::URLLoaderPtr dummy_new_loader;
  mojo::MakeRequest(&dummy_new_loader);
  network::mojom::URLLoaderClientPtr new_client;
  network::mojom::URLLoaderClientRequest new_client_request =
      mojo::MakeRequest(&new_client);

  mojo::DataPipe data_pipe(64);
  uint32_t len = static_cast<uint32_t>(view_id.size());
  CHECK_EQ(MOJO_RESULT_OK,
           data_pipe.producer_handle->WriteData(
               view_id.c_str(), &len, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));

  new_client->OnStartLoadingResponseBody(std::move(data_pipe.consumer_handle));

  network::URLLoaderCompletionStatus status(net::OK);
  status.decoded_body_length = len;
  new_client->OnComplete(status);

  network::mojom::URLLoaderPtr original_loader;
  network::mojom::URLLoaderClientRequest original_client;
  delegate_->InterceptResponse(std::move(dummy_new_loader),
                               std::move(new_client_request), &original_loader,
                               &original_client);

  // Make a deep copy of ResourceResponseHead before passing it cross-thread.
  auto resource_response = base::MakeRefCounted<network::ResourceResponse>();
  resource_response->head = response_head;
  auto deep_copied_response = resource_response->DeepCopy();

  auto transferrable_loader = content::mojom::TransferrableURLLoader::New();
  transferrable_loader->url = GURL(
      extensions::Extension::GetBaseURLFromExtensionId(extension_id).spec() +
      base::GenerateGUID());
  transferrable_loader->url_loader = original_loader.PassInterface();
  transferrable_loader->url_loader_client = std::move(original_client);
  transferrable_loader->head = std::move(deep_copied_response->head);

  int64_t expected_content_size = response_head.content_length;
  bool embedded = resource_type_ != content::RESOURCE_TYPE_MAIN_FRAME;
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &extensions::StreamsPrivateAPI::SendExecuteMimeTypeHandlerEvent,
          expected_content_size, extension_id, view_id, embedded,
          frame_tree_node_id_, -1 /* render_process_id */,
          -1 /* render_frame_id */, nullptr /* stream */,
          std::move(transferrable_loader), response_url));
}
