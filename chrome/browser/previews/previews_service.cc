// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/previews/previews_service.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/common/chrome_constants.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_features.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/optimization_guide/optimization_guide_service.h"
#include "components/previews/content/previews_io_data.h"
#include "components/previews/content/previews_optimization_guide.h"
#include "components/previews/content/previews_ui_service.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_logger.h"
#include "components/previews/core/previews_opt_out_store.h"
#include "components/previews/core/previews_opt_out_store_sql.h"
#include "content/public/browser/browser_thread.h"

namespace {

// Returns true if previews can be shown for |type|.
bool IsPreviewsTypeEnabled(previews::PreviewsType type) {
  bool server_previews_enabled =
      previews::params::ArePreviewsAllowed() &&
      base::FeatureList::IsEnabled(
          data_reduction_proxy::features::kDataReductionProxyDecidesTransform);
  switch (type) {
    case previews::PreviewsType::OFFLINE:
      return previews::params::IsOfflinePreviewsEnabled();
    case previews::PreviewsType::LOFI:
      return server_previews_enabled || previews::params::IsClientLoFiEnabled();
    case previews::PreviewsType::LITE_PAGE:
      return server_previews_enabled;
    case previews::PreviewsType::AMP_REDIRECTION:
      return previews::params::IsAMPRedirectionPreviewEnabled();
    case previews::PreviewsType::NOSCRIPT:
      return previews::params::IsNoScriptPreviewsEnabled();
    case previews::PreviewsType::UNSPECIFIED:
      // Not a real previews type so treat as false.
      return false;
    case previews::PreviewsType::NONE:
    case previews::PreviewsType::LAST:
      break;
  }
  NOTREACHED();
  return false;
}

// Returns the version of preview treatment |type|. Defaults to 0 if not
// specified in field trial config.
int GetPreviewsTypeVersion(previews::PreviewsType type) {
  switch (type) {
    case previews::PreviewsType::OFFLINE:
      return previews::params::OfflinePreviewsVersion();
    case previews::PreviewsType::LOFI:
      return previews::params::ClientLoFiVersion();
    case previews::PreviewsType::LITE_PAGE:
      return data_reduction_proxy::params::LitePageVersion();
    case previews::PreviewsType::AMP_REDIRECTION:
      return previews::params::AMPRedirectionPreviewsVersion();
    case previews::PreviewsType::NOSCRIPT:
      return previews::params::NoScriptPreviewsVersion();
    case previews::PreviewsType::NONE:
    case previews::PreviewsType::UNSPECIFIED:
    case previews::PreviewsType::LAST:
      break;
  }
  NOTREACHED();
  return -1;
}

// Returns the enabled PreviewsTypes with their version.
std::unique_ptr<previews::PreviewsTypeList> GetEnabledPreviews() {
  std::unique_ptr<previews::PreviewsTypeList> enabled_previews(
      new previews::PreviewsTypeList());

  // Loop across all previews types (relies on sequential enum values).
  for (int i = static_cast<int>(previews::PreviewsType::NONE) + 1;
       i < static_cast<int>(previews::PreviewsType::LAST); ++i) {
    previews::PreviewsType type = static_cast<previews::PreviewsType>(i);
    if (IsPreviewsTypeEnabled(type))
      enabled_previews->push_back({type, GetPreviewsTypeVersion(type)});
  }
  return enabled_previews;
}

}  // namespace

PreviewsService::PreviewsService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

PreviewsService::~PreviewsService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

void PreviewsService::Initialize(
    previews::PreviewsIOData* previews_io_data,
    optimization_guide::OptimizationGuideService* optimization_guide_service,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    const base::FilePath& profile_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Get the background thread to run SQLite on.
  scoped_refptr<base::SequencedTaskRunner> background_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});

  previews_ui_service_ = std::make_unique<previews::PreviewsUIService>(
      previews_io_data, io_task_runner,
      std::make_unique<previews::PreviewsOptOutStoreSQL>(
          io_task_runner, background_task_runner,
          profile_path.Append(chrome::kPreviewsOptOutDBFilename),
          GetEnabledPreviews()),
      optimization_guide_service
          ? std::make_unique<previews::PreviewsOptimizationGuide>(
                optimization_guide_service, io_task_runner)
          : nullptr,
      base::Bind(&IsPreviewsTypeEnabled),
      std::make_unique<previews::PreviewsLogger>());
}
