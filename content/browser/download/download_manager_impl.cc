// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_manager_impl.h"

#include <iterator>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/debug/alias.h"
#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/supports_user_data.h"
#include "base/synchronization/lock.h"
#include "build/build_config.h"
#include "components/download/downloader/in_progress/download_entry.h"
#include "components/download/downloader/in_progress/in_progress_cache_impl.h"
#include "components/download/public/common/download_create_info.h"
#include "components/download/public/common/download_file.h"
#include "components/download/public/common/download_interrupt_reasons.h"
#include "components/download/public/common/download_item_factory.h"
#include "components/download/public/common/download_item_impl.h"
#include "components/download/public/common/download_request_handle_interface.h"
#include "components/download/public/common/download_stats.h"
#include "components/download/public/common/download_task_runner.h"
#include "components/download/public/common/download_url_loader_factory_getter.h"
#include "components/download/public/common/download_url_parameters.h"
#include "components/download/public/common/download_utils.h"
#include "components/download/public/common/url_download_handler_factory.h"
#include "content/browser/byte_stream.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/devtools/render_frame_devtools_agent_host.h"
#include "content/browser/download/blob_download_url_loader_factory_getter.h"
#include "content/browser/download/byte_stream_input_stream.h"
#include "content/browser/download/download_resource_handler.h"
#include "content/browser/download/download_url_loader_factory_getter_impl.h"
#include "content/browser/download/download_utils.h"
#include "content/browser/download/network_download_url_loader_factory_getter.h"
#include "content/browser/download/url_downloader.h"
#include "content/browser/download/url_downloader_factory.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/storage_partition_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/throttling_url_loader.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/download_manager_delegate.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/previews_state.h"
#include "content/public/common/referrer.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/load_flags.h"
#include "net/base/request_priority.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/url_request/url_request_context.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "services/network/public/cpp/features.h"
#include "storage/browser/blob/blob_url_loader_factory.h"
#include "storage/browser/blob/blob_url_request_job_factory.h"
#include "url/origin.h"

#if defined(USE_X11)
#include "base/nix/xdg_util.h"
#endif

namespace content {
namespace {

StoragePartitionImpl* GetStoragePartition(BrowserContext* context,
                                          int render_process_id,
                                          int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  SiteInstance* site_instance = nullptr;
  if (render_process_id >= 0) {
    RenderFrameHost* render_frame_host_ =
        RenderFrameHost::FromID(render_process_id, render_frame_id);
    if (render_frame_host_)
      site_instance = render_frame_host_->GetSiteInstance();
  }
  return static_cast<StoragePartitionImpl*>(
      BrowserContext::GetStoragePartition(context, site_instance));
}

bool CanRequestURLFromRenderer(int render_process_id, GURL url) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Check if the renderer is permitted to request the requested URL.
  if (!ChildProcessSecurityPolicyImpl::GetInstance()->CanRequestURL(
          render_process_id, url)) {
    DVLOG(1) << "Denied unauthorized download request for "
             << url.possibly_invalid_spec();
    return false;
  }
  return true;
}

// Creates an interrupted download and calls StartDownload. Can be called on
// any thread.
void CreateInterruptedDownload(
    std::unique_ptr<download::DownloadUrlParameters> params,
    download::DownloadInterruptReason reason,
    base::WeakPtr<DownloadManagerImpl> download_manager) {
  std::unique_ptr<download::DownloadCreateInfo> failed_created_info(
      new download::DownloadCreateInfo(
          base::Time::Now(), base::WrapUnique(new download::DownloadSaveInfo)));
  failed_created_info->url_chain.push_back(params->url());
  failed_created_info->result = reason;
  std::unique_ptr<ByteStreamReader> empty_byte_stream;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &DownloadManager::StartDownload, download_manager,
          std::move(failed_created_info),
          std::make_unique<ByteStreamInputStream>(std::move(empty_byte_stream)),
          nullptr, params->callback()));
}

void BeginDownload(std::unique_ptr<download::DownloadUrlParameters> params,
                   std::unique_ptr<storage::BlobDataHandle> blob_data_handle,
                   content::ResourceContext* resource_context,
                   uint32_t download_id,
                   base::WeakPtr<DownloadManagerImpl> download_manager) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  download::UrlDownloadHandler::UniqueUrlDownloadHandlerPtr downloader(
      nullptr, base::OnTaskRunnerDeleter(base::ThreadTaskRunnerHandle::Get()));

  params->set_blob_storage_context_getter(
      base::BindOnce(&BlobStorageContextGetter, resource_context));
  std::unique_ptr<net::URLRequest> url_request =
      DownloadRequestCore::CreateRequestOnIOThread(download_id, params.get());
  if (blob_data_handle) {
    storage::BlobProtocolHandler::SetRequestedBlobDataHandle(
        url_request.get(), std::move(blob_data_handle));
  }

  // If there's a valid renderer process associated with the request, then the
  // request should be driven by the ResourceLoader. Pass it over to the
  // ResourceDispatcherHostImpl which will in turn pass it along to the
  // ResourceLoader.
  if (params->render_process_host_id() >= 0) {
    download::DownloadInterruptReason reason =
        DownloadManagerImpl::BeginDownloadRequest(
            std::move(url_request), resource_context, params.get());

    // If the download was accepted, the DownloadResourceHandler is now
    // responsible for driving the request to completion.
    // Otherwise, create an interrupted download.
    if (reason != download::DOWNLOAD_INTERRUPT_REASON_NONE) {
      CreateInterruptedDownload(std::move(params), reason, download_manager);
      return;
    }
  } else {
    downloader.reset(UrlDownloader::BeginDownload(download_manager,
                                                  std::move(url_request),
                                                  params.get(), false)
                         .release());
  }
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &download::UrlDownloadHandler::Delegate::OnUrlDownloadHandlerCreated,
          download_manager, std::move(downloader)));
}

class DownloadItemFactoryImpl : public download::DownloadItemFactory {
 public:
  DownloadItemFactoryImpl() {}
  ~DownloadItemFactoryImpl() override {}

  download::DownloadItemImpl* CreatePersistedItem(
      download::DownloadItemImplDelegate* delegate,
      const std::string& guid,
      uint32_t download_id,
      const base::FilePath& current_path,
      const base::FilePath& target_path,
      const std::vector<GURL>& url_chain,
      const GURL& referrer_url,
      const GURL& site_url,
      const GURL& tab_url,
      const GURL& tab_refererr_url,
      const std::string& mime_type,
      const std::string& original_mime_type,
      base::Time start_time,
      base::Time end_time,
      const std::string& etag,
      const std::string& last_modified,
      int64_t received_bytes,
      int64_t total_bytes,
      const std::string& hash,
      download::DownloadItem::DownloadState state,
      download::DownloadDangerType danger_type,
      download::DownloadInterruptReason interrupt_reason,
      bool opened,
      base::Time last_access_time,
      bool transient,
      const std::vector<download::DownloadItem::ReceivedSlice>& received_slices)
      override {
    return new download::DownloadItemImpl(
        delegate, guid, download_id, current_path, target_path, url_chain,
        referrer_url, site_url, tab_url, tab_refererr_url, mime_type,
        original_mime_type, start_time, end_time, etag, last_modified,
        received_bytes, total_bytes, hash, state, danger_type, interrupt_reason,
        opened, last_access_time, transient, received_slices);
  }

  download::DownloadItemImpl* CreateActiveItem(
      download::DownloadItemImplDelegate* delegate,
      uint32_t download_id,
      const download::DownloadCreateInfo& info) override {
    return new download::DownloadItemImpl(delegate, download_id, info);
  }

  download::DownloadItemImpl* CreateSavePageItem(
      download::DownloadItemImplDelegate* delegate,
      uint32_t download_id,
      const base::FilePath& path,
      const GURL& url,
      const std::string& mime_type,
      std::unique_ptr<download::DownloadRequestHandleInterface> request_handle)
      override {
    return new download::DownloadItemImpl(delegate, download_id, path, url,
                                          mime_type, std::move(request_handle));
  }
};

#if defined(USE_X11)
base::FilePath GetTemporaryDownloadDirectory() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  return base::nix::GetXDGDirectory(env.get(), "XDG_DATA_HOME", ".local/share");
}
#endif

scoped_refptr<download::DownloadURLLoaderFactoryGetter>
CreateDownloadURLLoaderFactoryGetter(StoragePartitionImpl* storage_partition,
                                     RenderFrameHost* rfh,
                                     bool is_download) {
  network::mojom::URLLoaderFactoryPtrInfo proxy_factory_ptr_info;
  network::mojom::URLLoaderFactoryRequest proxy_factory_request;
  if (rfh) {
    network::mojom::URLLoaderFactoryPtrInfo devtools_factory_ptr_info;
    network::mojom::URLLoaderFactoryRequest devtools_factory_request =
        MakeRequest(&devtools_factory_ptr_info);
    if (RenderFrameDevToolsAgentHost::WillCreateURLLoaderFactory(
            static_cast<RenderFrameHostImpl*>(rfh), true, is_download,
            &devtools_factory_request)) {
      proxy_factory_ptr_info = std::move(devtools_factory_ptr_info);
      proxy_factory_request = std::move(devtools_factory_request);
    }
  }
  return base::MakeRefCounted<NetworkDownloadURLLoaderFactoryGetter>(
      storage_partition->url_loader_factory_getter(),
      std::move(proxy_factory_ptr_info), std::move(proxy_factory_request));
}

}  // namespace

DownloadManagerImpl::DownloadManagerImpl(BrowserContext* browser_context)
    : item_factory_(new DownloadItemFactoryImpl()),
      shutdown_needed_(true),
      initialized_(false),
      history_db_initialized_(false),
      in_progress_cache_initialized_(false),
      browser_context_(browser_context),
      delegate_(nullptr),
      weak_factory_(this) {
  DCHECK(browser_context);
  download::SetIOTaskRunner(
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService))
    download::UrlDownloadHandlerFactory::Install(new UrlDownloaderFactory());
  in_progress_manager_ = std::make_unique<download::InProgressDownloadManager>(
      this, base::BindRepeating(&IsOriginSecure));
  in_progress_manager_->Initialize(
      IsOffTheRecord() ? base::FilePath() : browser_context_->GetPath(),
      base::BindOnce(&DownloadManagerImpl::PostInitialization,
                     weak_factory_.GetWeakPtr(),
                     DOWNLOAD_INITIALIZATION_DEPENDENCY_IN_PROGRESS_CACHE));
}

DownloadManagerImpl::~DownloadManagerImpl() {
  DCHECK(!shutdown_needed_);
  download::SetIOTaskRunner(nullptr);
}

download::DownloadItemImpl* DownloadManagerImpl::CreateActiveItem(
    uint32_t id,
    const download::DownloadCreateInfo& info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!base::ContainsKey(downloads_, id));
  download::DownloadItemImpl* download =
      item_factory_->CreateActiveItem(this, id, info);

  downloads_[id] = base::WrapUnique(download);
  downloads_by_guid_[download->GetGuid()] = download;
  return download;
}

void DownloadManagerImpl::GetNextId(const DownloadIdCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (delegate_) {
    delegate_->GetNextId(callback);
    return;
  }
  static uint32_t next_id = download::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

void DownloadManagerImpl::DetermineDownloadTarget(
    download::DownloadItemImpl* item,
    const DownloadTargetCallback& callback) {
  // Note that this next call relies on
  // DownloadItemImplDelegate::DownloadTargetCallback and
  // DownloadManagerDelegate::DownloadTargetCallback having the same
  // type.  If the types ever diverge, gasket code will need to
  // be written here.
  if (!delegate_ || !delegate_->DetermineDownloadTarget(item, callback)) {
    base::FilePath target_path = item->GetForcedFilePath();
    // TODO(asanka): Determine a useful path if |target_path| is empty.
    callback.Run(target_path,
                 download::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, target_path,
                 download::DOWNLOAD_INTERRUPT_REASON_NONE);
  }
}

bool DownloadManagerImpl::ShouldCompleteDownload(
    download::DownloadItemImpl* item,
    const base::Closure& complete_callback) {
  if (!delegate_ ||
      delegate_->ShouldCompleteDownload(item, complete_callback)) {
    return true;
  }
  // Otherwise, the delegate has accepted responsibility to run the
  // callback when the download is ready for completion.
  return false;
}

bool DownloadManagerImpl::ShouldOpenFileBasedOnExtension(
    const base::FilePath& path) {
  if (!delegate_)
    return false;

  return delegate_->ShouldOpenFileBasedOnExtension(path);
}

bool DownloadManagerImpl::ShouldOpenDownload(
    download::DownloadItemImpl* item,
    const ShouldOpenDownloadCallback& callback) {
  if (!delegate_)
    return true;

  // Relies on DownloadItemImplDelegate::ShouldOpenDownloadCallback and
  // DownloadManagerDelegate::DownloadOpenDelayedCallback "just happening"
  // to have the same type :-}.
  return delegate_->ShouldOpenDownload(item, callback);
}

void DownloadManagerImpl::SetDelegate(DownloadManagerDelegate* delegate) {
  delegate_ = delegate;
}

DownloadManagerDelegate* DownloadManagerImpl::GetDelegate() const {
  return delegate_;
}

void DownloadManagerImpl::Shutdown() {
  DVLOG(20) << __func__ << "() shutdown_needed_ = " << shutdown_needed_;
  if (!shutdown_needed_)
    return;
  shutdown_needed_ = false;

  for (auto& observer : observers_)
    observer.ManagerGoingDown(this);
  // TODO(benjhayden): Consider clearing observers_.

  // If there are in-progress downloads, cancel them. This also goes for
  // dangerous downloads which will remain in history if they aren't explicitly
  // accepted or discarded. Canceling will remove the intermediate download
  // file.
  for (const auto& it : downloads_) {
    download::DownloadItemImpl* download = it.second.get();
    if (download->GetState() == download::DownloadItem::IN_PROGRESS)
      download->Cancel(false);
  }
  downloads_.clear();
  downloads_by_guid_.clear();
  url_download_handlers_.clear();

  // We'll have nothing more to report to the observers after this point.
  observers_.Clear();

  in_progress_manager_->ShutDown();

  if (delegate_)
    delegate_->Shutdown();
  delegate_ = nullptr;
}

bool DownloadManagerImpl::InterceptDownload(
    const download::DownloadCreateInfo& info) {
  WebContents* web_contents = WebContentsImpl::FromRenderFrameHostID(
      info.render_process_id, info.render_frame_id);
  if (!delegate_ ||
      !delegate_->InterceptDownloadIfApplicable(
          info.url(), info.mime_type, info.request_origin, web_contents)) {
    return false;
  }
  if (info.request_handle)
    info.request_handle->CancelRequest(false);
  return true;
}

base::FilePath DownloadManagerImpl::GetDefaultDownloadDirectory() {
  base::FilePath default_download_directory;
#if defined(USE_X11)
  // TODO(thomasanderson,crbug.com/784010): Remove this when all Linux
  // distros with versions of GTK lower than 3.14.7 are no longer
  // supported.  This should happen when support for Ubuntu Trusty and
  // Debian Jessie are removed.
  default_download_directory = GetTemporaryDownloadDirectory();
#else
  if (delegate_) {
    base::FilePath website_save_directory;  // Unused
    bool skip_dir_check = false;            // Unused
    delegate_->GetSaveDir(GetBrowserContext(), &website_save_directory,
                          &default_download_directory, &skip_dir_check);
  }
#endif
  if (default_download_directory.empty()) {
    // |default_download_directory| can still be empty if ContentBrowserClient
    // returned an empty path for the downloads directory.
    default_download_directory =
        GetContentClient()->browser()->GetDefaultDownloadDirectory();
  }

  return default_download_directory;
}

void DownloadManagerImpl::OnNewDownloadStarted(
    download::DownloadItem* download) {
  for (auto& observer : observers_)
    observer.OnDownloadCreated(this, download);
}

download::DownloadItemImpl* DownloadManagerImpl::GetDownloadItem(
    uint32_t id,
    bool new_download,
    const download::DownloadCreateInfo& info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_NE(download::DownloadItem::kInvalidId, id);

  download::DownloadItemImpl* download = nullptr;
  if (new_download) {
    download = CreateActiveItem(id, info);
  } else {
    auto item_iterator = downloads_.find(id);
    // Trying to resume an interrupted download.
    if (item_iterator == downloads_.end() ||
        (item_iterator->second->GetState() ==
         download::DownloadItem::CANCELLED)) {
      return nullptr;
    }
    download = item_iterator->second.get();
  }
  DownloadItemUtils::AttachInfo(
      download, GetBrowserContext(),
      WebContentsImpl::FromRenderFrameHostID(info.render_process_id,
                                             info.render_frame_id));
  return download;
}

net::URLRequestContextGetter* DownloadManagerImpl::GetURLRequestContextGetter(
    const download::DownloadCreateInfo& info) {
  StoragePartition* storage_partition = GetStoragePartition(
      browser_context_, info.render_process_id, info.render_frame_id);
  return storage_partition ? storage_partition->GetURLRequestContext()
                           : nullptr;
}

void DownloadManagerImpl::StartDownload(
    std::unique_ptr<download::DownloadCreateInfo> info,
    std::unique_ptr<download::InputStream> stream,
    scoped_refptr<download::DownloadURLLoaderFactoryGetter>
        url_loader_factory_getter,
    const download::DownloadUrlParameters::OnStartedCallback& on_started) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(info);
  in_progress_manager_->StartDownload(std::move(info), std::move(stream),
                                      std::move(url_loader_factory_getter),
                                      on_started);
}

void DownloadManagerImpl::CheckForHistoryFilesRemoval() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (const auto& it : downloads_) {
    download::DownloadItemImpl* item = it.second.get();
    CheckForFileRemoval(item);
  }
}

void DownloadManagerImpl::CheckForFileRemoval(
    download::DownloadItemImpl* download_item) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if ((download_item->GetState() == download::DownloadItem::COMPLETE) &&
      !download_item->GetFileExternallyRemoved() && delegate_) {
    delegate_->CheckForFileExistence(
        download_item,
        base::BindOnce(&DownloadManagerImpl::OnFileExistenceChecked,
                       weak_factory_.GetWeakPtr(), download_item->GetId()));
  }
}

void DownloadManagerImpl::OnFileExistenceChecked(uint32_t download_id,
                                                 bool result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!result) {  // File does not exist.
    if (base::ContainsKey(downloads_, download_id))
      downloads_[download_id]->OnDownloadedFileRemoved();
  }
}

std::string DownloadManagerImpl::GetApplicationClientIdForFileScanning() const {
  if (delegate_)
    return delegate_->ApplicationClientIdForFileScanning();
  return std::string();
}

BrowserContext* DownloadManagerImpl::GetBrowserContext() const {
  return browser_context_;
}

void DownloadManagerImpl::CreateSavePackageDownloadItem(
    const base::FilePath& main_file_path,
    const GURL& page_url,
    const std::string& mime_type,
    int render_process_id,
    int render_frame_id,
    std::unique_ptr<download::DownloadRequestHandleInterface> request_handle,
    const DownloadItemImplCreated& item_created) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  GetNextId(
      base::Bind(&DownloadManagerImpl::CreateSavePackageDownloadItemWithId,
                 weak_factory_.GetWeakPtr(), main_file_path, page_url,
                 mime_type, render_process_id, render_frame_id,
                 base::Passed(std::move(request_handle)), item_created));
}

void DownloadManagerImpl::CreateSavePackageDownloadItemWithId(
    const base::FilePath& main_file_path,
    const GURL& page_url,
    const std::string& mime_type,
    int render_process_id,
    int render_frame_id,
    std::unique_ptr<download::DownloadRequestHandleInterface> request_handle,
    const DownloadItemImplCreated& item_created,
    uint32_t id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_NE(download::DownloadItem::kInvalidId, id);
  DCHECK(!base::ContainsKey(downloads_, id));
  download::DownloadItemImpl* download_item = item_factory_->CreateSavePageItem(
      this, id, main_file_path, page_url, mime_type, std::move(request_handle));
  DownloadItemUtils::AttachInfo(download_item, GetBrowserContext(),
                                WebContentsImpl::FromRenderFrameHostID(
                                    render_process_id, render_frame_id));
  downloads_[download_item->GetId()] = base::WrapUnique(download_item);
  DCHECK(!base::ContainsKey(downloads_by_guid_, download_item->GetGuid()));
  downloads_by_guid_[download_item->GetGuid()] = download_item;
  for (auto& observer : observers_)
    observer.OnDownloadCreated(this, download_item);
  if (!item_created.is_null())
    item_created.Run(download_item);
}

// Resume a download of a specific URL. We send the request to the
// ResourceDispatcherHost, and let it send us responses like a regular
// download.
void DownloadManagerImpl::ResumeInterruptedDownload(
    std::unique_ptr<download::DownloadUrlParameters> params,
    uint32_t id,
    const GURL& site_url) {
  BeginDownloadInternal(std::move(params), nullptr /* blob_data_handle */,
                        nullptr /* blob_url_loader_factory */, id, site_url);
}

void DownloadManagerImpl::SetDownloadItemFactoryForTesting(
    std::unique_ptr<download::DownloadItemFactory> item_factory) {
  item_factory_ = std::move(item_factory);
}

void DownloadManagerImpl::SetDownloadFileFactoryForTesting(
    std::unique_ptr<download::DownloadFileFactory> file_factory) {
  in_progress_manager_->set_file_factory(std::move(file_factory));
}

download::DownloadFileFactory*
DownloadManagerImpl::GetDownloadFileFactoryForTesting() {
  return in_progress_manager_->file_factory();
}

void DownloadManagerImpl::DownloadRemoved(
    download::DownloadItemImpl* download) {
  if (!download)
    return;

  downloads_by_guid_.erase(download->GetGuid());
  downloads_.erase(download->GetId());
}

void DownloadManagerImpl::DownloadInterrupted(
    download::DownloadItemImpl* download) {
  WebContents* web_contents = DownloadItemUtils::GetWebContents(download);
  if (!web_contents) {
    download::RecordDownloadCountWithSource(
        download::INTERRUPTED_WITHOUT_WEBCONTENTS, download->download_source());
  }
}

base::Optional<download::DownloadEntry> DownloadManagerImpl::GetInProgressEntry(
    download::DownloadItemImpl* download) {
  return in_progress_manager_->GetInProgressEntry(download);
}

bool DownloadManagerImpl::IsOffTheRecord() const {
  return browser_context_->IsOffTheRecord();
}

void DownloadManagerImpl::ReportBytesWasted(
    download::DownloadItemImpl* download) {
  in_progress_manager_->ReportBytesWasted(download);
}

void DownloadManagerImpl::OnUrlDownloadHandlerCreated(
    download::UrlDownloadHandler::UniqueUrlDownloadHandlerPtr downloader) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (downloader)
    url_download_handlers_.push_back(std::move(downloader));
}

// static
download::DownloadInterruptReason DownloadManagerImpl::BeginDownloadRequest(
    std::unique_ptr<net::URLRequest> url_request,
    ResourceContext* resource_context,
    download::DownloadUrlParameters* params) {
  if (ResourceDispatcherHostImpl::Get()->is_shutdown())
    return download::DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN;

  // The URLRequest needs to be initialized with the referrer and other
  // information prior to issuing it.
  ResourceDispatcherHostImpl::Get()->InitializeURLRequest(
      url_request.get(),
      Referrer(params->referrer(),
               Referrer::NetReferrerPolicyToBlinkReferrerPolicy(
                   params->referrer_policy())),
      true,  // download.
      params->render_process_host_id(), params->render_view_host_routing_id(),
      params->render_frame_host_routing_id(), PREVIEWS_OFF, resource_context);

  // We treat a download as a main frame load, and thus update the policy URL on
  // redirects.
  //
  // TODO(davidben): Is this correct? If this came from a
  // ViewHostMsg_DownloadUrl in a frame, should it have first-party URL set
  // appropriately?
  url_request->set_first_party_url_policy(
      net::URLRequest::UPDATE_FIRST_PARTY_URL_ON_REDIRECT);

  const GURL& url = url_request->original_url();

  const net::URLRequestContext* request_context = url_request->context();
  if (!request_context->job_factory()->IsHandledProtocol(url.scheme())) {
    DVLOG(1) << "Download request for unsupported protocol: "
             << url.possibly_invalid_spec();
    return download::DOWNLOAD_INTERRUPT_REASON_NETWORK_INVALID_REQUEST;
  }

  // From this point forward, the |DownloadResourceHandler| is responsible for
  // |started_callback|.
  // TODO(ananta)
  // Find a better way to create the DownloadResourceHandler instance.
  std::unique_ptr<ResourceHandler> handler(
      DownloadResourceHandler::CreateForNewRequest(url_request.get(),
                                                   params->request_origin(),
                                                   params->download_source()));

  ResourceDispatcherHostImpl::Get()->BeginURLRequest(
      std::move(url_request), std::move(handler), true,  // download
      params->content_initiated(), params->do_not_prompt_for_login(),
      resource_context);
  return download::DOWNLOAD_INTERRUPT_REASON_NONE;
}

void DownloadManagerImpl::InterceptNavigation(
    std::unique_ptr<network::ResourceRequest> resource_request,
    std::vector<GURL> url_chain,
    scoped_refptr<network::ResourceResponse> response,
    network::mojom::URLLoaderClientEndpointsPtr url_loader_client_endpoints,
    net::CertStatus cert_status,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!delegate_) {
    DropDownload();
    return;
  }

  const GURL& url = resource_request->url;
  const std::string& method = resource_request->method;

  ResourceRequestInfo::WebContentsGetter web_contents_getter =
      base::BindRepeating(WebContents::FromFrameTreeNodeId, frame_tree_node_id);

  base::OnceCallback<void(bool /* download allowed */)>
      on_download_checks_done = base::BindOnce(
          &DownloadManagerImpl::InterceptNavigationOnChecksComplete,
          weak_factory_.GetWeakPtr(), web_contents_getter,
          std::move(resource_request), std::move(url_chain),
          std::move(response), cert_status,
          std::move(url_loader_client_endpoints));

  delegate_->CheckDownloadAllowed(std::move(web_contents_getter), url, method,
                                  std::move(on_download_checks_done));
}

int DownloadManagerImpl::RemoveDownloadsByURLAndTime(
    const base::Callback<bool(const GURL&)>& url_filter,
    base::Time remove_begin,
    base::Time remove_end) {
  int count = 0;
  auto it = downloads_.begin();
  while (it != downloads_.end()) {
    download::DownloadItemImpl* download = it->second.get();

    // Increment done here to protect against invalidation below.
    ++it;

    if (download->GetState() != download::DownloadItem::IN_PROGRESS &&
        url_filter.Run(download->GetURL()) &&
        download->GetStartTime() >= remove_begin &&
        (remove_end.is_null() || download->GetStartTime() < remove_end)) {
      download->Remove();
      count++;
    }
  }
  return count;
}

void DownloadManagerImpl::DownloadUrl(
    std::unique_ptr<download::DownloadUrlParameters> params) {
  DownloadUrl(std::move(params), nullptr /* blob_data_handle */,
              nullptr /* blob_url_loader_factory */);
}

void DownloadManagerImpl::DownloadUrl(
    std::unique_ptr<download::DownloadUrlParameters> params,
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle,
    scoped_refptr<network::SharedURLLoaderFactory> blob_url_loader_factory) {
  if (params->post_id() >= 0) {
    // Check this here so that the traceback is more useful.
    DCHECK(params->prefer_cache());
    DCHECK_EQ("POST", params->method());
  }

  download::RecordDownloadCountWithSource(
      download::DownloadCountTypes::DOWNLOAD_TRIGGERED_COUNT,
      params->download_source());
  auto* rfh = RenderFrameHost::FromID(params->render_process_host_id(),
                                      params->render_frame_host_routing_id());
  BeginDownloadInternal(std::move(params), std::move(blob_data_handle),
                        std::move(blob_url_loader_factory),
                        download::DownloadItem::kInvalidId,
                        rfh ? rfh->GetSiteInstance()->GetSiteURL() : GURL());
}

void DownloadManagerImpl::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DownloadManagerImpl::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

download::DownloadItem* DownloadManagerImpl::CreateDownloadItem(
    const std::string& guid,
    uint32_t id,
    const base::FilePath& current_path,
    const base::FilePath& target_path,
    const std::vector<GURL>& url_chain,
    const GURL& referrer_url,
    const GURL& site_url,
    const GURL& tab_url,
    const GURL& tab_refererr_url,
    const std::string& mime_type,
    const std::string& original_mime_type,
    base::Time start_time,
    base::Time end_time,
    const std::string& etag,
    const std::string& last_modified,
    int64_t received_bytes,
    int64_t total_bytes,
    const std::string& hash,
    download::DownloadItem::DownloadState state,
    download::DownloadDangerType danger_type,
    download::DownloadInterruptReason interrupt_reason,
    bool opened,
    base::Time last_access_time,
    bool transient,
    const std::vector<download::DownloadItem::ReceivedSlice>& received_slices) {
  if (base::ContainsKey(downloads_, id)) {
    NOTREACHED();
    return nullptr;
  }
  DCHECK(!base::ContainsKey(downloads_by_guid_, guid));
  download::DownloadItemImpl* item = item_factory_->CreatePersistedItem(
      this, guid, id, current_path, target_path, url_chain, referrer_url,
      site_url, tab_url, tab_refererr_url, mime_type, original_mime_type,
      start_time, end_time, etag, last_modified, received_bytes, total_bytes,
      hash, state, danger_type, interrupt_reason, opened, last_access_time,
      transient, received_slices);
  DownloadItemUtils::AttachInfo(item, GetBrowserContext(), nullptr);
  downloads_[id] = base::WrapUnique(item);
  downloads_by_guid_[guid] = item;
  for (auto& observer : observers_)
    observer.OnDownloadCreated(this, item);
  DVLOG(20) << __func__ << "() download = " << item->DebugString(true);
  return item;
}

void DownloadManagerImpl::PostInitialization(
    DownloadInitializationDependency dependency) {
  // If initialization has occurred (ie. in tests), skip post init steps.
  if (initialized_)
    return;

  switch (dependency) {
    case DOWNLOAD_INITIALIZATION_DEPENDENCY_HISTORY_DB:
      history_db_initialized_ = true;
      break;
    case DOWNLOAD_INITIALIZATION_DEPENDENCY_IN_PROGRESS_CACHE:
      in_progress_cache_initialized_ = true;
      break;
    case DOWNLOAD_INITIALIZATION_DEPENDENCY_NONE:
    default:
      NOTREACHED();
      break;
  }

  // Download manager is only initialized if both history db and in progress
  // cache are initialized.
  initialized_ = history_db_initialized_ && in_progress_cache_initialized_;

  if (initialized_) {
    for (auto& observer : observers_)
      observer.OnManagerInitialized();
  }
}

bool DownloadManagerImpl::IsManagerInitialized() const {
  return initialized_;
}

int DownloadManagerImpl::InProgressCount() const {
  int count = 0;
  for (const auto& it : downloads_) {
    if (it.second->GetState() == download::DownloadItem::IN_PROGRESS)
      ++count;
  }
  return count;
}

int DownloadManagerImpl::NonMaliciousInProgressCount() const {
  int count = 0;
  for (const auto& it : downloads_) {
    if (it.second->GetState() == download::DownloadItem::IN_PROGRESS &&
        it.second->GetDangerType() !=
            download::DOWNLOAD_DANGER_TYPE_DANGEROUS_URL &&
        it.second->GetDangerType() !=
            download::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT &&
        it.second->GetDangerType() !=
            download::DOWNLOAD_DANGER_TYPE_DANGEROUS_HOST &&
        it.second->GetDangerType() !=
            download::DOWNLOAD_DANGER_TYPE_POTENTIALLY_UNWANTED) {
      ++count;
    }
  }
  return count;
}

download::DownloadItem* DownloadManagerImpl::GetDownload(uint32_t download_id) {
  return base::ContainsKey(downloads_, download_id)
             ? downloads_[download_id].get()
             : nullptr;
}

download::DownloadItem* DownloadManagerImpl::GetDownloadByGuid(
    const std::string& guid) {
  return base::ContainsKey(downloads_by_guid_, guid) ? downloads_by_guid_[guid]
                                                     : nullptr;
}

void DownloadManagerImpl::OnUrlDownloadStarted(
    std::unique_ptr<download::DownloadCreateInfo> download_create_info,
    std::unique_ptr<download::InputStream> stream,
    scoped_refptr<download::DownloadURLLoaderFactoryGetter>
        url_loader_factory_getter,
    const download::DownloadUrlParameters::OnStartedCallback& callback) {
  StartDownload(std::move(download_create_info), std::move(stream),
                std::move(url_loader_factory_getter), callback);
}

void DownloadManagerImpl::OnUrlDownloadStopped(
    download::UrlDownloadHandler* downloader) {
  for (auto ptr = url_download_handlers_.begin();
       ptr != url_download_handlers_.end(); ++ptr) {
    if (ptr->get() == downloader) {
      url_download_handlers_.erase(ptr);
      return;
    }
  }
}

void DownloadManagerImpl::GetAllDownloads(DownloadVector* downloads) {
  for (const auto& it : downloads_) {
    downloads->push_back(it.second.get());
  }
}

void DownloadManagerImpl::OpenDownload(download::DownloadItemImpl* download) {
  int num_unopened = 0;
  for (const auto& it : downloads_) {
    download::DownloadItemImpl* item = it.second.get();
    if ((item->GetState() == download::DownloadItem::COMPLETE) &&
        !item->GetOpened())
      ++num_unopened;
  }
  download::RecordOpensOutstanding(num_unopened);

  if (delegate_)
    delegate_->OpenDownload(download);
}

bool DownloadManagerImpl::IsMostRecentDownloadItemAtFilePath(
    download::DownloadItemImpl* download) {
  return delegate_ ? delegate_->IsMostRecentDownloadItemAtFilePath(download)
                   : false;
}

void DownloadManagerImpl::ShowDownloadInShell(
    download::DownloadItemImpl* download) {
  if (delegate_)
    delegate_->ShowDownloadInShell(download);
}

void DownloadManagerImpl::DropDownload() {
  download::RecordDownloadCount(download::DOWNLOAD_DROPPED_COUNT);
  for (auto& observer : observers_)
    observer.OnDownloadDropped(this);
}

void DownloadManagerImpl::InterceptNavigationOnChecksComplete(
    ResourceRequestInfo::WebContentsGetter web_contents_getter,
    std::unique_ptr<network::ResourceRequest> resource_request,
    std::vector<GURL> url_chain,
    scoped_refptr<network::ResourceResponse> response,
    net::CertStatus cert_status,
    network::mojom::URLLoaderClientEndpointsPtr url_loader_client_endpoints,
    bool is_download_allowed) {
  if (!is_download_allowed) {
    DropDownload();
    return;
  }

  int render_process_id = -1;
  int render_frame_id = -1;
  GURL site_url, tab_url, tab_referrer_url;
  RenderFrameHost* render_frame_host = nullptr;
  WebContents* web_contents = std::move(web_contents_getter).Run();
  if (web_contents) {
    render_frame_host = web_contents->GetMainFrame();
    if (render_frame_host) {
      render_process_id = render_frame_host->GetProcess()->GetID();
      render_frame_id = render_frame_host->GetRoutingID();
    }
    NavigationEntry* entry = web_contents->GetController().GetVisibleEntry();
    if (entry) {
      tab_url = entry->GetURL();
      tab_referrer_url = entry->GetReferrer().url;
    }
  }
  StoragePartitionImpl* storage_partition =
      GetStoragePartition(browser_context_, render_process_id, render_frame_id);
  in_progress_manager_->InterceptDownloadFromNavigation(
      std::move(resource_request), render_process_id, render_frame_id, site_url,
      tab_url, tab_referrer_url, std::move(url_chain), std::move(response),
      std::move(cert_status), std::move(url_loader_client_endpoints),
      CreateDownloadURLLoaderFactoryGetter(storage_partition, render_frame_host,
                                           false));
}

void DownloadManagerImpl::BeginResourceDownloadOnChecksComplete(
    std::unique_ptr<download::DownloadUrlParameters> params,
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle,
    scoped_refptr<network::SharedURLLoaderFactory> blob_url_loader_factory,
    uint32_t id,
    const GURL& site_url,
    bool is_download_allowed) {
  if (!is_download_allowed) {
    DropDownload();
    return;
  }

  GURL tab_url, tab_referrer_url;
  auto* rfh = RenderFrameHost::FromID(params->render_process_host_id(),
                                      params->render_frame_host_routing_id());
  if (rfh) {
    auto* web_contents = WebContents::FromRenderFrameHost(rfh);
    NavigationEntry* entry = web_contents->GetController().GetVisibleEntry();
    if (entry) {
      tab_url = entry->GetURL();
      tab_referrer_url = entry->GetReferrer().url;
    }
  }

  scoped_refptr<download::DownloadURLLoaderFactoryGetter>
      url_loader_factory_getter;
  if (blob_url_loader_factory) {
    DCHECK(params->url().SchemeIsBlob());
    url_loader_factory_getter =
        base::MakeRefCounted<DownloadURLLoaderFactoryGetterImpl>(
            blob_url_loader_factory->Clone());
  } else if (params->url().SchemeIsBlob()) {
    url_loader_factory_getter =
        base::MakeRefCounted<BlobDownloadURLLoaderFactoryGetter>(
            params->url(), std::move(blob_data_handle));
  } else {
    StoragePartitionImpl* storage_partition =
        static_cast<StoragePartitionImpl*>(
            BrowserContext::GetStoragePartitionForSite(browser_context_,
                                                       site_url));
    url_loader_factory_getter =
        CreateDownloadURLLoaderFactoryGetter(storage_partition, rfh, true);
  }

  in_progress_manager_->BeginDownload(std::move(params),
                                      std::move(url_loader_factory_getter), id,
                                      site_url, tab_url, tab_referrer_url);
}

void DownloadManagerImpl::BeginDownloadInternal(
    std::unique_ptr<download::DownloadUrlParameters> params,
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle,
    scoped_refptr<network::SharedURLLoaderFactory> blob_url_loader_factory,
    uint32_t id,
    const GURL& site_url) {
  // Check if the renderer is permitted to request the requested URL.
  if (params->render_process_host_id() >= 0 &&
      !CanRequestURLFromRenderer(params->render_process_host_id(),
                                 params->url())) {
    CreateInterruptedDownload(
        std::move(params),
        download::DOWNLOAD_INTERRUPT_REASON_NETWORK_INVALID_REQUEST,
        weak_factory_.GetWeakPtr());
    return;
  }

  if (base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    auto* rfh = RenderFrameHost::FromID(params->render_process_host_id(),
                                        params->render_frame_host_routing_id());
    bool content_initiated = params->content_initiated();
    // If it's from the web, we don't trust it, so we push the throttle on.
    if (rfh && content_initiated) {
      ResourceRequestInfo::WebContentsGetter web_contents_getter =
          base::BindRepeating(WebContents::FromFrameTreeNodeId,
                              rfh->GetFrameTreeNodeId());
      const GURL& url = params->url();
      const std::string& method = params->method();

      base::OnceCallback<void(bool /* download allowed */)>
          on_can_download_checks_done = base::BindOnce(
              &DownloadManagerImpl::BeginResourceDownloadOnChecksComplete,
              weak_factory_.GetWeakPtr(), std::move(params),
              std::move(blob_data_handle), std::move(blob_url_loader_factory),
              id, site_url);
      if (delegate_) {
        delegate_->CheckDownloadAllowed(std::move(web_contents_getter), url,
                                        method,
                                        std::move(on_can_download_checks_done));
        return;
      }
    }

    BeginResourceDownloadOnChecksComplete(
        std::move(params), std::move(blob_data_handle),
        std::move(blob_url_loader_factory), id, site_url,
        rfh ? !content_initiated : true);
  } else {
    StoragePartition* storage_partition =
        BrowserContext::GetStoragePartitionForSite(browser_context_, site_url);
    params->set_url_request_context_getter(
        storage_partition->GetURLRequestContext());
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&BeginDownload, std::move(params),
                       std::move(blob_data_handle),
                       browser_context_->GetResourceContext(), id,
                       weak_factory_.GetWeakPtr()));
   }
}

}  // namespace content
