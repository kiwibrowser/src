// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/breaking_news/breaking_news_gcm_app_handler.h"

#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/gcm_driver/gcm_profile_service.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/gcm_driver/instance_id/instance_id_driver.h"
#include "components/ntp_snippets/breaking_news/breaking_news_metrics.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/ntp_snippets/time_serialization.h"
#include "components/variations/variations_associated_data.h"

using instance_id::InstanceID;

namespace ntp_snippets {

namespace {

const char kBreakingNewsGCMAppID[] = "com.google.breakingnews.gcm";

// The sender ID is used in the registration process.
// See: https://developers.google.com/cloud-messaging/gcm#senderid
const char kBreakingNewsGCMSenderId[] = "667617379155";

// OAuth2 Scope passed to getToken to obtain GCM registration tokens.
// Must match Java GoogleCloudMessaging.INSTANCE_ID_SCOPE.
const char kGCMScope[] = "GCM";

// The action key in pushed GCM message.
const char kPushedActionKey[] = "action";
// Allowed action key values:
const char kPushToRefreshAction[] = "push-to-refresh";
const char kPushByValueAction[] = "push-by-value";

// Key of the news json in the data in the pushed breaking news.
const char kPushedNewsKey[] = "payload";

// Lower bound time between two token validations when listening.
const int kTokenValidationPeriodMinutesDefault = 60 * 24;
const char kTokenValidationPeriodMinutesParamName[] =
    "token_validation_period_minutes";

base::TimeDelta GetTokenValidationPeriod() {
  return base::TimeDelta::FromMinutes(
      variations::GetVariationParamByFeatureAsInt(
          kBreakingNewsPushFeature, kTokenValidationPeriodMinutesParamName,
          kTokenValidationPeriodMinutesDefault));
}

const bool kEnableTokenValidationDefault = true;
const char kEnableTokenValidationParamName[] = "enable_token_validation";

bool IsTokenValidationEnabled() {
  return variations::GetVariationParamByFeatureAsBool(
      kBreakingNewsPushFeature, kEnableTokenValidationParamName,
      kEnableTokenValidationDefault);
}

// Lower bound time between two forced subscriptions when listening. A
// forced subscription is a normal subscription to the content
// suggestions server, which cannot be omitted.
const int kForcedSubscriptionPeriodMinutesDefault = 60 * 24 * 7;
const char kForcedSubscriptionPeriodMinutesParamName[] =
    "forced_subscription_period_minutes";

base::TimeDelta GetForcedSubscriptionPeriod() {
  return base::TimeDelta::FromMinutes(
      variations::GetVariationParamByFeatureAsInt(
          kBreakingNewsPushFeature, kForcedSubscriptionPeriodMinutesParamName,
          kForcedSubscriptionPeriodMinutesDefault));
}

const bool kEnableForcedSubscriptionDefault = true;
const char kEnableForcedSubscriptionParamName[] = "enable_forced_subscription";

bool IsForcedSubscriptionEnabled() {
  return variations::GetVariationParamByFeatureAsBool(
      kBreakingNewsPushFeature, kEnableForcedSubscriptionParamName,
      kEnableForcedSubscriptionDefault);
}

}  // namespace

BreakingNewsGCMAppHandler::BreakingNewsGCMAppHandler(
    gcm::GCMDriver* gcm_driver,
    instance_id::InstanceIDDriver* instance_id_driver,
    PrefService* pref_service,
    std::unique_ptr<SubscriptionManager> subscription_manager,
    const ParseJSONCallback& parse_json_callback,
    base::Clock* clock,
    std::unique_ptr<base::OneShotTimer> token_validation_timer,
    std::unique_ptr<base::OneShotTimer> forced_subscription_timer)
    : gcm_driver_(gcm_driver),
      instance_id_driver_(instance_id_driver),
      pref_service_(pref_service),
      subscription_manager_(std::move(subscription_manager)),
      parse_json_callback_(parse_json_callback),
      clock_(clock),
      token_validation_timer_(std::move(token_validation_timer)),
      forced_subscription_timer_(std::move(forced_subscription_timer)),
      weak_ptr_factory_(this) {
#if !defined(OS_ANDROID)
#error The BreakingNewsGCMAppHandler should only be used on Android.
#endif  // !OS_ANDROID
  DCHECK(token_validation_timer_);
  DCHECK(!token_validation_timer_->IsRunning());
  DCHECK(forced_subscription_timer_);
  DCHECK(!forced_subscription_timer_->IsRunning());
}

BreakingNewsGCMAppHandler::~BreakingNewsGCMAppHandler() {
  if (IsListening()) {
    StopListening();
  }
}

void BreakingNewsGCMAppHandler::StartListening(
    OnNewRemoteSuggestionCallback on_new_remote_suggestion_callback,
    OnRefreshRequestedCallback on_refresh_requested_callback) {
  DCHECK(!IsListening());
  DCHECK(!on_new_remote_suggestion_callback.is_null());
  on_new_remote_suggestion_callback_ =
      std::move(on_new_remote_suggestion_callback);

  DCHECK(!on_refresh_requested_callback.is_null());
  on_refresh_requested_callback_ = std::move(on_refresh_requested_callback);

  Subscribe(/*force_token_retrieval=*/false);
  gcm_driver_->AddAppHandler(kBreakingNewsGCMAppID, this);
  if (IsTokenValidationEnabled()) {
    ScheduleNextTokenValidation();
  }
  if (IsForcedSubscriptionEnabled()) {
    ScheduleNextForcedSubscription();
  }
}

void BreakingNewsGCMAppHandler::StopListening() {
  DCHECK(IsListening());
  token_validation_timer_->Stop();
  forced_subscription_timer_->Stop();
  DCHECK_EQ(gcm_driver_->GetAppHandler(kBreakingNewsGCMAppID), this);
  gcm_driver_->RemoveAppHandler(kBreakingNewsGCMAppID);
  on_new_remote_suggestion_callback_ = OnNewRemoteSuggestionCallback();
  subscription_manager_->Unsubscribe();
}

bool BreakingNewsGCMAppHandler::IsListening() const {
  return !on_new_remote_suggestion_callback_.is_null();
}

void BreakingNewsGCMAppHandler::Subscribe(bool force_token_retrieval) {
  // TODO(mamir): "Whether to subscribe to content suggestions server" logic
  // should be moved to the SubscriptionManager.
  std::string token =
      pref_service_->GetString(prefs::kBreakingNewsGCMSubscriptionTokenCache);
  // If a token has been already obtained, subscribe directly at the content
  // suggestions server. Otherwise, obtain a GCM token first.
  if (!token.empty() && !force_token_retrieval) {
    if (!subscription_manager_->IsSubscribed() ||
        subscription_manager_->NeedsToResubscribe()) {
      subscription_manager_->Subscribe(token);
    }
    return;
  }

  // TODO(vitaliii): Use |BindOnce| instead of |Bind|, because the callback is
  // meant to be run only once.
  instance_id_driver_->GetInstanceID(kBreakingNewsGCMAppID)
      ->GetToken(kBreakingNewsGCMSenderId, kGCMScope,
                 /*options=*/std::map<std::string, std::string>(),
                 base::Bind(&BreakingNewsGCMAppHandler::DidRetrieveToken,
                            weak_ptr_factory_.GetWeakPtr()));
}

void BreakingNewsGCMAppHandler::DidRetrieveToken(
    const std::string& subscription_token,
    InstanceID::Result result) {
  if (!IsListening()) {
    // After we requested the token, |StopListening| has been called. Thus,
    // ignore the token.
    return;
  }

  metrics::OnTokenRetrieved(result);

  switch (result) {
    case InstanceID::SUCCESS:
      // The received token is assumed to be valid, therefore, we reschedule
      // validation.
      pref_service_->SetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime,
                              SerializeTime(clock_->Now()));
      if (IsTokenValidationEnabled()) {
        ScheduleNextTokenValidation();
      }
      pref_service_->SetString(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                               subscription_token);
      subscription_manager_->Subscribe(subscription_token);
      return;
    case InstanceID::INVALID_PARAMETER:
    case InstanceID::DISABLED:
    case InstanceID::ASYNC_OPERATION_PENDING:
    case InstanceID::SERVER_ERROR:
    case InstanceID::UNKNOWN_ERROR:
      DLOG(WARNING)
          << "Push messaging subscription failed; InstanceID::Result = "
          << result;
      break;
    case InstanceID::NETWORK_ERROR:
      break;
  }
}

void BreakingNewsGCMAppHandler::ResubscribeIfInvalidToken() {
  DCHECK(IsListening());
  DCHECK(IsTokenValidationEnabled());

  // InstanceIDAndroid::ValidateToken just returns |true| on Android. Instead it
  // is ok to retrieve a token, because it is cached.
  // TODO(vitaliii): Use |BindOnce| instead of |Bind|, because the callback is
  // meant to be run only once.
  instance_id_driver_->GetInstanceID(kBreakingNewsGCMAppID)
      ->GetToken(
          kBreakingNewsGCMSenderId, kGCMScope,
          /*options=*/std::map<std::string, std::string>(),
          base::Bind(&BreakingNewsGCMAppHandler::DidReceiveTokenForValidation,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BreakingNewsGCMAppHandler::DidReceiveTokenForValidation(
    const std::string& new_token,
    InstanceID::Result result) {
  if (!IsListening()) {
    // After we requested the token, |StopListening| has been called. Thus,
    // ignore the token.
    return;
  }

  metrics::OnTokenRetrieved(result);

  base::Optional<base::TimeDelta> time_since_last_validation;
  if (pref_service_->HasPrefPath(
          prefs::kBreakingNewsGCMLastTokenValidationTime)) {
    const base::Time last_validation_time =
        DeserializeTime(pref_service_->GetInt64(
            prefs::kBreakingNewsGCMLastTokenValidationTime));
    time_since_last_validation = clock_->Now() - last_validation_time;
  }

  // We intentionally reschedule as normal even if we don't get a token.
  pref_service_->SetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime,
                          SerializeTime(clock_->Now()));
  ScheduleNextTokenValidation();

  base::Optional<bool> was_token_valid;
  if (result == InstanceID::SUCCESS) {
    const std::string old_token =
        pref_service_->GetString(prefs::kBreakingNewsGCMSubscriptionTokenCache);

    was_token_valid = old_token == new_token;
    if (!*was_token_valid) {
      subscription_manager_->Resubscribe(new_token);
    }
  }

  metrics::OnTokenValidationAttempted(time_since_last_validation,
                                      was_token_valid);
}

void BreakingNewsGCMAppHandler::ScheduleNextTokenValidation() {
  DCHECK(IsListening());
  DCHECK(IsTokenValidationEnabled());

  const base::Time last_validation_time = DeserializeTime(
      pref_service_->GetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime));
  // Timer runs the task immediately if delay is <= 0.
  token_validation_timer_->Start(
      FROM_HERE,
      /*delay=*/last_validation_time + GetTokenValidationPeriod() -
          clock_->Now(),
      base::Bind(&BreakingNewsGCMAppHandler::ResubscribeIfInvalidToken,
                 weak_ptr_factory_.GetWeakPtr()));
}

void BreakingNewsGCMAppHandler::ForceSubscribe() {
  DCHECK(IsForcedSubscriptionEnabled());

  // We intentionally reschedule as normal even if there is no token or
  // subscription fails.
  pref_service_->SetInt64(prefs::kBreakingNewsGCMLastForcedSubscriptionTime,
                          SerializeTime(clock_->Now()));
  ScheduleNextForcedSubscription();

  const std::string token =
      pref_service_->GetString(prefs::kBreakingNewsGCMSubscriptionTokenCache);
  if (!token.empty()) {
    subscription_manager_->Subscribe(token);
  }
}

void BreakingNewsGCMAppHandler::ScheduleNextForcedSubscription() {
  DCHECK(IsListening());
  DCHECK(IsForcedSubscriptionEnabled());

  const base::Time last_forced_subscription_time =
      DeserializeTime(pref_service_->GetInt64(
          prefs::kBreakingNewsGCMLastForcedSubscriptionTime));
  // Timer runs the task immediately if delay is <= 0.
  forced_subscription_timer_->Start(
      FROM_HERE,
      /*delay=*/last_forced_subscription_time + GetForcedSubscriptionPeriod() -
          clock_->Now(),
      base::Bind(&BreakingNewsGCMAppHandler::ForceSubscribe,
                 weak_ptr_factory_.GetWeakPtr()));
}

void BreakingNewsGCMAppHandler::ShutdownHandler() {}

void BreakingNewsGCMAppHandler::OnStoreReset() {
  pref_service_->ClearPref(prefs::kBreakingNewsGCMSubscriptionTokenCache);
}

void BreakingNewsGCMAppHandler::OnMessage(const std::string& app_id,
                                          const gcm::IncomingMessage& message) {
  DCHECK_EQ(app_id, kBreakingNewsGCMAppID);

  if (!IsListening()) {
    // The content suggestions server may push a message right when the client
    // unsubscribes leading to a race condition. Ignore such messages.
    DLOG(WARNING) << "Received a pushed message while not listening.";
    return;
  }

  gcm::MessageData::const_iterator it = message.data.find(kPushedActionKey);

  bool contains_pushed_action = (it != message.data.end());
  if (!contains_pushed_action) {
    LOG(WARNING) << "Receiving pushed content failure: Action is missing.";
    metrics::OnMessageReceived(metrics::ReceivedMessageAction::NO_ACTION);
    return;
  }
  const std::string& action = it->second;

  if (action == kPushToRefreshAction) {
    metrics::OnMessageReceived(metrics::ReceivedMessageAction::PUSH_TO_REFRESH);
    OnPushToRefreshMessage();
    return;
  }

  if (action == kPushByValueAction) {
    metrics::OnMessageReceived(metrics::ReceivedMessageAction::PUSH_BY_VALUE);
    OnPushByValueMessage(message);
    return;
  }

  LOG(WARNING) << "Receiving pushed content failure: Invalid action.";
  metrics::OnMessageReceived(metrics::ReceivedMessageAction::INVALID_ACTION);
}

void BreakingNewsGCMAppHandler::OnPushByValueMessage(
    const gcm::IncomingMessage& message) {
  gcm::MessageData::const_iterator it = message.data.find(kPushedNewsKey);
  bool contains_pushed_news = (it != message.data.end());
  if (!contains_pushed_news) {
    LOG(WARNING)
        << "Receiving pushed content failure: Breaking News ID missing.";
  }

  const std::string& news = it->second;
  parse_json_callback_.Run(news,
                           base::Bind(&BreakingNewsGCMAppHandler::OnJsonSuccess,
                                      weak_ptr_factory_.GetWeakPtr()),
                           base::Bind(&BreakingNewsGCMAppHandler::OnJsonError,
                                      weak_ptr_factory_.GetWeakPtr(), news));
}

void BreakingNewsGCMAppHandler::OnPushToRefreshMessage() {
  on_refresh_requested_callback_.Run();
}

void BreakingNewsGCMAppHandler::OnMessagesDeleted(const std::string& app_id) {
  // Messages don't get deleted.
  NOTREACHED() << "BreakingNewsGCMAppHandler messages don't get deleted.";
}

void BreakingNewsGCMAppHandler::OnSendError(
    const std::string& app_id,
    const gcm::GCMClient::SendErrorDetails& details) {
  // Should never be called because we don't send GCM messages to
  // the server.
  NOTREACHED() << "BreakingNewsGCMAppHandler doesn't send GCM messages.";
}

void BreakingNewsGCMAppHandler::OnSendAcknowledged(
    const std::string& app_id,
    const std::string& message_id) {
  // Should never be called because we don't send GCM messages to
  // the server.
  NOTREACHED() << "BreakingNewsGCMAppHandler doesn't send GCM messages.";
}

// static
void BreakingNewsGCMAppHandler::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                               /*default_value=*/std::string());
  registry->RegisterInt64Pref(prefs::kBreakingNewsGCMLastTokenValidationTime,
                              /*default_value=*/0);
  registry->RegisterInt64Pref(prefs::kBreakingNewsGCMLastForcedSubscriptionTime,
                              /*default_value=*/0);
}

// TODO(vitaliii): Add a test to ensure that this clears everything.
// static
void BreakingNewsGCMAppHandler::ClearProfilePrefs(PrefService* pref_service) {
  pref_service->ClearPref(prefs::kBreakingNewsGCMSubscriptionTokenCache);
  pref_service->ClearPref(prefs::kBreakingNewsGCMLastTokenValidationTime);
  pref_service->ClearPref(prefs::kBreakingNewsGCMSubscriptionTokenCache);
}

void BreakingNewsGCMAppHandler::OnJsonSuccess(
    std::unique_ptr<base::Value> content) {
  DCHECK(content);

  if (!IsListening()) {
    // |StopListening| might be called after JSON parse request is submitted,
    // but the request cannot be canceled, so we just ignore the parsed JSON.
    return;
  }

  std::vector<FetchedCategory> fetched_categories;
  if (!JsonToCategories(*content, &fetched_categories,
                        /*fetch_time=*/base::Time::Now())) {
    std::string content_json;
    base::JSONWriter::Write(*content, &content_json);
    LOG(WARNING)
        << "Received invalid breaking news: can't interpret value, json is "
        << content_json;
    return;
  }
  if (fetched_categories.size() != 1) {
    LOG(WARNING)
        << "Received invalid breaking news: expected 1 category, but got "
        << fetched_categories.size();
    return;
  }
  if (fetched_categories[0].suggestions.size() != 1) {
    LOG(WARNING)
        << "Received invalid breaking news: expected 1 suggestion, but got "
        << fetched_categories[0].suggestions.size();
    return;
  }

  on_new_remote_suggestion_callback_.Run(
      std::move(fetched_categories[0].suggestions[0]));
}

void BreakingNewsGCMAppHandler::OnJsonError(const std::string& json_str,
                                            const std::string& error) {
  LOG(WARNING) << "Error parsing JSON:" << error
               << " when parsing:" << json_str;
}

}  // namespace ntp_snippets
