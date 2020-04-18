// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_
#define CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "chrome/browser/media/webrtc/webrtc_event_log_manager_common.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

// A class implementing this interace can register for notification of an
// upload's eventual result (success/failure).
class WebRtcEventLogUploaderObserver {
 public:
  virtual void OnWebRtcEventLogUploadComplete(const base::FilePath& log_file,
                                              bool upload_successful) = 0;

 protected:
  virtual ~WebRtcEventLogUploaderObserver() = default;
};

// A sublcass of this interface would take ownership of a file, and either
// upload it to a remote server (actual implementation), or pretend to do
// so (in unit tests). It will typically take on an observer of type
// WebRtcEventLogUploaderObserver, and inform it of the success or failure
// of the upload.
class WebRtcEventLogUploader {
 public:
  // Since we'll need more than one instance of the abstract
  // WebRtcEventLogUploader, we'll need an abstract factory for it.
  class Factory {
   public:
    virtual ~Factory() = default;

    // Creates uploaders. The observer is passed to each call of Create,
    // rather than be memorized by the factory's constructor, because factories
    // created by unit tests have no visibility into the real implementation's
    // observer (WebRtcRemoteEventLogManager).
    // This takes ownership of the file. The caller must not attempt to access
    // the file after invoking Create().
    virtual std::unique_ptr<WebRtcEventLogUploader> Create(
        const base::FilePath& log_file,
        WebRtcEventLogUploaderObserver* observer) = 0;
  };

  virtual ~WebRtcEventLogUploader() = default;
};

// Primary implementation of WebRtcEventLogUploader. Uploads log files to crash.
// Deletes log files whether they were successfully uploaded or not.
class WebRtcEventLogUploaderImpl : public WebRtcEventLogUploader {
 public:
  class Factory : public WebRtcEventLogUploader::Factory {
   public:
    ~Factory() override = default;

    std::unique_ptr<WebRtcEventLogUploader> Create(
        const base::FilePath& log_file,
        WebRtcEventLogUploaderObserver* observer) override;

   protected:
    friend class WebRtcEventLogUploaderImplTest;

    std::unique_ptr<WebRtcEventLogUploader> CreateWithCustomMaxSizeForTesting(
        const base::FilePath& log_file,
        WebRtcEventLogUploaderObserver* observer,
        size_t max_remote_log_file_size_bytes);
  };

  WebRtcEventLogUploaderImpl(const base::FilePath& log_file,
                             WebRtcEventLogUploaderObserver* observer,
                             size_t max_remote_log_file_size_bytes);
  ~WebRtcEventLogUploaderImpl() override;

 protected:
  friend class WebRtcEventLogUploaderImplTest;

  // Prepare the data that will be uploaded. Runs on io_task_runner_.
  bool PrepareUploadData();

  // Prepares the URLRequestContextGetter. This has to run on the UI thread,
  // but once complete, the URLRequestContextGetter it produces, which is
  // stored in request_context_getter_, may be used on any task context.
  void PrepareRequestContext();

  // Initiates the upload. Runs on io_task_runner_, so that the callback will
  // also be called on io_task_runner_.
  void StartUpload();

  // Called on io_task_runner_.  Before this is called, other methods of the
  // URLFetcherDelegate API may be called, but this is guaranteed to be the
  // last call, so deleting |this| is permissible afterwards.
  void OnURLFetchComplete(const net::URLFetcher* source);

  // Cleanup and reporting to |observer_|.
  void ReportResult(bool result);

  // Remove the log file which is owned by |this|.
  void DeleteLogFile();

  // Allows testing the behavior for excessively large files.
  void SetMaxRemoteLogFileSizeBytesForTesting(size_t max_size_bytes);

  // The URL used for uploading the logs.
  static const char kUploadURL[];

 private:
  class Delegate : public net::URLFetcherDelegate {
   public:
    explicit Delegate(WebRtcEventLogUploaderImpl* owner);
    ~Delegate() override = default;

    // net::URLFetcherDelegate implementation.
#if DCHECK_IS_ON()
    void OnURLFetchUploadProgress(const net::URLFetcher* source,
                                  int64_t current,
                                  int64_t total) override;
#endif
    void OnURLFetchComplete(const net::URLFetcher* source) override;

   private:
    WebRtcEventLogUploaderImpl* const owner_;
  } delegate_;

  // The path to the WebRTC event log file that this uploader is in charge of.
  const base::FilePath log_file_;

  // The observer to be notified when this upload succeeds or fails.
  WebRtcEventLogUploaderObserver* const observer_;

  // Maximum allowed file size. In production code, this is a hard-coded,
  // but unit tests may set other values.
  const size_t max_log_file_size_bytes_;

  // This is written to on the UI thread, but used on the IO thread. It allows
  // the creation of URLFetcher objects.
  net::URLRequestContextGetter* request_context_getter_;

  // This object is in charge of the actual upload.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // To avoid an unnecessary hop to the UI thread when something is amiss with
  // the data we wish to upload, PrepareUploadData() is called first, and saves
  // the data here. When back from the UI thread, StartUpload will read this.
  std::string post_data_;

  // The object lives on this IO-capable task runner.
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
};

#endif  // CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_
