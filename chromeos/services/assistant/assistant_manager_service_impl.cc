// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/assistant/assistant_manager_service_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "build/util/webkit_version.h"
#include "chromeos/assistant/internal/internal_constants.h"
#include "chromeos/assistant/internal/internal_util.h"
#include "chromeos/services/assistant/service.h"
#include "chromeos/services/assistant/utils.h"
#include "chromeos/system/version_loader.h"
#include "libassistant/shared/internal_api/assistant_manager_delegate.h"
#include "libassistant/shared/internal_api/assistant_manager_internal.h"
#include "url/gurl.h"

using assistant_client::ActionModule;

namespace chromeos {
namespace assistant {

const char kWiFiDeviceSettingId[] = "WIFI";
const char kBluetoothDeviceSettingId[] = "BLUETOOTH";

AssistantManagerServiceImpl::AssistantManagerServiceImpl(
    mojom::AudioInputPtr audio_input,
    device::mojom::BatteryMonitorPtr battery_monitor)
    : platform_api_(CreateLibAssistantConfig(),
                    std::move(audio_input),
                    std::move(battery_monitor)),
      action_module_(std::make_unique<action::CrosActionModule>(this)),
      display_connection_(std::make_unique<CrosDisplayConnection>(this)),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {}

AssistantManagerServiceImpl::~AssistantManagerServiceImpl() {}

void AssistantManagerServiceImpl::Start(const std::string& access_token,
                                        base::OnceClosure callback) {
  // LibAssistant creation will make file IO. Post the creation to
  // background thread to avoid DCHECK.
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&assistant_client::AssistantManager::Create,
                     &platform_api_, CreateLibAssistantConfig()),
      base::BindOnce(&AssistantManagerServiceImpl::StartAssistantInternal,
                     base::Unretained(this), std::move(callback), access_token,
                     chromeos::version_loader::GetARCVersion()));

  // Set the flag to avoid starting the service multiple times.
  state_ = State::STARTED;
}

AssistantManagerService::State AssistantManagerServiceImpl::GetState() const {
  return state_;
}

void AssistantManagerServiceImpl::SetAccessToken(
    const std::string& access_token) {
  // Push the |access_token| we got as an argument into AssistantManager before
  // starting to ensure that all server requests will be authenticated once
  // it is started. |user_id| is used to pair a user to their |access_token|,
  // since we do not support multi-user in this example we can set it to a
  // dummy value like "0".
  assistant_manager_->SetAuthTokens({std::pair<std::string, std::string>(
      /* user_id: */ "0", access_token)});
}

void AssistantManagerServiceImpl::EnableListening(bool enable) {
  assistant_manager_->EnableListening(enable);
}

AssistantSettingsManager*
AssistantManagerServiceImpl::GetAssistantSettingsManager() {
  return assistant_settings_manager_.get();
}

void AssistantManagerServiceImpl::SendGetSettingsUiRequest(
    const std::string& selector,
    GetSettingsUiResponseCallback callback) {
  std::string serialized_proto = SerializeGetSettingsUiRequest(selector);
  assistant_manager_internal_->SendGetSettingsUiRequest(
      serialized_proto, std::string(), [
        repeating_callback =
            base::AdaptCallbackForRepeating(std::move(callback)),
        weak_ptr = weak_factory_.GetWeakPtr(),
        task_runner = main_thread_task_runner_
      ](const assistant_client::VoicelessResponse& response) {
        // This callback may be called from server multiple times. We should
        // only process non-empty response.
        std::string settings = UnwrapGetSettingsUiResponse(response);
        if (!settings.empty()) {
          task_runner->PostTask(
              FROM_HERE,
              base::BindOnce(
                  &AssistantManagerServiceImpl::HandleGetSettingsResponse,
                  std::move(weak_ptr), repeating_callback, settings));
        }
      });
}

void AssistantManagerServiceImpl::SendUpdateSettingsUiRequest(
    const std::string& update,
    UpdateSettingsUiResponseCallback callback) {
  std::string serialized_proto = SerializeUpdateSettingsUiRequest(update);
  assistant_manager_internal_->SendUpdateSettingsUiRequest(
      serialized_proto, std::string(), [
        repeating_callback =
            base::AdaptCallbackForRepeating(std::move(callback)),
        weak_ptr = weak_factory_.GetWeakPtr(),
        task_runner = main_thread_task_runner_
      ](const assistant_client::VoicelessResponse& response) {
        // This callback may be called from server multiple times. We should
        // only process non-empty response.
        std::string update = UnwrapUpdateSettingsUiResponse(response);
        if (!update.empty()) {
          task_runner->PostTask(
              FROM_HERE,
              base::BindOnce(
                  &AssistantManagerServiceImpl::HandleUpdateSettingsResponse,
                  std::move(weak_ptr), repeating_callback, update));
        }
      });
}

void AssistantManagerServiceImpl::StartVoiceInteraction() {
  assistant_manager_->StartAssistantInteraction();
}

void AssistantManagerServiceImpl::StopActiveInteraction() {
  assistant_manager_->StopAssistantInteraction();
}

void AssistantManagerServiceImpl::SendTextQuery(const std::string& query) {
  assistant_manager_internal_->SendTextQuery(query);
}

void AssistantManagerServiceImpl::AddAssistantEventSubscriber(
    mojom::AssistantEventSubscriberPtr subscriber) {
  subscribers_.AddPtr(std::move(subscriber));
}

void AssistantManagerServiceImpl::OnConversationTurnStarted() {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AssistantManagerServiceImpl::OnConversationTurnStartedOnMainThread,
          weak_factory_.GetWeakPtr()));
}

void AssistantManagerServiceImpl::OnConversationTurnFinished(
    assistant_client::ConversationStateListener::Resolution resolution) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AssistantManagerServiceImpl::OnConversationTurnFinishedOnMainThread,
          weak_factory_.GetWeakPtr(), resolution));
}

void AssistantManagerServiceImpl::OnShowHtml(const std::string& html) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AssistantManagerServiceImpl::OnShowHtmlOnMainThread,
                     weak_factory_.GetWeakPtr(), html));
}

void AssistantManagerServiceImpl::OnShowSuggestions(
    const std::vector<action::Suggestion>& suggestions) {
  // Convert to mojom struct for IPC.
  std::vector<mojom::AssistantSuggestionPtr> ptrs;
  for (const action::Suggestion& suggestion : suggestions) {
    mojom::AssistantSuggestionPtr ptr = mojom::AssistantSuggestion::New();
    ptr->text = suggestion.text;
    ptr->icon_url = GURL(suggestion.icon_url);
    ptr->action_url = GURL(suggestion.action_url);
    ptrs.push_back(std::move(ptr));
  }

  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AssistantManagerServiceImpl::OnShowSuggestionsOnMainThread,
          weak_factory_.GetWeakPtr(), std::move(ptrs)));
}

void AssistantManagerServiceImpl::OnShowText(const std::string& text) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AssistantManagerServiceImpl::OnShowTextOnMainThread,
                     weak_factory_.GetWeakPtr(), text));
}

void AssistantManagerServiceImpl::OnOpenUrl(const std::string& url) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AssistantManagerServiceImpl::OnOpenUrlOnMainThread,
                     weak_factory_.GetWeakPtr(), url));
}

void AssistantManagerServiceImpl::OnRecognitionStateChanged(
    assistant_client::ConversationStateListener::RecognitionState state,
    const assistant_client::ConversationStateListener::RecognitionResult&
        recognition_result) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AssistantManagerServiceImpl::OnRecognitionStateChangedOnMainThread,
          weak_factory_.GetWeakPtr(), state, recognition_result));
}

void AssistantManagerServiceImpl::OnSpeechLevelUpdated(
    const float speech_level) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &AssistantManagerServiceImpl::OnSpeechLevelUpdatedOnMainThread,
          weak_factory_.GetWeakPtr(), speech_level));
}

ActionModule::Result AssistantManagerServiceImpl::HandleModifySettingClientOp(
    const std::string& modify_setting_args_proto) {
  DVLOG(2) << "HandleModifySettingClientOp=" << modify_setting_args_proto;
  // TODO(b/78184487): Wire up to Chrome API to update device setting.
  return ActionModule::Result::Ok();
}

bool AssistantManagerServiceImpl::IsSettingSupported(
    const std::string& setting_id) {
  DVLOG(2) << "IsSettingSupported=" << setting_id;
  return (setting_id == kWiFiDeviceSettingId ||
          setting_id == kBluetoothDeviceSettingId);
}

void AssistantManagerServiceImpl::StartAssistantInternal(
    base::OnceCallback<void()> callback,
    const std::string& access_token,
    const std::string& arc_version,
    assistant_client::AssistantManager* assistant_manager) {
  assistant_manager_.reset(assistant_manager);
  assistant_manager_internal_ =
      UnwrapAssistantManagerInternal(assistant_manager_.get());

  auto* internal_options =
      assistant_manager_internal_->CreateDefaultInternalOptions();
  SetAssistantOptions(internal_options, BuildUserAgent(arc_version));
  assistant_manager_internal_->SetOptions(*internal_options, [](bool success) {
    DVLOG(2) << "set options: " << success;
  });

  assistant_manager_internal_->SetDisplayConnection(display_connection_.get());
  assistant_manager_internal_->RegisterActionModule(action_module_.get());
  assistant_manager_internal_->SetAssistantManagerDelegate(this);
  assistant_manager_->AddConversationStateListener(this);

  if (!assistant_settings_manager_) {
    assistant_settings_manager_ =
        std::make_unique<AssistantSettingsManagerImpl>(this);
  }

  SetAccessToken(access_token);

  assistant_client::Callback0 assistant_callback([callback = std::move(
                                                      callback)]() mutable {
    std::move(callback).Run();
  });

  assistant_manager_->Start(std::move(assistant_callback));

  state_ = State::RUNNING;
}

std::string AssistantManagerServiceImpl::BuildUserAgent(
    const std::string& arc_version) const {
  int32_t os_major_version = 0;
  int32_t os_minor_version = 0;
  int32_t os_bugfix_version = 0;
  base::SysInfo::OperatingSystemVersionNumbers(
      &os_major_version, &os_minor_version, &os_bugfix_version);

  std::string user_agent;
  base::StringAppendF(&user_agent,
                      "Mozilla/5.0 (X11; CrOS %s %d.%d.%d; %s) "
                      "AppleWebKit/%d.%d (KHTML, like Gecko)",
                      base::SysInfo::OperatingSystemArchitecture().c_str(),
                      os_major_version, os_minor_version, os_bugfix_version,
                      base::SysInfo::GetLsbReleaseBoard().c_str(),
                      WEBKIT_VERSION_MAJOR, WEBKIT_VERSION_MINOR);

  if (!arc_version.empty()) {
    base::StringAppendF(&user_agent, " ARC/%s", arc_version.c_str());
  }
  return user_agent;
}

void AssistantManagerServiceImpl::HandleGetSettingsResponse(
    base::RepeatingCallback<void(const std::string&)> callback,
    const std::string& settings) {
  callback.Run(settings);
}

void AssistantManagerServiceImpl::HandleUpdateSettingsResponse(
    base::RepeatingCallback<void(const std::string&)> callback,
    const std::string& result) {
  callback.Run(result);
}

void AssistantManagerServiceImpl::OnConversationTurnStartedOnMainThread() {
  subscribers_.ForAllPtrs([](auto* ptr) { ptr->OnInteractionStarted(); });
}

void AssistantManagerServiceImpl::OnConversationTurnFinishedOnMainThread(
    assistant_client::ConversationStateListener::Resolution resolution) {
  switch (resolution) {
    // Interaction ended normally.
    // Note that TIMEOUT here does not refer to server timeout, but rather mic
    // timeout due to speech inactivity. As this case does not require special
    // UI logic, it is treated here as a normal interaction completion.
    case assistant_client::ConversationStateListener::Resolution::NORMAL:
    case assistant_client::ConversationStateListener::Resolution::
        NORMAL_WITH_FOLLOW_ON:
    case assistant_client::ConversationStateListener::Resolution::TIMEOUT:
      subscribers_.ForAllPtrs([](auto* ptr) {
        ptr->OnInteractionFinished(
            mojom::AssistantInteractionResolution::kNormal);
      });
      break;
    // Interaction ended due to interruption.
    case assistant_client::ConversationStateListener::Resolution::BARGE_IN:
    case assistant_client::ConversationStateListener::Resolution::CANCELLED:
      subscribers_.ForAllPtrs([](auto* ptr) {
        ptr->OnInteractionFinished(
            mojom::AssistantInteractionResolution::kInterruption);
      });
      break;
    // Interaction ended due to multi-device hotword loss.
    case assistant_client::ConversationStateListener::Resolution::NO_RESPONSE:
      subscribers_.ForAllPtrs([](auto* ptr) {
        ptr->OnInteractionFinished(
            mojom::AssistantInteractionResolution::kMultiDeviceHotwordLoss);
      });
      break;
    // Interaction ended due to error.
    case assistant_client::ConversationStateListener::Resolution::
        COMMUNICATION_ERROR:
      subscribers_.ForAllPtrs([](auto* ptr) {
        ptr->OnInteractionFinished(
            mojom::AssistantInteractionResolution::kError);
      });
      break;
  }
}

void AssistantManagerServiceImpl::OnShowHtmlOnMainThread(
    const std::string& html) {
  subscribers_.ForAllPtrs([&html](auto* ptr) { ptr->OnHtmlResponse(html); });
}

void AssistantManagerServiceImpl::OnShowSuggestionsOnMainThread(
    const std::vector<mojom::AssistantSuggestionPtr>& suggestions) {
  subscribers_.ForAllPtrs([&suggestions](auto* ptr) {
    ptr->OnSuggestionsResponse(mojo::Clone(suggestions));
  });
}

void AssistantManagerServiceImpl::OnShowTextOnMainThread(
    const std::string& text) {
  subscribers_.ForAllPtrs([&text](auto* ptr) { ptr->OnTextResponse(text); });
}

void AssistantManagerServiceImpl::OnOpenUrlOnMainThread(
    const std::string& url) {
  subscribers_.ForAllPtrs(
      [&url](auto* ptr) { ptr->OnOpenUrlResponse(GURL(url)); });
}

void AssistantManagerServiceImpl::OnRecognitionStateChangedOnMainThread(
    assistant_client::ConversationStateListener::RecognitionState state,
    const assistant_client::ConversationStateListener::RecognitionResult&
        recognition_result) {
  switch (state) {
    case assistant_client::ConversationStateListener::RecognitionState::STARTED:
      subscribers_.ForAllPtrs(
          [](auto* ptr) { ptr->OnSpeechRecognitionStarted(); });
      break;
    case assistant_client::ConversationStateListener::RecognitionState::
        INTERMEDIATE_RESULT:
      subscribers_.ForAllPtrs([&recognition_result](auto* ptr) {
        ptr->OnSpeechRecognitionIntermediateResult(
            recognition_result.high_confidence_text,
            recognition_result.low_confidence_text);
      });
      break;
    case assistant_client::ConversationStateListener::RecognitionState::
        END_OF_UTTERANCE:
      subscribers_.ForAllPtrs(
          [](auto* ptr) { ptr->OnSpeechRecognitionEndOfUtterance(); });
      break;
    case assistant_client::ConversationStateListener::RecognitionState::
        FINAL_RESULT:
      subscribers_.ForAllPtrs([&recognition_result](auto* ptr) {
        ptr->OnSpeechRecognitionFinalResult(
            recognition_result.recognized_speech);
      });
      break;
  }
}

void AssistantManagerServiceImpl::OnSpeechLevelUpdatedOnMainThread(
    const float speech_level) {
  subscribers_.ForAllPtrs(
      [&speech_level](auto* ptr) { ptr->OnSpeechLevelUpdated(speech_level); });
}

}  // namespace assistant
}  // namespace chromeos
