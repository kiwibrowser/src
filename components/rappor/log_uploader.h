// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_LOG_UPLOADER_H_
#define COMPONENTS_RAPPOR_LOG_UPLOADER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/rappor/log_uploader_interface.h"
#include "url/gurl.h"

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace rappor {

// Uploads logs from RapporServiceImpl.  Logs are passed in via QueueLog(),
// stored internally, and uploaded one at a time.  A queued log will be uploaded
// at a fixed interval after the successful upload of the previous logs.  If an
// upload fails, the uploader will keep retrying the upload with an exponential
// backoff interval.
class LogUploader : public LogUploaderInterface {
 public:
  // Constructor takes the |server_url| that logs should be uploaded to, the
  // |mime_type| of the uploaded data, and |request_context| to create uploads
  // with.
  LogUploader(
      const GURL& server_url,
      const std::string& mime_type,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  // If the object is destroyed (or the program terminates) while logs are
  // queued, the logs are lost.
  ~LogUploader() override;

  // LogUploaderInterface:
  void Start() override;
  void Stop() override;
  void QueueLog(const std::string& log) override;

 protected:
  // Checks if an upload has been scheduled.
  virtual bool IsUploadScheduled() const;

  // Schedules a future call to StartScheduledUpload if one isn't already
  // pending.  Can be overridden for testing.
  virtual void ScheduleNextUpload(base::TimeDelta interval);

  // Starts transmission of the next log. Exposed for tests.
  void StartScheduledUpload();

  // Increases the upload interval each time it's called, to handle the case
  // where the server is having issues. Exposed for tests.
  static base::TimeDelta BackOffUploadInterval(base::TimeDelta);

 private:
  // Returns true if the uploader is allowed to start another upload.
  bool CanStartUpload() const;

  // Drops excess logs until we are under the size limit.
  void DropExcessLogs();

  // Called after transmission completes (whether successful or not).
  void OnSimpleLoaderComplete(std::unique_ptr<std::string> response_body);

  // Called when the upload is completed.
  void OnUploadFinished(bool server_is_healthy);

  // The server URL to upload logs to.
  const GURL server_url_;

  // The mime type to specify on uploaded logs.
  const std::string mime_type_;

  // The URL loader factory used to send uploads.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // True if the uploader is currently running.
  bool is_running_;

  // The outstanding transmission.
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;

  // The logs that still need to be uploaded.
  base::queue<std::string> queued_logs_;

  // A timer used to delay before attempting another upload.
  base::OneShotTimer upload_timer_;

  // Indicates that the last triggered upload hasn't resolved yet.
  bool has_callback_pending_;

  // The interval to wait after an upload's URLFetcher completion before
  // starting the next upload attempt.
  base::TimeDelta upload_interval_;

  DISALLOW_COPY_AND_ASSIGN(LogUploader);
};

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_LOG_UPLOADER_H_
