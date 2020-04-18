// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/chrome_url_request_util.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/chrome_manifest_url_handlers.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/component_extension_resource_manager.h"
#include "extensions/browser/extension_protocols.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/url_request_util.h"
#include "extensions/common/file_util.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/file_data_pipe_producer.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_simple_job.h"
#include "ui/base/resource/resource_bundle.h"

using extensions::ExtensionsBrowserClient;

namespace {

void DetermineCharset(const std::string& mime_type,
                      const base::RefCountedMemory* data,
                      std::string* out_charset) {
  if (base::StartsWith(mime_type, "text/",
                       base::CompareCase::INSENSITIVE_ASCII)) {
    // All of our HTML files should be UTF-8 and for other resource types
    // (like images), charset doesn't matter.
    DCHECK(base::IsStringUTF8(base::StringPiece(
        reinterpret_cast<const char*>(data->front()), data->size())));
    *out_charset = "utf-8";
  }
}

// A request for an extension resource in a Chrome .pak file. These are used
// by component extensions.
class URLRequestResourceBundleJob : public net::URLRequestSimpleJob {
 public:
  URLRequestResourceBundleJob(net::URLRequest* request,
                              net::NetworkDelegate* network_delegate,
                              const base::FilePath& filename,
                              int resource_id,
                              const std::string& content_security_policy,
                              bool send_cors_header)
      : net::URLRequestSimpleJob(request, network_delegate),
        filename_(filename),
        resource_id_(resource_id),
        weak_factory_(this) {
    // Leave cache headers out of resource bundle requests.
    response_info_.headers = extensions::BuildHttpHeaders(
        content_security_policy, send_cors_header, base::Time());
  }

  // Overridden from URLRequestSimpleJob:
  int GetRefCountedData(
      std::string* mime_type,
      std::string* charset,
      scoped_refptr<base::RefCountedMemory>* data,
      const net::CompletionCallback& callback) const override {
    const ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    *data = rb.LoadDataResourceBytes(resource_id_);

    // Add the Content-Length header now that we know the resource length.
    response_info_.headers->AddHeader(
        base::StringPrintf("%s: %s", net::HttpRequestHeaders::kContentLength,
                           base::NumberToString((*data)->size()).c_str()));

    std::string* read_mime_type = new std::string;
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::Bind(&net::GetMimeTypeFromFile, filename_,
                   base::Unretained(read_mime_type)),
        base::Bind(&URLRequestResourceBundleJob::OnMimeTypeRead,
                   weak_factory_.GetWeakPtr(), mime_type, charset, *data,
                   base::Owned(read_mime_type), callback));

    return net::ERR_IO_PENDING;
  }

  void GetResponseInfo(net::HttpResponseInfo* info) override {
    *info = response_info_;
  }

 private:
  ~URLRequestResourceBundleJob() override {}

  void OnMimeTypeRead(std::string* out_mime_type,
                      std::string* charset,
                      scoped_refptr<base::RefCountedMemory> data,
                      std::string* read_mime_type,
                      const net::CompletionCallback& callback,
                      bool read_result) {
    response_info_.headers->AddHeader(
        base::StringPrintf("%s: %s", net::HttpRequestHeaders::kContentType,
                           read_mime_type->c_str()));
    *out_mime_type = *read_mime_type;
    DetermineCharset(*read_mime_type, data.get(), charset);
    int result = read_result ? net::OK : net::ERR_INVALID_URL;
    callback.Run(result);
  }

  // We need the filename of the resource to determine the mime type.
  base::FilePath filename_;

  // The resource bundle id to load.
  int resource_id_;

  net::HttpResponseInfo response_info_;

  mutable base::WeakPtrFactory<URLRequestResourceBundleJob> weak_factory_;
};

// Loads an extension resource in a Chrome .pak file. These are used by
// component extensions.
class ResourceBundleFileLoader : public network::mojom::URLLoader {
 public:
  static void CreateAndStart(const network::ResourceRequest& request,
                             network::mojom::URLLoaderRequest loader,
                             network::mojom::URLLoaderClientPtrInfo client_info,
                             const base::FilePath& filename,
                             int resource_id,
                             const std::string& content_security_policy,
                             bool send_cors_header) {
    // Owns itself. Will live as long as its URLLoader and URLLoaderClientPtr
    // bindings are alive - essentially until either the client gives up or all
    // file data has been sent to it.
    auto* bundle_loader =
        new ResourceBundleFileLoader(content_security_policy, send_cors_header);
    bundle_loader->Start(request, std::move(loader), std::move(client_info),
                         filename, resource_id);
  }

  // mojom::URLLoader implementation:
  void FollowRedirect(const base::Optional<net::HttpRequestHeaders>&
                          modified_request_headers) override {
    NOTREACHED() << "No redirects for local file loads.";
  }
  // Current implementation reads all resource data at start of resource
  // load, so priority, and pausing is not currently implemented.
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}
  void ProceedWithResponse() override {}

 private:
  ResourceBundleFileLoader(const std::string& content_security_policy,
                           bool send_cors_header)
      : binding_(this), weak_factory_(this) {
    response_headers_ = extensions::BuildHttpHeaders(
        content_security_policy, send_cors_header, base::Time());
  }
  ~ResourceBundleFileLoader() override = default;

  void Start(const network::ResourceRequest& request,
             network::mojom::URLLoaderRequest loader,
             network::mojom::URLLoaderClientPtrInfo client_info,
             const base::FilePath& filename,
             int resource_id) {
    client_.Bind(std::move(client_info));
    binding_.Bind(std::move(loader));
    binding_.set_connection_error_handler(base::BindOnce(
        &ResourceBundleFileLoader::OnBindingError, base::Unretained(this)));
    client_.set_connection_error_handler(base::BindOnce(
        &ResourceBundleFileLoader::OnConnectionError, base::Unretained(this)));
    const ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    scoped_refptr<base::RefCountedMemory> data =
        rb.LoadDataResourceBytes(resource_id);

    std::string* read_mime_type = new std::string;
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&net::GetMimeTypeFromFile, filename,
                       base::Unretained(read_mime_type)),
        base::BindOnce(&ResourceBundleFileLoader::OnMimeTypeRead,
                       weak_factory_.GetWeakPtr(), std::move(data),
                       base::Owned(read_mime_type)));
  }

  void OnMimeTypeRead(scoped_refptr<base::RefCountedMemory> data,
                      std::string* read_mime_type,
                      bool read_result) {
    if (!read_result) {
      client_->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
      return;
    }
    network::ResourceResponseHead head;
    head.request_start = base::TimeTicks::Now();
    head.response_start = base::TimeTicks::Now();
    head.content_length = data->size();
    head.mime_type = *read_mime_type;
    DetermineCharset(head.mime_type, data.get(), &head.charset);
    mojo::DataPipe pipe(data->size());
    if (!pipe.consumer_handle.is_valid()) {
      client_->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
      client_.reset();
      MaybeDeleteSelf();
      return;
    }
    head.headers = response_headers_;
    head.headers->AddHeader(
        base::StringPrintf("%s: %s", net::HttpRequestHeaders::kContentLength,
                           base::NumberToString(head.content_length).c_str()));
    if (!head.mime_type.empty()) {
      head.headers->AddHeader(
          base::StringPrintf("%s: %s", net::HttpRequestHeaders::kContentType,
                             head.mime_type.c_str()));
    }
    client_->OnReceiveResponse(head, nullptr);
    client_->OnStartLoadingResponseBody(std::move(pipe.consumer_handle));

    uint32_t write_size = data->size();
    MojoResult result = pipe.producer_handle->WriteData(
        data->front(), &write_size, MOJO_WRITE_DATA_FLAG_NONE);
    OnFileWritten(result);
  }

  void OnConnectionError() {
    client_.reset();
    MaybeDeleteSelf();
  }

  void OnBindingError() {
    binding_.Close();
    MaybeDeleteSelf();
  }

  void MaybeDeleteSelf() {
    if (!binding_.is_bound() && !client_.is_bound())
      delete this;
  }

  void OnFileWritten(MojoResult result) {
    // All the data has been written now. The consumer will be notified that
    // there will be no more data to read from now.
    if (result == MOJO_RESULT_OK)
      client_->OnComplete(network::URLLoaderCompletionStatus(net::OK));
    else
      client_->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    client_.reset();
    MaybeDeleteSelf();
  }

  mojo::Binding<network::mojom::URLLoader> binding_;
  network::mojom::URLLoaderClientPtr client_;
  scoped_refptr<net::HttpResponseHeaders> response_headers_;
  base::WeakPtrFactory<ResourceBundleFileLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ResourceBundleFileLoader);
};

}  // namespace

namespace extensions {
namespace chrome_url_request_util {

bool AllowCrossRendererResourceLoad(const GURL& url,
                                    content::ResourceType resource_type,
                                    ui::PageTransition page_transition,
                                    int child_id,
                                    bool is_incognito,
                                    const Extension* extension,
                                    const ExtensionSet& extensions,
                                    const ProcessMap& process_map,
                                    bool* allowed) {
  if (url_request_util::AllowCrossRendererResourceLoad(
          url, resource_type, page_transition, child_id, is_incognito,
          extension, extensions, process_map, allowed)) {
    return true;
  }

  // If there aren't any explicitly marked web accessible resources, the
  // load should be allowed only if it is by DevTools. A close approximation is
  // checking if the extension contains a DevTools page.
  if (extension &&
      !chrome_manifest_urls::GetDevToolsPage(extension).is_empty()) {
    *allowed = true;
    return true;
  }

  // Couldn't determine if the resource is allowed or not.
  return false;
}

net::URLRequestJob* MaybeCreateURLRequestResourceBundleJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const base::FilePath& directory_path,
    const std::string& content_security_policy,
    bool send_cors_header) {
  base::FilePath resources_path;
  base::FilePath relative_path;
  // Try to load extension resources from chrome resource file if
  // directory_path is a descendant of resources_path. resources_path
  // corresponds to src/chrome/browser/resources in source tree.
  if (base::PathService::Get(chrome::DIR_RESOURCES, &resources_path) &&
      // Since component extension resources are included in
      // component_extension_resources.pak file in resources_path, calculate
      // extension relative path against resources_path.
      resources_path.AppendRelativePath(directory_path, &relative_path)) {
    base::FilePath request_path =
        extensions::file_util::ExtensionURLToRelativeFilePath(request->url());
    int resource_id = 0;
    if (ExtensionsBrowserClient::Get()
            ->GetComponentExtensionResourceManager()
            ->IsComponentExtensionResource(
                directory_path, request_path, &resource_id)) {
      relative_path = relative_path.Append(request_path);
      relative_path = relative_path.NormalizePathSeparators();
      return new URLRequestResourceBundleJob(request,
                                             network_delegate,
                                             relative_path,
                                             resource_id,
                                             content_security_policy,
                                             send_cors_header);
    }
  }
  return NULL;
}

base::FilePath GetBundleResourcePath(
    const network::ResourceRequest& request,
    const base::FilePath& extension_resources_path,
    int* resource_id) {
  *resource_id = 0;
  // |chrome_resources_path| corresponds to src/chrome/browser/resources in
  // source tree.
  base::FilePath chrome_resources_path;
  if (!base::PathService::Get(chrome::DIR_RESOURCES, &chrome_resources_path))
    return base::FilePath();

  // Since component extension resources are included in
  // component_extension_resources.pak file in |chrome_resources_path|,
  // calculate the extension |request_relative_path| against
  // |chrome_resources_path|.
  if (!chrome_resources_path.IsParent(extension_resources_path))
    return base::FilePath();

  const base::FilePath request_relative_path =
      extensions::file_util::ExtensionURLToRelativeFilePath(request.url);
  if (!ExtensionsBrowserClient::Get()
           ->GetComponentExtensionResourceManager()
           ->IsComponentExtensionResource(extension_resources_path,
                                          request_relative_path, resource_id)) {
    return base::FilePath();
  }
  DCHECK_NE(0, *resource_id);

  return request_relative_path;
}

void LoadResourceFromResourceBundle(
    const network::ResourceRequest& request,
    network::mojom::URLLoaderRequest loader,
    const base::FilePath& resource_relative_path,
    int resource_id,
    const std::string& content_security_policy,
    network::mojom::URLLoaderClientPtr client,
    bool send_cors_header) {
  DCHECK(!resource_relative_path.empty());
  ResourceBundleFileLoader::CreateAndStart(
      request, std::move(loader), client.PassInterface(),
      resource_relative_path, resource_id, content_security_policy,
      send_cors_header);
}

}  // namespace chrome_url_request_util
}  // namespace extensions
