// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_
#define CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "components/download/public/background_service/download_params.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/offline_items_collection/core/offline_content_provider.h"
#include "components/offline_items_collection/core/offline_item.h"
#include "content/public/browser/background_fetch_delegate.h"
#include "ui/gfx/image/image.h"
#include "url/origin.h"

class Profile;

namespace download {
class DownloadService;
}  // namespace download

namespace offline_items_collection {
class OfflineContentAggregator;
}  // namespace offline_items_collection

// Implementation of BackgroundFetchDelegate using the DownloadService. This
// also implements OfflineContentProvider which allows it to show notifications
// for its downloads.
class BackgroundFetchDelegateImpl
    : public content::BackgroundFetchDelegate,
      public offline_items_collection::OfflineContentProvider,
      public KeyedService {
 public:
  explicit BackgroundFetchDelegateImpl(Profile* profile);

  ~BackgroundFetchDelegateImpl() override;

  // KeyedService implementation:
  void Shutdown() override;

  // BackgroundFetchDelegate implementation:
  void GetIconDisplaySize(GetIconDisplaySizeCallback callback) override;
  void CreateDownloadJob(std::unique_ptr<content::BackgroundFetchDescription>
                             fetch_description) override;
  void DownloadUrl(const std::string& job_unique_id,
                   const std::string& guid,
                   const std::string& method,
                   const GURL& url,
                   const net::NetworkTrafficAnnotationTag& traffic_annotation,
                   const net::HttpRequestHeaders& headers) override;
  void Abort(const std::string& job_unique_id) override;

  // Abort all ongoing downloads and fail the fetch. Currently only used when
  // the bytes downloaded exceed the total download size, if specified.
  void FailFetch(const std::string& job_unique_id);

  void OnDownloadStarted(const std::string& guid,
                         std::unique_ptr<content::BackgroundFetchResponse>);

  void OnDownloadUpdated(const std::string& guid, uint64_t bytes_downloaded);

  void OnDownloadFailed(const std::string& guid,
                        download::Client::FailureReason reason);

  void OnDownloadSucceeded(const std::string& guid,
                           const base::FilePath& path,
                           uint64_t size);

  // OfflineContentProvider implementation:
  void OpenItem(const offline_items_collection::ContentId& id) override;
  void RemoveItem(const offline_items_collection::ContentId& id) override;
  void CancelDownload(const offline_items_collection::ContentId& id) override;
  void PauseDownload(const offline_items_collection::ContentId& id) override;
  void ResumeDownload(const offline_items_collection::ContentId& id,
                      bool has_user_gesture) override;
  void GetItemById(const offline_items_collection::ContentId& id,
                   SingleItemCallback callback) override;
  void GetAllItems(MultipleItemCallback callback) override;
  void GetVisualsForItem(const offline_items_collection::ContentId& id,
                         const VisualsCallback& callback) override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

  base::WeakPtr<BackgroundFetchDelegateImpl> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  struct JobDetails {
    JobDetails(JobDetails&&);
    explicit JobDetails(
        std::unique_ptr<content::BackgroundFetchDescription> fetch_description);
    ~JobDetails();

    void UpdateOfflineItem();

    bool cancelled;

    // Set of DownloadService GUIDs that are currently downloading. They are
    // added by DownloadUrl and are removed when the download completes, fails
    // or is cancelled.
    base::flat_set<std::string> current_download_guids;

    offline_items_collection::OfflineItem offline_item;
    std::unique_ptr<content::BackgroundFetchDescription> fetch_description;

   private:
    // Whether we should report progress of the job in terms of size of
    // downloads or in terms of the number of files being downloaded.
    bool ShouldReportProgressBySize();

    DISALLOW_COPY_AND_ASSIGN(JobDetails);
  };

  // Updates the OfflineItem that controls the contents of download
  // notifications and notifies any OfflineContentProvider::Observer that was
  // registered with this instance.
  void UpdateOfflineItemAndUpdateObservers(JobDetails* job_details);

  void OnDownloadReceived(const std::string& guid,
                          download::DownloadParams::StartResult result);

  // The BackgroundFetchDelegateImplFactory depends on the
  // DownloadServiceFactory, so |download_service_| should outlive |this|.
  download::DownloadService* download_service_;

  // Map from individual download GUIDs to job unique ids.
  std::map<std::string, std::string> download_job_unique_id_map_;

  // Map from job unique ids to the details of the job.
  std::map<std::string, JobDetails> job_details_map_;

  offline_items_collection::OfflineContentAggregator*
      offline_content_aggregator_;

  // Set of Observers to be notified of any changes to the shown notifications.
  std::set<Observer*> observers_;

  base::WeakPtrFactory<BackgroundFetchDelegateImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchDelegateImpl);
};

#endif  // CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_
