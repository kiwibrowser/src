// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/content_ruleset_service.h"

#include <utility>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "components/subresource_filter/content/common/subresource_filter_messages.h"
#include "components/subresource_filter/core/browser/ruleset_service.h"
#include "components/subresource_filter/core/common/common_features.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "ipc/ipc_platform_file.h"

namespace subresource_filter {

namespace {

void SendRulesetToRenderProcess(base::File* file,
                                content::RenderProcessHost* rph) {
  DCHECK(rph);
  DCHECK(file);
  DCHECK(file->IsValid());
  rph->Send(new SubresourceFilterMsg_SetRulesetForProcess(
      IPC::TakePlatformFileForTransit(file->Duplicate())));
}

// The file handle is closed when the argument goes out of scope.
void CloseFile(base::File) {}

// Posts the |file| handle to the file thread so it can be closed.
void CloseFileOnFileThread(base::File* file) {
  if (!file->IsValid())
    return;
  base::PostTaskWithTraits(FROM_HERE,
                           {base::TaskPriority::BACKGROUND, base::MayBlock()},
                           base::BindOnce(&CloseFile, std::move(*file)));
}

}  // namespace

ContentRulesetService::ContentRulesetService(
    scoped_refptr<base::SequencedTaskRunner> blocking_task_runner)
    : ruleset_dealer_(std::make_unique<VerifiedRulesetDealer::Handle>(
          std::move(blocking_task_runner))) {
  // Must rely on notifications as RenderProcessHostObserver::RenderProcessReady
  // would only be called after queued IPC messages (potentially triggering a
  // navigation) had already been sent to the new renderer.
  notification_registrar_.Add(
      this, content::NOTIFICATION_RENDERER_PROCESS_CREATED,
      content::NotificationService::AllBrowserContextsAndSources());
}

ContentRulesetService::~ContentRulesetService() {
  CloseFileOnFileThread(&ruleset_data_);
}

void ContentRulesetService::SetRulesetPublishedCallbackForTesting(
    base::Closure callback) {
  ruleset_published_callback_ = callback;
}

void ContentRulesetService::PostAfterStartupTask(base::Closure task) {
  content::BrowserThread::PostAfterStartupTask(
      FROM_HERE,
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI),
      task);
}

void ContentRulesetService::TryOpenAndSetRulesetFile(
    const base::FilePath& file_path,
    base::OnceCallback<void(base::File)> callback) {
  ruleset_dealer_->TryOpenAndSetRulesetFile(file_path, std::move(callback));
}

void ContentRulesetService::PublishNewRulesetVersion(base::File ruleset_data) {
  DCHECK(ruleset_data.IsValid());
  CloseFileOnFileThread(&ruleset_data_);

  // If Ad Tagging is running, then every request does a lookup and it's
  // important that we verify the ruleset early on.
  if (base::FeatureList::IsEnabled(kAdTagging)) {
    // Even though the handle will immediately be destroyed, it will still
    // validate the ruleset on its task runner.
    VerifiedRuleset::Handle ruleset_handle(ruleset_dealer_.get());
  }

  ruleset_data_ = std::move(ruleset_data);
  for (auto it = content::RenderProcessHost::AllHostsIterator(); !it.IsAtEnd();
       it.Advance()) {
    SendRulesetToRenderProcess(&ruleset_data_, it.GetCurrentValue());
  }

  if (!ruleset_published_callback_.is_null())
    ruleset_published_callback_.Run();
}

void ContentRulesetService::set_ruleset_service(
    std::unique_ptr<RulesetService> ruleset_service) {
  ruleset_service_ = std::move(ruleset_service);
}

void ContentRulesetService::IndexAndStoreAndPublishRulesetIfNeeded(
    const UnindexedRulesetInfo& unindexed_ruleset_info) {
  DCHECK(ruleset_service_);
  ruleset_service_->IndexAndStoreAndPublishRulesetIfNeeded(
      unindexed_ruleset_info);
}

void ContentRulesetService::SetIsAfterStartupForTesting() {
  DCHECK(ruleset_service_);
  ruleset_service_->set_is_after_startup_for_testing();
}

void ContentRulesetService::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(type, content::NOTIFICATION_RENDERER_PROCESS_CREATED);
  if (!ruleset_data_.IsValid())
    return;
  SendRulesetToRenderProcess(
      &ruleset_data_,
      content::Source<content::RenderProcessHost>(source).ptr());
}

}  // namespace subresource_filter
