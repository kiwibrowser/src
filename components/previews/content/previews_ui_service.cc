// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/content/previews_ui_service.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "url/gurl.h"

namespace previews {

PreviewsUIService::PreviewsUIService(
    PreviewsIOData* previews_io_data,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    std::unique_ptr<PreviewsOptOutStore> previews_opt_out_store,
    std::unique_ptr<PreviewsOptimizationGuide> previews_opt_guide,
    const PreviewsIsEnabledCallback& is_enabled_callback,
    std::unique_ptr<PreviewsLogger> logger)
    : io_task_runner_(io_task_runner),
      logger_(std::move(logger)),
      weak_factory_(this) {
  DCHECK(logger_);
  previews_io_data->Initialize(
      weak_factory_.GetWeakPtr(), std::move(previews_opt_out_store),
      std::move(previews_opt_guide), is_enabled_callback);
}

PreviewsUIService::~PreviewsUIService() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void PreviewsUIService::SetIOData(base::WeakPtr<PreviewsIOData> io_data) {
  DCHECK(thread_checker_.CalledOnValidThread());
  io_data_ = io_data;
}

void PreviewsUIService::AddPreviewNavigation(const GURL& url,
                                             PreviewsType type,
                                             bool opt_out,
                                             uint64_t page_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  io_task_runner_->PostTask(
      FROM_HERE, base::Bind(&PreviewsIOData::AddPreviewNavigation, io_data_,
                            url, opt_out, type, page_id));
}

void PreviewsUIService::LogPreviewNavigation(const GURL& url,
                                             PreviewsType type,
                                             bool opt_out,
                                             base::Time time,
                                             uint64_t page_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  logger_->LogPreviewNavigation(url, type, opt_out, time, page_id);
}

void PreviewsUIService::LogPreviewDecisionMade(
    PreviewsEligibilityReason reason,
    const GURL& url,
    base::Time time,
    PreviewsType type,
    std::vector<PreviewsEligibilityReason>&& passed_reasons,
    uint64_t page_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  logger_->LogPreviewDecisionMade(reason, url, time, type,
                                  std::move(passed_reasons), page_id);
}

void PreviewsUIService::OnNewBlacklistedHost(const std::string& host,
                                             base::Time time) {
  DCHECK(thread_checker_.CalledOnValidThread());
  logger_->OnNewBlacklistedHost(host, time);
}

void PreviewsUIService::OnUserBlacklistedStatusChange(bool blacklisted) {
  DCHECK(thread_checker_.CalledOnValidThread());
  logger_->OnUserBlacklistedStatusChange(blacklisted);
}

void PreviewsUIService::OnBlacklistCleared(base::Time time) {
  DCHECK(thread_checker_.CalledOnValidThread());
  logger_->OnBlacklistCleared(time);
}

void PreviewsUIService::SetIgnorePreviewsBlacklistDecision(bool ignored) {
  DCHECK(thread_checker_.CalledOnValidThread());
  io_task_runner_->PostTask(
      FROM_HERE, base::Bind(&PreviewsIOData::SetIgnorePreviewsBlacklistDecision,
                            io_data_, ignored));
}

void PreviewsUIService::OnIgnoreBlacklistDecisionStatusChanged(bool ignored) {
  DCHECK(thread_checker_.CalledOnValidThread());
  logger_->OnIgnoreBlacklistDecisionStatusChanged(ignored);
}

PreviewsLogger* PreviewsUIService::previews_logger() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return logger_.get();
}

void PreviewsUIService::ClearBlackList(base::Time begin_time,
                                       base::Time end_time) {
  DCHECK(thread_checker_.CalledOnValidThread());
  io_task_runner_->PostTask(
      FROM_HERE, base::Bind(&PreviewsIOData::ClearBlackList, io_data_,
                            begin_time, end_time));
}

}  // namespace previews
