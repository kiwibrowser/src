// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/rules_monitor_service.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/declarative_net_request/ruleset_manager.h"
#include "extensions/browser/api/declarative_net_request/ruleset_matcher.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/api/declarative_net_request/utils.h"
#include "extensions/common/extension_id.h"
#include "extensions/common/file_util.h"

namespace extensions {
namespace declarative_net_request {

namespace {

static base::LazyInstance<
    BrowserContextKeyedAPIFactory<RulesMonitorService>>::Leaky g_factory =
    LAZY_INSTANCE_INITIALIZER;

void LoadRulesetOnIOThread(ExtensionId extension_id,
                           std::unique_ptr<RulesetMatcher> ruleset_matcher,
                           InfoMap* info_map) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(info_map);
  info_map->GetRulesetManager()->AddRuleset(extension_id,
                                            std::move(ruleset_matcher));
}

void UnloadRulesetOnIOThread(ExtensionId extension_id, InfoMap* info_map) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(info_map);
  info_map->GetRulesetManager()->RemoveRuleset(extension_id);
}

// Constructs the RulesetMatcher instance for a given extension and forwards the
// same to the IO thread.
void LoadRulesetOnFileTaskRunner(ExtensionId extension_id,
                                 int ruleset_checksum,
                                 base::FilePath indexed_file_path,
                                 scoped_refptr<InfoMap> info_map) {
  base::AssertBlockingAllowed();

  std::unique_ptr<RulesetMatcher> ruleset_matcher;
  RulesetMatcher::LoadRulesetResult result =
      RulesetMatcher::CreateVerifiedMatcher(indexed_file_path, ruleset_checksum,
                                            &ruleset_matcher);
  UMA_HISTOGRAM_ENUMERATION(
      "Extensions.DeclarativeNetRequest.LoadRulesetResult", result,
      RulesetMatcher::kLoadResultMax);
  if (result != RulesetMatcher::kLoadSuccess)
    return;

  base::OnceClosure task = base::BindOnce(
      &LoadRulesetOnIOThread, std::move(extension_id),
      std::move(ruleset_matcher), base::RetainedRef(std::move(info_map)));
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   std::move(task));
}

// Forwards the ruleset unloading to the IO thread.
void UnloadRulesetOnFileTaskRunner(ExtensionId extension_id,
                                   scoped_refptr<InfoMap> info_map) {
  base::AssertBlockingAllowed();

  base::OnceClosure task =
      base::BindOnce(&UnloadRulesetOnIOThread, std::move(extension_id),
                     base::RetainedRef(std::move(info_map)));
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   std::move(task));
}

}  // namespace

// static
BrowserContextKeyedAPIFactory<RulesMonitorService>*
RulesMonitorService::GetFactoryInstance() {
  return g_factory.Pointer();
}

bool RulesMonitorService::HasAnyRegisteredRulesets() const {
  return !extensions_with_rulesets_.empty();
}

bool RulesMonitorService::HasRegisteredRuleset(
    const Extension* extension) const {
  return extensions_with_rulesets_.find(extension) !=
         extensions_with_rulesets_.end();
}

RulesMonitorService::RulesMonitorService(
    content::BrowserContext* browser_context)
    : registry_observer_(this),
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {
  registry_observer_.Add(ExtensionRegistry::Get(browser_context));
}

RulesMonitorService::~RulesMonitorService() = default;

void RulesMonitorService::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  const ExtensionPrefs* prefs = ExtensionPrefs::Get(browser_context);

  int ruleset_checksum;
  if (!prefs->GetDNRRulesetChecksum(extension->id(), &ruleset_checksum))
    return;

  DCHECK(IsAPIAvailable());
  extensions_with_rulesets_.insert(extension);

  base::OnceClosure task = base::BindOnce(
      &LoadRulesetOnFileTaskRunner, extension->id(), ruleset_checksum,
      file_util::GetIndexedRulesetPath(extension->path()),
      base::WrapRefCounted(ExtensionSystem::Get(browser_context)->info_map()));
  file_task_runner_->PostTask(FROM_HERE, std::move(task));
}

void RulesMonitorService::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionReason reason) {
  // Return early if the extension does not have an indexed ruleset.
  if (!extensions_with_rulesets_.erase(extension))
    return;

  DCHECK(IsAPIAvailable());

  // Post the task first to the |file_task_runner_| and then to the IO thread,
  // even though we don't need to do any file IO. Posting the task directly to
  // the IO thread here can lead to RulesetManager::RemoveRuleset being called
  // before a corresponding RulesetManager::AddRuleset.
  base::OnceClosure task = base::BindOnce(
      &UnloadRulesetOnFileTaskRunner, extension->id(),
      base::WrapRefCounted(ExtensionSystem::Get(browser_context)->info_map()));
  file_task_runner_->PostTask(FROM_HERE, std::move(task));
}

}  // namespace declarative_net_request
}  // namespace extensions
