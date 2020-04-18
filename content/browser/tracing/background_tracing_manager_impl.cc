// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/background_tracing_manager_impl.h"

#include <utility>

#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/values.h"
#include "content/browser/tracing/background_memory_tracing_observer.h"
#include "content/browser/tracing/background_tracing_rule.h"
#include "content/browser/tracing/trace_message_filter.h"
#include "content/browser/tracing/tracing_controller_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/tracing_delegate.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "services/tracing/public/cpp/trace_event_agent.h"

namespace content {

namespace {

base::LazyInstance<BackgroundTracingManagerImpl>::Leaky
    g_background_tracing_manager = LAZY_INSTANCE_INITIALIZER;

// These values are used for a histogram. Do not reorder.
enum BackgroundTracingMetrics {
  SCENARIO_ACTIVATION_REQUESTED = 0,
  SCENARIO_ACTIVATED_SUCCESSFULLY = 1,
  RECORDING_ENABLED = 2,
  PREEMPTIVE_TRIGGERED = 3,
  REACTIVE_TRIGGERED = 4,
  FINALIZATION_ALLOWED = 5,
  FINALIZATION_DISALLOWED = 6,
  FINALIZATION_STARTED = 7,
  OBSOLETE_FINALIZATION_COMPLETE = 8,
  SCENARIO_ACTION_FAILED_LOWRES_CLOCK = 9,
  UPLOAD_FAILED = 10,
  UPLOAD_SUCCEEDED = 11,
  NUMBER_OF_BACKGROUND_TRACING_METRICS,
};

void RecordBackgroundTracingMetric(BackgroundTracingMetrics metric) {
  UMA_HISTOGRAM_ENUMERATION("Tracing.Background.ScenarioState", metric,
                            NUMBER_OF_BACKGROUND_TRACING_METRICS);
}

}  // namespace

BackgroundTracingManagerImpl::TracingTimer::TracingTimer(
    StartedFinalizingCallback callback) : callback_(callback) {
}

BackgroundTracingManagerImpl::TracingTimer::~TracingTimer() {
}

void BackgroundTracingManagerImpl::TracingTimer::StartTimer(int seconds) {
  tracing_timer_.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(seconds), this,
      &BackgroundTracingManagerImpl::TracingTimer::TracingTimerFired);
}

void BackgroundTracingManagerImpl::TracingTimer::CancelTimer() {
  tracing_timer_.Stop();
}

void BackgroundTracingManagerImpl::TracingTimer::TracingTimerFired() {
  BackgroundTracingManagerImpl::GetInstance()->BeginFinalizing(callback_);
}

void BackgroundTracingManagerImpl::TracingTimer::FireTimerForTesting() {
  CancelTimer();
  TracingTimerFired();
}

BackgroundTracingManager* BackgroundTracingManager::GetInstance() {
  return BackgroundTracingManagerImpl::GetInstance();
}

BackgroundTracingManagerImpl* BackgroundTracingManagerImpl::GetInstance() {
  return g_background_tracing_manager.Pointer();
}

BackgroundTracingManagerImpl::BackgroundTracingManagerImpl()
    : delegate_(GetContentClient()->browser()->GetTracingDelegate()),
      is_gathering_(false),
      is_tracing_(false),
      requires_anonymized_data_(true),
      trigger_handle_ids_(0),
      triggered_named_event_handle_(-1) {
  AddEnabledStateObserver(BackgroundMemoryTracingObserver::GetInstance());
}

BackgroundTracingManagerImpl::~BackgroundTracingManagerImpl() {
  NOTREACHED();
}

void BackgroundTracingManagerImpl::AddMetadataGeneratorFunction() {
  TracingControllerImpl::GetInstance()
      ->GetTraceEventAgent()
      ->AddMetadataGeneratorFunction(base::BindRepeating(
          &BackgroundTracingManagerImpl::GenerateMetadataDict,
          base::Unretained(this)));
}

void BackgroundTracingManagerImpl::WhenIdle(
    base::Callback<void()> idle_callback) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  idle_callback_ = std::move(idle_callback);
}

bool BackgroundTracingManagerImpl::SetActiveScenario(
    std::unique_ptr<BackgroundTracingConfig> config,
    BackgroundTracingManager::ReceiveCallback receive_callback,
    DataFiltering data_filtering) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  RecordBackgroundTracingMetric(SCENARIO_ACTIVATION_REQUESTED);

  if (is_tracing_)
    return false;

  // If we don't have a high resolution timer available, traces will be
  // too inaccurate to be useful.
  if (!base::TimeTicks::IsHighResolution()) {
    RecordBackgroundTracingMetric(SCENARIO_ACTION_FAILED_LOWRES_CLOCK);
    return false;
  }

  bool requires_anonymized_data = (data_filtering == ANONYMIZE_DATA);

  // If the profile hasn't loaded or been created yet, this is a startup
  // scenario and we have to wait until initialization is finished to validate
  // that the scenario can run.
  if (!delegate_ || delegate_->IsProfileLoaded()) {
    // TODO(oysteine): Retry when time_until_allowed has elapsed.
    if (config && delegate_ &&
        !delegate_->IsAllowedToBeginBackgroundScenario(
            *config.get(), requires_anonymized_data)) {
      return false;
    }
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&BackgroundTracingManagerImpl::ValidateStartupScenario,
                       base::Unretained(this)));
  }

  std::unique_ptr<const content::BackgroundTracingConfigImpl> config_impl(
      static_cast<BackgroundTracingConfigImpl*>(config.release()));

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  if (config_impl) {
    // No point in tracing if there's nowhere to send it.
    if (receive_callback.is_null())
      return false;

    // If the scenario requires us to toggle Blink features, we want
    // to neither override anything else nor to do we want to activate
    // the scenario without doing the toggle, so if something else has
    // configured these switches we just abort.
    if (!config_impl->enable_blink_features().empty() &&
        command_line->HasSwitch(switches::kEnableBlinkFeatures)) {
      return false;
    }

    if (!config_impl->disable_blink_features().empty() &&
        command_line->HasSwitch(switches::kDisableBlinkFeatures)) {
      return false;
    }
  }

  config_ = std::move(config_impl);
  receive_callback_ = std::move(receive_callback);
  requires_anonymized_data_ = requires_anonymized_data;

  if (config_) {
    DCHECK(!config_->rules().empty());
    for (const auto& rule : config_->rules())
      rule->Install();

    if (!config_->enable_blink_features().empty()) {
      command_line->AppendSwitchASCII(switches::kEnableBlinkFeatures,
                                      config_->enable_blink_features());
    }

    if (!config_->disable_blink_features().empty()) {
      command_line->AppendSwitchASCII(switches::kDisableBlinkFeatures,
                                      config_->disable_blink_features());
    }
  }

  // Notify observers before starting tracing.
  for (auto* observer : background_tracing_observers_)
    observer->OnScenarioActivated(config_.get());

  StartTracingIfConfigNeedsIt();
  RecordBackgroundTracingMetric(SCENARIO_ACTIVATED_SUCCESSFULLY);
  return true;
}

bool BackgroundTracingManagerImpl::HasActiveScenario() {
  return !!config_;
}

void BackgroundTracingManagerImpl::OnStartTracingDone(
    BackgroundTracingConfigImpl::CategoryPreset preset) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto* observer : background_tracing_observers_)
    observer->OnTracingEnabled(preset);
}

void BackgroundTracingManagerImpl::AddEnabledStateObserver(
    EnabledStateObserver* observer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  background_tracing_observers_.insert(observer);
}

void BackgroundTracingManagerImpl::RemoveEnabledStateObserver(
    EnabledStateObserver* observer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  background_tracing_observers_.erase(observer);
}

void BackgroundTracingManagerImpl::AddTraceMessageFilter(
    TraceMessageFilter* trace_message_filter) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  trace_message_filters_.insert(trace_message_filter);

  for (auto* observer : trace_message_filter_observers_)
    observer->OnTraceMessageFilterAdded(trace_message_filter);
}

void BackgroundTracingManagerImpl::RemoveTraceMessageFilter(
    TraceMessageFilter* trace_message_filter) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto* observer : trace_message_filter_observers_)
    observer->OnTraceMessageFilterRemoved(trace_message_filter);

  trace_message_filters_.erase(trace_message_filter);
}

void BackgroundTracingManagerImpl::AddTraceMessageFilterObserver(
    TraceMessageFilterObserver* observer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  trace_message_filter_observers_.insert(observer);

  for (auto& filter : trace_message_filters_)
    observer->OnTraceMessageFilterAdded(filter.get());
}

void BackgroundTracingManagerImpl::RemoveTraceMessageFilterObserver(
    TraceMessageFilterObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  trace_message_filter_observers_.erase(observer);

  for (auto& filter : trace_message_filters_)
    observer->OnTraceMessageFilterRemoved(filter.get());
}

bool BackgroundTracingManagerImpl::IsTracingForTesting() {
  return is_tracing_;
}

void BackgroundTracingManagerImpl::ValidateStartupScenario() {
  if (!config_ || !delegate_)
    return;

  if (!delegate_->IsAllowedToBeginBackgroundScenario(
          *config_.get(), requires_anonymized_data_)) {
    AbortScenario();
  }
}

void BackgroundTracingManagerImpl::StartTracingIfConfigNeedsIt() {
  if (!config_)
    return;

  if (config_->tracing_mode() == BackgroundTracingConfigImpl::PREEMPTIVE) {
    StartTracing(config_->category_preset(),
                 base::trace_event::RECORD_CONTINUOUSLY);
  }
  // There is nothing to do in case of reactive tracing.
}

BackgroundTracingRule*
BackgroundTracingManagerImpl::GetRuleAbleToTriggerTracing(
    TriggerHandle handle) const {
  if (!config_)
    return nullptr;

  // If the last trace is still uploading, we don't allow a new one to trigger.
  if (is_gathering_)
    return nullptr;

  if (!IsTriggerHandleValid(handle)) {
    return nullptr;
  }

  std::string trigger_name = GetTriggerNameFromHandle(handle);
  for (const auto& rule : config_->rules()) {
    if (rule->ShouldTriggerNamedEvent(trigger_name))
      return rule.get();
  }

  return nullptr;
}

void BackgroundTracingManagerImpl::OnHistogramTrigger(
    const std::string& histogram_name) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&BackgroundTracingManagerImpl::OnHistogramTrigger,
                       base::Unretained(this), histogram_name));
    return;
  }

  if (!config_)
    return;

  for (const auto& rule : config_->rules()) {
    if (rule->ShouldTriggerNamedEvent(histogram_name))
      OnRuleTriggered(rule.get(), StartedFinalizingCallback());
  }
}

void BackgroundTracingManagerImpl::TriggerNamedEvent(
    BackgroundTracingManagerImpl::TriggerHandle handle,
    StartedFinalizingCallback callback) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&BackgroundTracingManagerImpl::TriggerNamedEvent,
                       base::Unretained(this), handle, std::move(callback)));
    return;
  }

  bool is_valid_trigger = true;

  const BackgroundTracingRule* triggered_rule =
      GetRuleAbleToTriggerTracing(handle);
  if (!triggered_rule)
    is_valid_trigger = false;

  // A different reactive config than the running one tried to trigger.
  if (!config_ ||
      (config_->tracing_mode() == BackgroundTracingConfigImpl::REACTIVE &&
       is_tracing_ && triggered_named_event_handle_ != handle)) {
    is_valid_trigger = false;
  }

  if (!is_valid_trigger) {
    if (!callback.is_null())
      std::move(callback).Run(false);
    return;
  }

  triggered_named_event_handle_ = handle;
  OnRuleTriggered(triggered_rule, std::move(callback));
}

void BackgroundTracingManagerImpl::OnRuleTriggered(
    const BackgroundTracingRule* triggered_rule,
    StartedFinalizingCallback callback) {
  CHECK(config_);

  double trigger_chance = triggered_rule->trigger_chance();
  if (trigger_chance < 1.0 && base::RandDouble() > trigger_chance) {
    if (!callback.is_null())
      std::move(callback).Run(false);
    return;
  }

  last_triggered_rule_.reset(new base::DictionaryValue);
  triggered_rule->IntoDict(last_triggered_rule_.get());
  int trace_delay = triggered_rule->GetTraceDelay();

  if (config_->tracing_mode() == BackgroundTracingConfigImpl::REACTIVE) {
    // In reactive mode, a trigger starts tracing, or finalizes tracing
    // immediately if it's already running.
    RecordBackgroundTracingMetric(REACTIVE_TRIGGERED);

    if (!is_tracing_) {
      // It was not already tracing, start a new trace.
      StartTracing(triggered_rule->category_preset(),
                   base::trace_event::RECORD_UNTIL_FULL);
    } else {
      // Some reactive configs that trigger again while tracing should just
      // end right away (to not capture multiple navigations, for example).
      // For others we just want to ignore the repeated trigger.
      if (triggered_rule->stop_tracing_on_repeated_reactive()) {
        trace_delay = -1;
      } else {
        if (!callback.is_null())
          std::move(callback).Run(false);
        return;
      }
    }
  } else {
    // In preemptive mode, a trigger starts finalizing a trace if one is
    // running and we're not got a finalization timer running,
    // otherwise we do nothing.
    if (!is_tracing_ || is_gathering_ || tracing_timer_) {
      if (!callback.is_null())
        std::move(callback).Run(false);
      return;
    }

    RecordBackgroundTracingMetric(PREEMPTIVE_TRIGGERED);
  }

  if (trace_delay < 0) {
    BeginFinalizing(std::move(callback));
  } else {
    tracing_timer_.reset(new TracingTimer(std::move(callback)));
    tracing_timer_->StartTimer(trace_delay);
  }

  if (!rule_triggered_callback_for_testing_.is_null())
    rule_triggered_callback_for_testing_.Run();
}

BackgroundTracingManagerImpl::TriggerHandle
BackgroundTracingManagerImpl::RegisterTriggerType(const char* trigger_name) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  trigger_handle_ids_ += 1;

  trigger_handles_.insert(
      std::pair<TriggerHandle, std::string>(trigger_handle_ids_, trigger_name));

  return static_cast<TriggerHandle>(trigger_handle_ids_);
}

bool BackgroundTracingManagerImpl::IsTriggerHandleValid(
    BackgroundTracingManager::TriggerHandle handle) const {
  return trigger_handles_.find(handle) != trigger_handles_.end();
}

std::string BackgroundTracingManagerImpl::GetTriggerNameFromHandle(
    BackgroundTracingManager::TriggerHandle handle) const {
  CHECK(IsTriggerHandleValid(handle));
  return trigger_handles_.find(handle)->second;
}

void BackgroundTracingManagerImpl::InvalidateTriggerHandlesForTesting() {
  trigger_handles_.clear();
}

void BackgroundTracingManagerImpl::SetRuleTriggeredCallbackForTesting(
    const base::Closure& callback) {
  rule_triggered_callback_for_testing_ = callback;
};

void BackgroundTracingManagerImpl::FireTimerForTesting() {
  DCHECK(tracing_timer_);
  tracing_timer_->FireTimerForTesting();
}

void BackgroundTracingManagerImpl::StartTracing(
    BackgroundTracingConfigImpl::CategoryPreset preset,
    base::trace_event::TraceRecordMode record_mode) {
  base::trace_event::TraceConfig trace_config(
      GetCategoryFilterStringForCategoryPreset(preset), record_mode);
  if (requires_anonymized_data_)
    trace_config.EnableArgumentFilter();

  if (preset ==
      BackgroundTracingConfigImpl::CategoryPreset::BENCHMARK_MEMORY_LIGHT) {
    // On memory light mode, the periodic memory dumps are disabled.
    // TODO(ssid): Remove this when memory-infra supports peak detection
    // crbug.com/609935.
    base::trace_event::TraceConfig::MemoryDumpConfig memory_config;
    memory_config.allowed_dump_modes =
        std::set<base::trace_event::MemoryDumpLevelOfDetail>(
            {base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND});
    trace_config.ResetMemoryDumpConfig(memory_config);
  }

  is_tracing_ = TracingControllerImpl::GetInstance()->StartTracing(
      trace_config,
      base::Bind(&BackgroundTracingManagerImpl::OnStartTracingDone,
                 base::Unretained(this), preset));
  RecordBackgroundTracingMetric(RECORDING_ENABLED);
}

void BackgroundTracingManagerImpl::OnFinalizeStarted(
    base::Closure started_finalizing_closure,
    std::unique_ptr<const base::DictionaryValue> metadata,
    base::RefCountedString* file_contents) {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  RecordBackgroundTracingMetric(FINALIZATION_STARTED);
  UMA_HISTOGRAM_MEMORY_KB("Tracing.Background.FinalizingTraceSizeInKB",
                          file_contents->size() / 1024);

  if (!receive_callback_.is_null()) {
    std::move(receive_callback_)
        .Run(file_contents, std::move(metadata),
             base::BindOnce(&BackgroundTracingManagerImpl::OnFinalizeComplete,
                            base::Unretained(this)));
  }
  if (!started_finalizing_closure.is_null())
    std::move(started_finalizing_closure).Run();
}

void BackgroundTracingManagerImpl::OnFinalizeComplete(bool success) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&BackgroundTracingManagerImpl::OnFinalizeComplete,
                       base::Unretained(this), success));
    return;
  }

  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  is_gathering_ = false;

  if (!idle_callback_.is_null())
    idle_callback_.Run();

  bool is_allowed_begin =
      !delegate_ || (config_ &&
                     delegate_->IsAllowedToBeginBackgroundScenario(
                         *config_.get(), requires_anonymized_data_));

  // Now that a trace has completed, we may need to enable recording again.
  // TODO(oysteine): Retry later if IsAllowedToBeginBackgroundScenario fails.
  if (is_allowed_begin) {
    StartTracingIfConfigNeedsIt();
  } else {
    AbortScenario();
  }

  if (success) {
    RecordBackgroundTracingMetric(UPLOAD_SUCCEEDED);
  } else {
    RecordBackgroundTracingMetric(UPLOAD_FAILED);
  }
}

bool BackgroundTracingManagerImpl::IsAllowedFinalization() const {
  return !delegate_ ||
         (config_ && delegate_->IsAllowedToEndBackgroundScenario(
                         *config_.get(), requires_anonymized_data_));
}

std::unique_ptr<base::DictionaryValue>
BackgroundTracingManagerImpl::GenerateMetadataDict() {
  if (!IsAllowedFinalization())
    return nullptr;

  auto metadata_dict = std::make_unique<base::DictionaryValue>();
  if (config_) {
    auto config_dict = std::make_unique<base::DictionaryValue>();
    config_->IntoDict(config_dict.get());
    metadata_dict->Set("config", std::move(config_dict));
    metadata_dict->SetString("scenario_name", config_->scenario_name());
  }
  if (last_triggered_rule_)
    metadata_dict->Set("last_triggered_rule", std::move(last_triggered_rule_));
  return metadata_dict;
}

void BackgroundTracingManagerImpl::BeginFinalizing(
    StartedFinalizingCallback callback) {
  is_gathering_ = true;
  is_tracing_ = false;
  triggered_named_event_handle_ = -1;
  tracing_timer_.reset();

  scoped_refptr<TracingControllerImpl::TraceDataEndpoint> trace_data_endpoint;
  bool is_allowed_finalization = IsAllowedFinalization();
  base::Closure started_finalizing_closure;
  if (!callback.is_null()) {
    started_finalizing_closure =
        base::BindRepeating(callback, is_allowed_finalization);
  }
  if (is_allowed_finalization) {
    trace_data_endpoint = TracingControllerImpl::CreateCompressedStringEndpoint(
        TracingControllerImpl::CreateCallbackEndpoint(base::Bind(
            &BackgroundTracingManagerImpl::OnFinalizeStarted,
            base::Unretained(this), std::move(started_finalizing_closure))),
        true /* compress_with_background_priority */);
    RecordBackgroundTracingMetric(FINALIZATION_ALLOWED);
  } else {
    if (!std::move(callback).is_null()) {
      trace_data_endpoint =
          TracingControllerImpl::CreateCallbackEndpoint(base::BindRepeating(
              [](base::Closure closure,
                 std::unique_ptr<const base::DictionaryValue> metadata,
                 base::RefCountedString* file_contents) {
                std::move(closure).Run();
              },
              std::move(started_finalizing_closure)));
    }
    RecordBackgroundTracingMetric(FINALIZATION_DISALLOWED);
  }

  content::TracingControllerImpl::GetInstance()->StopTracing(
      trace_data_endpoint);
}

void BackgroundTracingManagerImpl::AbortScenario() {
  is_tracing_ = false;
  triggered_named_event_handle_ = -1;
  config_.reset();
  tracing_timer_.reset();

  content::TracingControllerImpl::GetInstance()->StopTracing(nullptr);

  for (auto* observer : background_tracing_observers_)
    observer->OnScenarioAborted();
}

std::string
BackgroundTracingManagerImpl::GetCategoryFilterStringForCategoryPreset(
    BackgroundTracingConfigImpl::CategoryPreset preset) const {
  switch (preset) {
    case BackgroundTracingConfigImpl::CategoryPreset::BENCHMARK:
      return "benchmark,toplevel";
    case BackgroundTracingConfigImpl::CategoryPreset::BENCHMARK_DEEP:
      return "*,disabled-by-default-benchmark.detailed,"
             "disabled-by-default-v8.cpu_profile,"
             "disabled-by-default-v8.runtime_stats";
    case BackgroundTracingConfigImpl::CategoryPreset::BENCHMARK_GPU:
      return "benchmark,toplevel,gpu";
    case BackgroundTracingConfigImpl::CategoryPreset::BENCHMARK_IPC:
      return "benchmark,toplevel,ipc";
    case BackgroundTracingConfigImpl::CategoryPreset::BENCHMARK_STARTUP:
      return "benchmark,toplevel,startup,disabled-by-default-file,"
             "disabled-by-default-toplevel.flow,"
             "disabled-by-default-ipc.flow";
    case BackgroundTracingConfigImpl::CategoryPreset::BENCHMARK_BLINK_GC:
      return "blink_gc,disabled-by-default-blink_gc";
    case BackgroundTracingConfigImpl::CategoryPreset::BENCHMARK_MEMORY_HEAVY:
    case BackgroundTracingConfigImpl::CategoryPreset::BENCHMARK_MEMORY_LIGHT:
      return "-*,disabled-by-default-memory-infra";
    case BackgroundTracingConfigImpl::CategoryPreset::
        BENCHMARK_EXECUTION_METRIC:
      return "blink.console,v8";
    case BackgroundTracingConfigImpl::CategoryPreset::BLINK_STYLE:
      return "blink_style";
    case BackgroundTracingConfigImpl::CategoryPreset::CATEGORY_PRESET_UNSET:
      NOTREACHED();
  }
  NOTREACHED();
  return "";
}

}  // namspace content
