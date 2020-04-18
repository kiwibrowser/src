// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/update_engine.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/guid.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/component.h"
#include "components/update_client/configurator.h"
#include "components/update_client/crx_update_item.h"
#include "components/update_client/persisted_data.h"
#include "components/update_client/update_checker.h"
#include "components/update_client/update_client_errors.h"
#include "components/update_client/utils.h"

namespace update_client {

UpdateContext::UpdateContext(
    scoped_refptr<Configurator> config,
    bool is_foreground,
    const std::vector<std::string>& ids,
    UpdateClient::CrxDataCallback crx_data_callback,
    const UpdateEngine::NotifyObserversCallback& notify_observers_callback,
    UpdateEngine::Callback callback,
    CrxDownloader::Factory crx_downloader_factory)
    : config(config),
      is_foreground(is_foreground),
      enabled_component_updates(config->EnabledComponentUpdates()),
      ids(ids),
      crx_data_callback(std::move(crx_data_callback)),
      notify_observers_callback(notify_observers_callback),
      callback(std::move(callback)),
      crx_downloader_factory(crx_downloader_factory),
      session_id(base::GenerateGUID()) {
  for (const auto& id : ids)
    components.insert(
        std::make_pair(id, std::make_unique<Component>(*this, id)));
}

UpdateContext::~UpdateContext() {}

UpdateEngine::UpdateEngine(
    scoped_refptr<Configurator> config,
    UpdateChecker::Factory update_checker_factory,
    CrxDownloader::Factory crx_downloader_factory,
    scoped_refptr<PingManager> ping_manager,
    const NotifyObserversCallback& notify_observers_callback)
    : config_(config),
      update_checker_factory_(update_checker_factory),
      crx_downloader_factory_(crx_downloader_factory),
      ping_manager_(ping_manager),
      metadata_(
          std::make_unique<PersistedData>(config->GetPrefService(),
                                          config->GetActivityDataService())),
      notify_observers_callback_(notify_observers_callback) {}

UpdateEngine::~UpdateEngine() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void UpdateEngine::Update(bool is_foreground,
                          const std::vector<std::string>& ids,
                          UpdateClient::CrxDataCallback crx_data_callback,
                          Callback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (ids.empty()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), Error::INVALID_ARGUMENT));
    return;
  }

  if (IsThrottled(is_foreground)) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), Error::RETRY_LATER));
    return;
  }

  const auto update_context = base::MakeRefCounted<UpdateContext>(
      config_, is_foreground, ids, std::move(crx_data_callback),
      notify_observers_callback_, std::move(callback), crx_downloader_factory_);
  DCHECK(!update_context->session_id.empty());

  const auto result = update_contexts_.insert(
      std::make_pair(update_context->session_id, update_context));
  DCHECK(result.second);

  // Calls out to get the corresponding CrxComponent data for the CRXs in this
  // update context.
  std::vector<std::unique_ptr<CrxComponent>> crx_components =
      std::move(update_context->crx_data_callback).Run(update_context->ids);
  DCHECK_EQ(update_context->ids.size(), crx_components.size());

  for (size_t i = 0; i != update_context->ids.size(); ++i) {
    const auto& id = update_context->ids[i];

    DCHECK(update_context->components[id]->state() == ComponentState::kNew);

    auto& crx_component = crx_components[i];
    if (crx_component) {
      // This component can be checked for updates.
      DCHECK_EQ(id, GetCrxComponentID(*crx_component));
      auto& component = update_context->components[id];
      component->set_crx_component(std::move(crx_component));
      component->set_previous_version(component->crx_component()->version);
      component->set_previous_fp(component->crx_component()->fingerprint);
      update_context->components_to_check_for_updates.push_back(id);
    } else {
      // |CrxDataCallback| did not return a CrxComponent instance for this
      // component, which most likely, has been uninstalled. This component
      // is going to be transitioned to an error state when the its |Handle|
      // method is called later on by the engine.
      update_context->component_queue.push(id);
    }
  }

  if (update_context->components_to_check_for_updates.empty()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&UpdateEngine::HandleComponent,
                                  base::Unretained(this), update_context));
    return;
  }

  for (const auto& id : update_context->components_to_check_for_updates)
    update_context->components[id]->Handle(
        base::BindOnce(&UpdateEngine::ComponentCheckingForUpdatesStart,
                       base::Unretained(this), update_context, id));
}

void UpdateEngine::ComponentCheckingForUpdatesStart(
    scoped_refptr<UpdateContext> update_context,
    const std::string& id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(update_context);

  DCHECK_EQ(1u, update_context->components.count(id));
  DCHECK(update_context->components.at(id));

  // Handle |kChecking| state.
  auto& component = *update_context->components.at(id);
  component.Handle(
      base::BindOnce(&UpdateEngine::ComponentCheckingForUpdatesComplete,
                     base::Unretained(this), update_context));

  ++update_context->num_components_ready_to_check;
  if (update_context->num_components_ready_to_check <
      update_context->components_to_check_for_updates.size()) {
    return;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&UpdateEngine::DoUpdateCheck,
                                base::Unretained(this), update_context));
}

void UpdateEngine::DoUpdateCheck(scoped_refptr<UpdateContext> update_context) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(update_context);

  update_context->update_checker =
      update_checker_factory_(config_, metadata_.get());

  update_context->update_checker->CheckForUpdates(
      update_context->session_id,
      update_context->components_to_check_for_updates,
      update_context->components, config_->ExtraRequestParams(),
      update_context->enabled_component_updates,
      base::BindOnce(&UpdateEngine::UpdateCheckDone, base::Unretained(this),
                     update_context));
}

void UpdateEngine::UpdateCheckDone(scoped_refptr<UpdateContext> update_context,
                                   int error,
                                   int retry_after_sec) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(update_context);

  update_context->retry_after_sec = retry_after_sec;

  const int throttle_sec(update_context->retry_after_sec);
  DCHECK_LE(throttle_sec, 24 * 60 * 60);

  // Only positive values for throttle_sec are effective. 0 means that no
  // throttling occurs and has the effect of resetting the member.
  // Negative values are not trusted and are ignored.
  if (throttle_sec >= 0) {
    throttle_updates_until_ =
        throttle_sec ? base::TimeTicks::Now() +
                           base::TimeDelta::FromSeconds(throttle_sec)
                     : base::TimeTicks();
  }

  update_context->update_check_error = error;

  for (const auto& id : update_context->components_to_check_for_updates) {
    DCHECK_EQ(1u, update_context->components.count(id));
    DCHECK(update_context->components.at(id));

    auto& component = *update_context->components.at(id);
    component.UpdateCheckComplete();
  }
}

void UpdateEngine::ComponentCheckingForUpdatesComplete(
    scoped_refptr<UpdateContext> update_context) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(update_context);

  ++update_context->num_components_checked;
  if (update_context->num_components_checked <
      update_context->components_to_check_for_updates.size()) {
    return;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&UpdateEngine::UpdateCheckComplete,
                                base::Unretained(this), update_context));
}

void UpdateEngine::UpdateCheckComplete(
    scoped_refptr<UpdateContext> update_context) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(update_context);

  for (const auto& id : update_context->components_to_check_for_updates)
    update_context->component_queue.push(id);

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&UpdateEngine::HandleComponent,
                                base::Unretained(this), update_context));
}

void UpdateEngine::HandleComponent(
    scoped_refptr<UpdateContext> update_context) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(update_context);

  auto& queue = update_context->component_queue;

  if (queue.empty()) {
    const Error error = update_context->update_check_error
                            ? Error::UPDATE_CHECK_ERROR
                            : Error::NONE;

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&UpdateEngine::UpdateComplete, base::Unretained(this),
                       update_context, error));
    return;
  }

  const auto& id = queue.front();
  DCHECK_EQ(1u, update_context->components.count(id));
  const auto& component = update_context->components.at(id);
  DCHECK(component);

  auto& next_update_delay = update_context->next_update_delay;
  if (!next_update_delay.is_zero() && component->IsUpdateAvailable()) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&UpdateEngine::HandleComponent, base::Unretained(this),
                       update_context),
        next_update_delay);
    next_update_delay = base::TimeDelta();

    notify_observers_callback_.Run(
        UpdateClient::Observer::Events::COMPONENT_WAIT, id);
    return;
  }

  component->Handle(base::BindOnce(&UpdateEngine::HandleComponentComplete,
                                   base::Unretained(this), update_context));
}

void UpdateEngine::HandleComponentComplete(
    scoped_refptr<UpdateContext> update_context) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(update_context);

  auto& queue = update_context->component_queue;
  DCHECK(!queue.empty());

  const auto& id = queue.front();
  DCHECK_EQ(1u, update_context->components.count(id));
  const auto& component = update_context->components.at(id);
  DCHECK(component);

  if (component->IsHandled()) {
    update_context->next_update_delay = component->GetUpdateDuration();

    if (!component->events().empty()) {
      ping_manager_->SendPing(*component,
                              base::BindOnce([](int, const std::string&) {}));
    }

    queue.pop();
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&UpdateEngine::HandleComponent,
                                base::Unretained(this), update_context));
}

void UpdateEngine::UpdateComplete(scoped_refptr<UpdateContext> update_context,
                                  Error error) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(update_context);

  const auto num_erased = update_contexts_.erase(update_context->session_id);
  DCHECK_EQ(1u, num_erased);

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(update_context->callback), error));
}

bool UpdateEngine::GetUpdateState(const std::string& id,
                                  CrxUpdateItem* update_item) {
  DCHECK(thread_checker_.CalledOnValidThread());
  for (const auto& context : update_contexts_) {
    const auto& components = context.second->components;
    const auto it = components.find(id);
    if (it != components.end() && it->second->crx_component()) {
      *update_item = it->second->GetCrxUpdateItem();
      return true;
    }
  }
  return false;
}

bool UpdateEngine::IsThrottled(bool is_foreground) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (is_foreground || throttle_updates_until_.is_null())
    return false;

  const auto now(base::TimeTicks::Now());

  // Throttle the calls in the interval (t - 1 day, t) to limit the effect of
  // unset clocks or clock drift.
  return throttle_updates_until_ - base::TimeDelta::FromDays(1) < now &&
         now < throttle_updates_until_;
}

void UpdateEngine::SendUninstallPing(const std::string& id,
                                     const base::Version& version,
                                     int reason,
                                     Callback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  const auto update_context = base::MakeRefCounted<UpdateContext>(
      config_, false, std::vector<std::string>{id},
      UpdateClient::CrxDataCallback(), UpdateEngine::NotifyObserversCallback(),
      std::move(callback), nullptr);
  DCHECK(!update_context->session_id.empty());

  const auto result = update_contexts_.insert(
      std::make_pair(update_context->session_id, update_context));
  DCHECK(result.second);

  DCHECK(update_context);
  DCHECK_EQ(1u, update_context->ids.size());
  DCHECK_EQ(1u, update_context->components.count(id));
  const auto& component = update_context->components.at(id);

  component->Uninstall(version, reason);

  update_context->component_queue.push(id);

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&UpdateEngine::HandleComponent,
                                base::Unretained(this), update_context));
}

}  // namespace update_client
