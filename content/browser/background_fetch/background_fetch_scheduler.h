// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_SCHEDULER_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_SCHEDULER_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "content/browser/background_fetch/background_fetch_registration_id.h"
#include "content/browser/background_fetch/background_fetch_request_manager.h"
#include "content/common/content_export.h"

namespace content {

class BackgroundFetchRegistrationId;
class BackgroundFetchRequestInfo;
enum class BackgroundFetchReasonToAbort;

// Maintains a list of Controllers and chooses which ones should launch new
// downloads.
class CONTENT_EXPORT BackgroundFetchScheduler
    : public BackgroundFetchRequestManager {
 public:
  using FinishedCallback =
      base::OnceCallback<void(const BackgroundFetchRegistrationId&,
                              BackgroundFetchReasonToAbort)>;
  using MarkedCompleteCallback = base::OnceCallback<void()>;

  // Interface for download job controllers.
  class CONTENT_EXPORT Controller {
   public:
    virtual ~Controller();

    // Returns whether the Controller has any pending download requests.
    virtual bool HasMoreRequests() = 0;

    // Requests the download manager to start fetching |request|.
    virtual void StartRequest(
        scoped_refptr<BackgroundFetchRequestInfo> request) = 0;

    void Finish(BackgroundFetchReasonToAbort reason_to_abort);

    const BackgroundFetchRegistrationId& registration_id() const {
      return registration_id_;
    }

   protected:
    Controller(const BackgroundFetchRegistrationId& registration_id,
               FinishedCallback finished_callback);

   private:
    BackgroundFetchRegistrationId registration_id_;
    FinishedCallback finished_callback_;
  };

  using NextRequestCallback =
      base::OnceCallback<void(scoped_refptr<BackgroundFetchRequestInfo>)>;

  class CONTENT_EXPORT RequestProvider {
   public:
    virtual ~RequestProvider() {}

    // Retrieves the next pending request for |registration_id| and invoke
    // |callback| with it.
    virtual void PopNextRequest(
        const BackgroundFetchRegistrationId& registration_id,
        NextRequestCallback callback) = 0;

    // Marks |request| as complete and calls |callback| when done.
    virtual void MarkRequestAsComplete(
        const BackgroundFetchRegistrationId& registration_id,
        BackgroundFetchRequestInfo* request,
        MarkedCompleteCallback callback) = 0;
  };

  explicit BackgroundFetchScheduler(RequestProvider* request_provider);

  ~BackgroundFetchScheduler() override;

  // Adds a new job controller to the scheduler. May immediately start to
  // schedule jobs for |controller|.
  void AddJobController(Controller* controller);

  void set_max_concurrent_downloads(size_t new_max) {
    max_concurrent_downloads_ = new_max;
  }

  // BackgroundFetchRequestManager implementation:
  void MarkRequestAsComplete(
      const BackgroundFetchRegistrationId& registration_id,
      scoped_refptr<BackgroundFetchRequestInfo> request) override;
  void OnJobAborted(const BackgroundFetchRegistrationId& registration_id,
                    std::vector<std::string> aborted_guids) override;

 private:
  void ScheduleDownload();

  void DidPopNextRequest(BackgroundFetchScheduler::Controller* controller,
                         scoped_refptr<BackgroundFetchRequestInfo>);

  void DidMarkRequestAsComplete(
      BackgroundFetchScheduler::Controller* controller);

  RequestProvider* request_provider_;

  // The scheduler owns all the job controllers, holding them either in the
  // controller queue or the guid to controller map.
  base::circular_deque<Controller*> controller_queue_;
  std::map<std::string, Controller*> download_controller_map_;

  size_t max_concurrent_downloads_ = 1;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchScheduler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_SCHEDULER_H_
