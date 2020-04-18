// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_COMMON_IN_PROGRESS_DOWNLOAD_MANAGER_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_COMMON_IN_PROGRESS_DOWNLOAD_MANAGER_H_

#include <memory>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "components/download/public/common/download_export.h"
#include "components/download/public/common/download_file_factory.h"
#include "components/download/public/common/download_item_impl_delegate.h"
#include "components/download/public/common/url_download_handler.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace network {
struct ResourceResponse;
}

namespace download {

class DownloadURLLoaderFactoryGetter;
class DownloadUrlParameters;
class InProgressCache;

// Manager for handling all active downloads.
class COMPONENTS_DOWNLOAD_EXPORT InProgressDownloadManager
    : public UrlDownloadHandler::Delegate,
      public DownloadItemImplDelegate {
 public:
  using DownloadIdCallback = base::RepeatingCallback<void(uint32_t)>;

  // Class to be notified when download starts/stops.
  class COMPONENTS_DOWNLOAD_EXPORT Delegate {
   public:
    // Intercepts the download to another system if applicable. Returns true if
    // the download was intercepted.
    virtual bool InterceptDownload(
        const DownloadCreateInfo& download_create_info) = 0;

    // Called to get an ID for a new download. |callback| may be called
    // synchronously.
    virtual void GetNextId(const DownloadIdCallback& callback) = 0;

    // Gets the default download directory.
    virtual base::FilePath GetDefaultDownloadDirectory() = 0;

    // Gets the download item for the given id.
    // TODO(qinmin): remove this method and let InProgressDownloadManager
    // create the DownloadItem from in-progress cache.
    virtual DownloadItemImpl* GetDownloadItem(
        uint32_t id,
        bool new_download,
        const DownloadCreateInfo& download_create_info) = 0;

    // Gets the URLRequestContextGetter for sending requests.
    // TODO(qinmin): remove this once network service is fully enabled.
    virtual net::URLRequestContextGetter* GetURLRequestContextGetter(
        const DownloadCreateInfo& download_create_info) = 0;

    // Called when a new download is started.
    virtual void OnNewDownloadStarted(DownloadItem* download) = 0;
  };

  using IsOriginSecureCallback = base::RepeatingCallback<bool(const GURL&)>;
  InProgressDownloadManager(Delegate* delegate,
                            const IsOriginSecureCallback& is_origin_secure_cb);
  ~InProgressDownloadManager() override;
  // Called to start a download.
  void BeginDownload(
      std::unique_ptr<DownloadUrlParameters> params,
      scoped_refptr<DownloadURLLoaderFactoryGetter> url_loader_factory_getter,
      uint32_t download_id,
      const GURL& site_url,
      const GURL& tab_url,
      const GURL& tab_referrer_url);

  // Intercepts a download from navigation.
  void InterceptDownloadFromNavigation(
      std::unique_ptr<network::ResourceRequest> resource_request,
      int render_process_id,
      int render_frame_id,
      const GURL& site_url,
      const GURL& tab_url,
      const GURL& tab_referrer_url,
      std::vector<GURL> url_chain,
      scoped_refptr<network::ResourceResponse> response,
      net::CertStatus cert_status,
      network::mojom::URLLoaderClientEndpointsPtr url_loader_client_endpoints,
      scoped_refptr<DownloadURLLoaderFactoryGetter> url_loader_factory_getter);

  void Initialize(const base::FilePath& metadata_cache_dir,
                  base::OnceClosure callback);

  void StartDownload(
      std::unique_ptr<DownloadCreateInfo> info,
      std::unique_ptr<InputStream> stream,
      scoped_refptr<DownloadURLLoaderFactoryGetter> url_loader_factory_getter,
      const DownloadUrlParameters::OnStartedCallback& on_started);

  // Shutting down the manager and stop all downloads.
  void ShutDown();

  // DownloadItemImplDelegate implementations.
  void ResumeInterruptedDownload(std::unique_ptr<DownloadUrlParameters> params,
                                 uint32_t id,
                                 const GURL& site_url) override;
  base::Optional<DownloadEntry> GetInProgressEntry(
      DownloadItemImpl* download) override;
  void ReportBytesWasted(DownloadItemImpl* download) override;

  void set_file_factory(std::unique_ptr<DownloadFileFactory> file_factory) {
    file_factory_ = std::move(file_factory);
  }
  DownloadFileFactory* file_factory() { return file_factory_.get(); }

 private:
  // UrlDownloadHandler::Delegate implementations.
  void OnUrlDownloadStarted(
      std::unique_ptr<DownloadCreateInfo> download_create_info,
      std::unique_ptr<InputStream> input_stream,
      scoped_refptr<DownloadURLLoaderFactoryGetter> shared_url_loader_factory,
      const DownloadUrlParameters::OnStartedCallback& callback) override;
  void OnUrlDownloadStopped(UrlDownloadHandler* downloader) override;
  void OnUrlDownloadHandlerCreated(
      UrlDownloadHandler::UniqueUrlDownloadHandlerPtr downloader) override;

  // Start a download with given ID.
  void StartDownloadWithId(
      std::unique_ptr<DownloadCreateInfo> info,
      std::unique_ptr<InputStream> stream,
      scoped_refptr<DownloadURLLoaderFactoryGetter> url_loader_factory_getter,
      const DownloadUrlParameters::OnStartedCallback& on_started,
      bool new_download,
      uint32_t id);

  // Active download handlers.
  std::vector<UrlDownloadHandler::UniqueUrlDownloadHandlerPtr>
      url_download_handlers_;

  // Delegate to provide information to create a new download. Can be null.
  Delegate* delegate_;

  // Factory for the creation of download files.
  std::unique_ptr<DownloadFileFactory> file_factory_;

  // Cache for storing metadata about in progress downloads.
  std::unique_ptr<InProgressCache> download_metadata_cache_;

  // listens to information about in-progress download items.
  std::unique_ptr<DownloadItem::Observer> in_progress_download_observer_;

  // callback to check if an origin is secure.
  IsOriginSecureCallback is_origin_secure_cb_;

  base::WeakPtrFactory<InProgressDownloadManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InProgressDownloadManager);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_COMMON_IN_PROGRESS_DOWNLOAD_MANAGER_H_
