// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/counters/media_licenses_counter.h"

#include <stdint.h>

#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "components/browsing_data/core/pref_names.h"
#include "ppapi/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_PLUGINS)
#include "base/memory/ref_counted.h"
#include "base/task_runner_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "storage/browser/fileapi/file_system_context.h"
#endif  // BUILDFLAG(ENABLE_PLUGINS)

#if defined(OS_ANDROID)
#include "components/cdm/browser/media_drm_storage_impl.h"
#endif  // defined(OS_ANDROID)

namespace {
#if BUILDFLAG(ENABLE_PLUGINS)

// Determining the origins must be run on the file task thread.
std::set<GURL> CountOriginsOnFileTaskRunner(
    storage::FileSystemContext* filesystem_context) {
  DCHECK(filesystem_context->default_file_task_runner()
             ->RunsTasksInCurrentSequence());

  storage::FileSystemBackend* backend =
      filesystem_context->GetFileSystemBackend(
          storage::kFileSystemTypePluginPrivate);
  storage::FileSystemQuotaUtil* quota_util = backend->GetQuotaUtil();

  std::set<GURL> origins;
  quota_util->GetOriginsForTypeOnFileTaskRunner(
      storage::kFileSystemTypePluginPrivate, &origins);
  return origins;
}

// MediaLicensesCounterPlugin is used to determine the number of origins that
// have plugin private filesystem data (used by EME). It does not include
// origins that have content licenses owned by Flash.
class MediaLicensesCounterPlugin : public MediaLicensesCounter {
 public:
  explicit MediaLicensesCounterPlugin(Profile* profile);
  ~MediaLicensesCounterPlugin() override;

 private:
  // BrowsingDataCounter implementation.
  void Count() final;

  // Determining the set of origins used by the plugin private filesystem is
  // done asynchronously. This callback returns the results, which are
  // subsequently reported.
  void OnContentLicensesObtained(const std::set<GURL>& origins);

  base::WeakPtrFactory<MediaLicensesCounterPlugin> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaLicensesCounterPlugin);
};

MediaLicensesCounterPlugin::MediaLicensesCounterPlugin(Profile* profile)
    : MediaLicensesCounter(profile), weak_ptr_factory_(this) {}

MediaLicensesCounterPlugin::~MediaLicensesCounterPlugin() = default;

void MediaLicensesCounterPlugin::Count() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // Cancel existing requests.
  weak_ptr_factory_.InvalidateWeakPtrs();
  scoped_refptr<storage::FileSystemContext> filesystem_context =
      base::WrapRefCounted(
          content::BrowserContext::GetDefaultStoragePartition(profile_)
              ->GetFileSystemContext());
  base::PostTaskAndReplyWithResult(
      filesystem_context->default_file_task_runner(), FROM_HERE,
      base::Bind(&CountOriginsOnFileTaskRunner,
                 base::RetainedRef(filesystem_context)),
      base::Bind(&MediaLicensesCounterPlugin::OnContentLicensesObtained,
                 weak_ptr_factory_.GetWeakPtr()));
}

void MediaLicensesCounterPlugin::OnContentLicensesObtained(
    const std::set<GURL>& origins) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ReportResult(std::make_unique<MediaLicenseResult>(this, origins));
}

#elif defined(OS_ANDROID)

class MediaLicensesCounterAndroid : public MediaLicensesCounter {
 public:
  explicit MediaLicensesCounterAndroid(Profile* profile);
  ~MediaLicensesCounterAndroid() override;

 private:
  // BrowsingDataCounter implementation.
  void Count() final;

  DISALLOW_COPY_AND_ASSIGN(MediaLicensesCounterAndroid);
};

MediaLicensesCounterAndroid::MediaLicensesCounterAndroid(Profile* profile)
    : MediaLicensesCounter(profile) {}

MediaLicensesCounterAndroid::~MediaLicensesCounterAndroid() = default;

void MediaLicensesCounterAndroid::Count() {
  ReportResult(std::make_unique<MediaLicenseResult>(
      this, cdm::MediaDrmStorageImpl::GetAllOrigins(profile_->GetPrefs())));
}

#endif  // defined(OS_ANDROID)
}  // namespace

MediaLicensesCounter::MediaLicenseResult::MediaLicenseResult(
    const MediaLicensesCounter* source,
    const std::set<GURL>& origins)
    : FinishedResult(source, origins.size()) {
  if (!origins.empty())
    one_origin_ = origins.begin()->GetOrigin().host();
}

MediaLicensesCounter::MediaLicenseResult::~MediaLicenseResult() {}

const std::string& MediaLicensesCounter::MediaLicenseResult::GetOneOrigin()
    const {
  return one_origin_;
}

MediaLicensesCounter::MediaLicensesCounter(Profile* profile)
    : profile_(profile) {}

MediaLicensesCounter::~MediaLicensesCounter() {}

const char* MediaLicensesCounter::GetPrefName() const {
  return browsing_data::prefs::kDeleteMediaLicenses;
}

// static
std::unique_ptr<MediaLicensesCounter> MediaLicensesCounter::Create(
    Profile* profile) {
#if BUILDFLAG(ENABLE_PLUGINS)
  return std::make_unique<MediaLicensesCounterPlugin>(profile);
#elif defined(OS_ANDROID)
  return std::make_unique<MediaLicensesCounterAndroid>(profile);
#else
#error "Unsupported configuration"
#endif
}
