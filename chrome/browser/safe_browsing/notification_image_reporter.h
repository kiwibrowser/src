// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_NOTIFICATION_IMAGE_REPORTER_H_
#define CHROME_BROWSER_SAFE_BROWSING_NOTIFICATION_IMAGE_REPORTER_H_

#include <memory>

#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

class GURL;
class Profile;
class SkBitmap;

namespace base {
class RefCountedMemory;
}  // namespace base

namespace gfx {
class Size;
}  // namespace gfx

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace safe_browsing {

class SafeBrowsingDatabaseManager;

// Provides functionality for building and sending reports about notification
// content images to the Safe Browsing CSD server. This class and its member
// methods live on the UI thread.
class NotificationImageReporter {
 public:
  // CSD server URL to which notification image reports are sent.
  static const char kReportingUploadUrl[];

  explicit NotificationImageReporter(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  virtual ~NotificationImageReporter();

  // Report notification content image to SafeBrowsing CSD server if the user
  // has opted in, the origin is not whitelisted, and it is randomly sampled.
  // Can only be called on UI thread.
  void ReportNotificationImage(
      Profile* profile,
      const scoped_refptr<SafeBrowsingDatabaseManager>& database_manager,
      const GURL& origin,
      const SkBitmap& image);

 protected:
  // Get the percentage of images that should be reported from Finch.
  virtual double GetReportChance() const;

  // Tests may wish to override this to find out if reports are skipped.
  virtual void SkippedReporting();

  // Send the given report. Virtual to allow tests to override it.
  virtual void SendReportInternal(const GURL& url,
                                  const std::string& content_type,
                                  const std::string& report);

 private:
  // Downscales image to fit within 512x512 if necessary, and encodes as it PNG,
  // then invokes SendReport. This should be called on a blocking pool
  // thread and will invoke a member function on the UI thread using the
  // weakptr.
  static void DownscaleNotificationImageOnBlockingPool(
      const base::WeakPtr<NotificationImageReporter>& weak_ptr,
      const GURL& origin,
      const SkBitmap& image);

  // Serializes report using NotificationImageReportRequest protobuf defined in
  // chrome/common/safe_browsing/csd.proto and sends it to CSD server.
  void SendReport(const GURL& origin,
                  scoped_refptr<base::RefCountedMemory> data,
                  const gfx::Size& dimensions,
                  const gfx::Size& original_dimensions);

  // Called when the asynchronous CSD whitelist check completes.
  void OnWhitelistCheckDone(Profile* profile,
                            const GURL& origin,
                            const SkBitmap& image,
                            bool match_whitelist);
  static void OnWhitelistCheckDoneOnIO(
      base::WeakPtr<NotificationImageReporter> weak_ptr,
      Profile* profile,
      const GURL& origin,
      const SkBitmap& image,
      bool match_whitelist);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // Timestamps of when we sent notification images. Used to limit the number
  // of requests that we send in a day. Only access on the IO thread.
  // TODO(johnme): Serialize this so that it doesn't reset on browser restart.
  base::queue<base::Time> report_times_;

  base::WeakPtrFactory<NotificationImageReporter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NotificationImageReporter);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_NOTIFICATION_IMAGE_REPORTER_H_
