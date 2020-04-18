// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/notification_image_reporter.h"

#include <cmath>
#include <vector>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/metrics/histogram_functions.h"
#include "base/rand_util.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/chrome_report_sender.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/db/database_manager.h"
#include "components/safe_browsing/db/whitelist_checker_client.h"
#include "components/safe_browsing/proto/csd.pb.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace safe_browsing {

namespace {

const size_t kMaxReportsPerDay = 5;
const base::Feature kNotificationImageReporterFeature{
    "NotificationImageReporterFeature", base::FEATURE_ENABLED_BY_DEFAULT};
const char kReportChance[] = "ReportChance";
const char kDefaultMimeType[] = "image/png";

// Passed to ReportSender::Send as an ErrorCallback, so must take a GURL, but it
// is unused.
void LogReportResult(int net_error, int http_response_code) {
  base::UmaHistogramSparse("SafeBrowsing.NotificationImageReporter.NetError",
                           net_error);
}

constexpr net::NetworkTrafficAnnotationTag
    kNotificationImageReporterTrafficAnnotation =
        net::DefineNetworkTrafficAnnotation("notification_image_reporter", R"(
        semantics {
          sender: "Safe Browsing"
          description:
            "When an Image Notification is show on Android, and the user has "
            "opted into Safe Browsing Extended Reporting, a small fraction of "
            "images from non-whitelisted domains will be uploaded to Google to "
            "look for malicious images."
          trigger:
            "An image notification is triggered, the user is opted-in to "
            "extended reporting, and a random dice-roll picks this image to "
            "report."
          data:
            "The actual image and the origin that triggered the notificaton."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Users can control this feature via the 'Automatically report "
            "details of possible security incidents to Google' setting under "
            "'Privacy'. The feature is disabled by default."
          chrome_policy {
            SafeBrowsingExtendedReportingOptInAllowed {
              policy_options {mode: MANDATORY}
              SafeBrowsingExtendedReportingOptInAllowed: false
            }
          }
        })");

}  // namespace

const char NotificationImageReporter::kReportingUploadUrl[] =
    "https://safebrowsing.googleusercontent.com/safebrowsing/clientreport/"
    "notification-image";

NotificationImageReporter::NotificationImageReporter(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory), weak_factory_(this) {}

NotificationImageReporter::~NotificationImageReporter() {
}

void NotificationImageReporter::ReportNotificationImage(
    Profile* profile,
    const scoped_refptr<SafeBrowsingDatabaseManager>& database_manager,
    const GURL& origin,
    const SkBitmap& image) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(profile);
  DCHECK_EQ(origin, origin.GetOrigin());
  DCHECK(origin.is_valid());

  // Skip whitelisted origins to cut down on report volume.
  if (!database_manager) {
    SkippedReporting();
    return;
  }

  // Query the CSD Whitelist asynchronously on the IO thread.
  base::Callback<void(bool)> result_callback =
      base::Bind(&NotificationImageReporter::OnWhitelistCheckDoneOnIO,
                 weak_factory_.GetWeakPtr(), profile, origin, image);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(WhitelistCheckerClient::StartCheckCsdWhitelist,
                     database_manager, origin, result_callback));
}

void NotificationImageReporter::OnWhitelistCheckDoneOnIO(
    base::WeakPtr<NotificationImageReporter> weak_ptr,
    Profile* profile,
    const GURL& origin,
    const SkBitmap& image,
    bool match_whitelist) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&NotificationImageReporter::OnWhitelistCheckDone, weak_ptr,
                     profile, origin, image, match_whitelist));
}

void NotificationImageReporter::OnWhitelistCheckDone(Profile* profile,
                                                     const GURL& origin,
                                                     const SkBitmap& image,
                                                     bool match_whitelist) {
  if (match_whitelist) {
    SkippedReporting();
    return;
  }

  // Sample a Finch-controlled fraction only.
  double report_chance = GetReportChance();
  if (base::RandDouble() >= report_chance) {
    SkippedReporting();
    return;
  }

  // Avoid exceeding kMaxReportsPerDay.
  base::Time a_day_ago = base::Time::Now() - base::TimeDelta::FromDays(1);
  while (!report_times_.empty() &&
         report_times_.front() < /* older than */ a_day_ago) {
    report_times_.pop();
  }
  if (report_times_.size() >= kMaxReportsPerDay) {
    SkippedReporting();
    return;
  }
  // n.b. we write to report_times_ here even if we'll later end up skipping
  // reporting because GetExtendedReportingLevel was not SBER_LEVEL_SCOUT. That
  // saves us two thread hops, with the downside that we may underreport
  // notifications on the first day that a user opts in to SBER_LEVEL_SCOUT.
  report_times_.push(base::Time::Now());

  // Skip reporting unless SBER2 Scout is enabled.
  if (GetExtendedReportingLevel(*profile->GetPrefs()) != SBER_LEVEL_SCOUT) {
    SkippedReporting();
    return;
  }
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(
          &NotificationImageReporter::DownscaleNotificationImageOnBlockingPool,
          weak_factory_.GetWeakPtr(), origin, image));
}

double NotificationImageReporter::GetReportChance() const {
  // Get the report_chance from the Finch experiment. If there is no active
  // experiment, it will be set to the default of 0.
  double report_chance = variations::GetVariationParamByFeatureAsDouble(
      kNotificationImageReporterFeature, kReportChance, 0.0);

  if (report_chance < 0.0 || report_chance > 1.0) {
    DLOG(WARNING) << "Illegal value " << report_chance << " for the parameter "
                  << kReportChance << ". The value should be between 0 and 1.";
    report_chance = 0.0;
  }

  return report_chance;
}

void NotificationImageReporter::SkippedReporting() {}

// static
void NotificationImageReporter::DownscaleNotificationImageOnBlockingPool(
    const base::WeakPtr<NotificationImageReporter>& weak_ptr,
    const GURL& origin,
    const SkBitmap& image) {
  // Downscale to fit within 512x512. TODO(johnme): Get this from Finch.
  const double MAX_SIZE = 512;
  SkBitmap downscaled_image = image;
  if ((image.width() > MAX_SIZE || image.height() > MAX_SIZE) &&
      image.width() > 0 && image.height() > 0) {
    double scale =
        std::min(MAX_SIZE / image.width(), MAX_SIZE / image.height());
    downscaled_image =
        skia::ImageOperations::Resize(image, skia::ImageOperations::RESIZE_GOOD,
                                      std::lround(scale * image.width()),
                                      std::lround(scale * image.height()));
  }

  // Encode as PNG.
  std::vector<unsigned char> png_bytes;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(downscaled_image, false, &png_bytes)) {
    NOTREACHED();
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &NotificationImageReporter::SendReport, weak_ptr, origin,
          base::RefCountedBytes::TakeVector(&png_bytes),
          gfx::Size(downscaled_image.width(), downscaled_image.height()),
          gfx::Size(image.width(), image.height())));
}

void NotificationImageReporter::SendReport(
    const GURL& origin,
    scoped_refptr<base::RefCountedMemory> data,
    const gfx::Size& dimensions,
    const gfx::Size& original_dimensions) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  NotificationImageReportRequest report;
  report.set_notification_origin(origin.spec());
  report.mutable_image()->set_data(data->front(), data->size());
  report.mutable_image()->set_mime_type(kDefaultMimeType);
  report.mutable_image()->mutable_dimensions()->set_width(dimensions.width());
  report.mutable_image()->mutable_dimensions()->set_height(dimensions.height());
  if (dimensions != original_dimensions) {
    report.mutable_image()->mutable_original_dimensions()->set_width(
        original_dimensions.width());
    report.mutable_image()->mutable_original_dimensions()->set_height(
        original_dimensions.height());
  }

  std::string serialized_report;
  report.SerializeToString(&serialized_report);

  SendReportInternal(GURL(kReportingUploadUrl), "application/octet-stream",
                     serialized_report);
}

void NotificationImageReporter::SendReportInternal(
    const GURL& url,
    const std::string& content_type,
    const std::string& report) {
  ::SendReport(url_loader_factory_, kNotificationImageReporterTrafficAnnotation,
               url, content_type, report,
               base::Bind(&LogReportResult, net::OK, net::HTTP_OK),
               base::Bind(&LogReportResult));
  // TODO(johnme): Consider logging bandwidth and/or duration to UMA.
}

}  // namespace safe_browsing
