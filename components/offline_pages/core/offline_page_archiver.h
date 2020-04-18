// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_ARCHIVER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_ARCHIVER_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/task_scheduler/post_task.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "url/gurl.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace content {
class WebContents;
}  // namespace content

namespace offline_pages {

class SystemDownloadManager;

// The results of attempting to move the offline page to a public directory, and
// registering it with the system download manager.
struct PublishArchiveResult {
  SavePageResult move_result;
  base::FilePath new_file_path;
  int64_t download_id;
};

using PublishArchiveDoneCallback =
    base::OnceCallback<void(const OfflinePageItem&, PublishArchiveResult*)>;

// Interface of a class responsible for creation of the archive for offline use.
//
// Archiver will be implemented by embedder and may have additional methods that
// are not interesting from the perspective of OfflinePageModel. Example of such
// extra information or capability is a way to enumerate available WebContents
// to find the one that needs to be used to create archive (or to map it to the
// URL passed in CreateArchive in some other way).
//
// Archiver will be responsible for naming the file that is being saved (it has
// URL, title and the whole page content at its disposal). For that it should be
// also configured with the path where the archives are stored.
//
// Archiver should be able to archive multiple pages in parallel, as these are
// asynchronous calls carried out by some other component.
//
// If archiver gets two consecutive requests to archive the same page (may be
// run in parallel) it can generate 2 different names for files and save the
// same page separately, as if these were 2 completely unrelated pages. It is up
// to the caller (e.g. OfflinePageModel) to make sure that situation like that
// does not happen.
//
// If the page is not completely loaded, it is up to the implementation of the
// archiver whether to respond with ERROR_CONTENT_UNAVAILABLE, wait longer to
// actually snapshot a complete page, or snapshot whatever is available at that
// point in time (what the user sees).
class OfflinePageArchiver {
 public:
  // Results of the archive creation.
  enum class ArchiverResult {
    SUCCESSFULLY_CREATED,           // Archive created successfully.
    ERROR_DEVICE_FULL,              // Cannot save the archive - device is full.
    ERROR_CANCELED,                 // Caller canceled the request.
    ERROR_CONTENT_UNAVAILABLE,      // Content to archive is not available.
    ERROR_ARCHIVE_CREATION_FAILED,  // Creation of archive failed.
    ERROR_SECURITY_CERTIFICATE,     // Page was loaded on secure connection, but
                                    // there was a security error.
    ERROR_ERROR_PAGE,               // We detected an error page.
    ERROR_INTERSTITIAL_PAGE,        // We detected an interstitial page.
    ERROR_SKIPPED,                  // Page shouldn't be archived like NTP or
                                    // file urls.
    ERROR_DIGEST_CALCULATION_FAILED,  // Failed to compute digest.
  };

  // Describes the parameters to control how to create an archive.
  struct CreateArchiveParams {
    CreateArchiveParams()
        : remove_popup_overlay(false), use_page_problem_detectors(false) {}

    // Whether to remove popup overlay that obstructs viewing normal content.
    bool remove_popup_overlay;

    // Run page problem detectors while generating MTHML if true.
    bool use_page_problem_detectors;
  };

  typedef base::Callback<void(OfflinePageArchiver* /* archiver */,
                              ArchiverResult /* result */,
                              const GURL& /* url */,
                              const base::FilePath& /* file_path */,
                              const base::string16& /* title */,
                              int64_t /* file_size */,
                              const std::string& /* digest */)>
      CreateArchiveCallback;

  virtual ~OfflinePageArchiver() {}

  // Starts creating the archive in the |archives_dir| per
  // |create_archive_params|. Once archive is created |callback| will be called
  // with the result and additional information.
  virtual void CreateArchive(const base::FilePath& archives_dir,
                             const CreateArchiveParams& create_archive_params,
                             content::WebContents* web_contents,
                             const CreateArchiveCallback& callback) = 0;

  // Publishes the page on a background thread, then returns to the
  // OfflinePageModelTaskified's done callback.
  virtual void PublishArchive(
      const OfflinePageItem& offline_page,
      const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
      const base::FilePath& publish_directory,
      SystemDownloadManager* download_manager,
      PublishArchiveDoneCallback publish_done_callback);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_ARCHIVER_H_
