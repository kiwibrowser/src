// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEEDBACK_FEEDBACK_PROFILE_OBSERVER_H_
#define CHROME_BROWSER_FEEDBACK_FEEDBACK_PROFILE_OBSERVER_H_

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace content {
class BrowserContext;
}

namespace feedback {

class FeedbackUploader;

// FeedbackProfileObserver waits on profile creation notifications to check
// if the profile has any pending feedback reports to upload. If it does, it
// queues those reports for upload.
class FeedbackProfileObserver : public content::NotificationObserver {
 public:
  static void Initialize();

 private:
  friend struct base::LazyInstanceTraitsBase<FeedbackProfileObserver>;

  FeedbackProfileObserver();
  ~FeedbackProfileObserver() override;

  // content::NotificationObserver override
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Loads any unsent reports from disk and queues them to be uploaded in
  // the given browser context.
  void QueueUnsentReports(content::BrowserContext* context);

  static void QueueSingleReport(feedback::FeedbackUploader* uploader,
                                const std::string& data);

  // Used to track creation of profiles so we can load any unsent reports
  // for that profile.
  content::NotificationRegistrar prefs_registrar_;

  DISALLOW_COPY_AND_ASSIGN(FeedbackProfileObserver);
};

}  // namespace feedback

#endif  // CHROME_BROWSER_FEEDBACK_FEEDBACK_PROFILE_OBSERVER_H_
