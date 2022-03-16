// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/browser/translate_ranker_impl.h"

#include <cmath>
#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/metrics_hashes.h"
#include "base/strings/string_util.h"
#include "base/task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "components/assist_ranker/proto/ranker_model.pb.h"
#include "components/assist_ranker/proto/translate_ranker_model.pb.h"
#include "components/assist_ranker/ranker_model.h"
#include "components/assist_ranker/ranker_model_loader_impl.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/translate/core/common/translate_switches.h"
#include "components/variations/variations_associated_data.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "third_party/metrics_proto/translate_event.pb.h"
#include "url/gurl.h"

namespace translate {

namespace {

using assist_ranker::RankerModel;
using assist_ranker::RankerModelProto;
using assist_ranker::RankerModelStatus;
using assist_ranker::TranslateRankerModel;

const double kTranslationOfferDefaultThreshold = 0.5;

const char kTranslateRankerModelFileName[] = "Translate Ranker Model";
const char kUmaPrefix[] = "Translate.Ranker";
const char kUnknown[] = "UNKNOWN";

double Sigmoid(double x) {
  return 1.0 / (1.0 + exp(-x));
}

double SafeRatio(int numerator, int denominator) {
  return denominator ? (numerator / static_cast<double>(denominator)) : 0.0;
}

double ScoreComponent(const google::protobuf::Map<std::string, float>& weights,
                      const std::string& key) {
  auto i = weights.find(base::ToLowerASCII(key));
  if (i == weights.end())
    i = weights.find(kUnknown);
  return i == weights.end() ? 0.0 : i->second;
}

RankerModelStatus ValidateModel(const RankerModel& model) {
  if (model.proto().model_case() != RankerModelProto::kTranslate)
    return RankerModelStatus::VALIDATION_FAILED;

  if (model.proto().translate().model_revision_case() !=
      TranslateRankerModel::kTranslateLogisticRegressionModel) {
    return RankerModelStatus::INCOMPATIBLE;
  }

  return RankerModelStatus::OK;
}

}  // namespace

#if defined(OS_ANDROID)
const char kDefaultTranslateRankerModelURL[] =
    "https://gstatic.browser.mises.site/chrome/intelligence/assist/ranker/models/"
    "translate/android/translate_ranker_model_android_20170918.pb.bin";
#elif defined(USE_AURA)
const char kDefaultTranslateRankerModelURL[] =
    "https://gstatic.browser.mises.site/chrome/intelligence/assist/ranker/models/"
    "translate/translate_ranker_20180123.model";
#else
const char kDefaultTranslateRankerModelURL[] =
    "https://gstatic.browser.mises.site/chrome/intelligence/assist/ranker/models/"
    "translate/2017/03/translate_ranker_model_20170329.pb.bin";
#endif

const base::Feature kTranslateRankerQuery{"TranslateRankerQuery",
                                          base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTranslateRankerEnforcement{
    "TranslateRankerEnforcement", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kTranslateRankerAutoBlacklistOverride{
    "TranslateRankerAutoBlacklistOverride", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kTranslateRankerPreviousLanguageMatchesOverride{
    "TranslateRankerPreviousLanguageMatchesOverride",
    base::FEATURE_DISABLED_BY_DEFAULT};

TranslateRankerFeatures::TranslateRankerFeatures() {}

TranslateRankerFeatures::TranslateRankerFeatures(int accepted,
                                                 int denied,
                                                 int ignored,
                                                 const std::string& src,
                                                 const std::string& dst,
                                                 const std::string& cntry,
                                                 const std::string& locale)
    : accepted_count(accepted),
      denied_count(denied),
      ignored_count(ignored),
      total_count(accepted_count + denied_count + ignored_count),
      src_lang(src),
      dst_lang(dst),
      country(cntry),
      app_locale(locale),
      accepted_ratio(SafeRatio(accepted_count, total_count)),
      denied_ratio(SafeRatio(denied_count, total_count)),
      ignored_ratio(SafeRatio(ignored_count, total_count)) {}

// TODO(hamelphi): Log locale in TranslateEventProtos.
TranslateRankerFeatures::TranslateRankerFeatures(
    const metrics::TranslateEventProto& translate_event)
    : TranslateRankerFeatures(translate_event.accept_count(),
                              translate_event.decline_count(),
                              translate_event.ignore_count(),
                              translate_event.source_language(),
                              translate_event.target_language(),
                              translate_event.country(),
                              "" /*locale*/) {}

TranslateRankerFeatures::~TranslateRankerFeatures() {}

void TranslateRankerFeatures::WriteTo(std::ostream& stream) const {
  stream << "src_lang='" << src_lang << "', "
         << "dst_lang='" << dst_lang << "', "
         << "country='" << country << "', "
         << "app_locale='" << app_locale << "', "
         << "accept_count=" << accepted_count << ", "
         << "denied_count=" << denied_count << ", "
         << "ignored_count=" << ignored_count << ", "
         << "total_count=" << total_count << ", "
         << "accept_ratio=" << accepted_ratio << ", "
         << "decline_ratio=" << denied_ratio << ", "
         << "ignore_ratio=" << ignored_ratio;
}

TranslateRankerImpl::TranslateRankerImpl(const base::FilePath& model_path,
                                         const GURL& model_url,
                                         ukm::UkmRecorder* ukm_recorder)
    : ukm_recorder_(ukm_recorder),
      is_logging_enabled_(false),
      is_query_enabled_(base::FeatureList::IsEnabled(kTranslateRankerQuery)),
      is_enforcement_enabled_(
          base::FeatureList::IsEnabled(kTranslateRankerEnforcement)),
      is_auto_blacklist_override_enabled_(base::FeatureList::IsEnabled(
          translate::kTranslateRankerAutoBlacklistOverride)),
      is_previous_language_matches_override_enabled_(
          base::FeatureList::IsEnabled(
              translate::kTranslateRankerPreviousLanguageMatchesOverride)),
      weak_ptr_factory_(this) {
  if (is_query_enabled_ || is_enforcement_enabled_) {
    model_loader_ = std::make_unique<assist_ranker::RankerModelLoaderImpl>(
        base::Bind(&ValidateModel),
        base::Bind(&TranslateRankerImpl::OnModelAvailable,
                   weak_ptr_factory_.GetWeakPtr()),
        TranslateDownloadManager::GetInstance()->request_context(), model_path,
        model_url, kUmaPrefix);
    // Kick off the initial load from cache.
    model_loader_->NotifyOfRankerActivity();
  }
}

TranslateRankerImpl::~TranslateRankerImpl() {}

// static
base::FilePath TranslateRankerImpl::GetModelPath(
    const base::FilePath& data_dir) {
  if (data_dir.empty())
    return base::FilePath();

  // Otherwise, look for the file in data dir.
  return data_dir.AppendASCII(kTranslateRankerModelFileName);
}

// static
GURL TranslateRankerImpl::GetModelURL() {
  if (true || (!base::FeatureList::IsEnabled(kTranslateRankerQuery) &&
      !base::FeatureList::IsEnabled(kTranslateRankerEnforcement))) {
    return GURL();
  }
  // Allow override of the ranker model URL from the command line.
  std::string raw_url;
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kTranslateRankerModelURL)) {
    raw_url =
        command_line->GetSwitchValueASCII(switches::kTranslateRankerModelURL);
  } else {
    // Otherwise take the ranker model URL from the ranker query variation.
    raw_url = variations::GetVariationParamValueByFeature(
        kTranslateRankerQuery, switches::kTranslateRankerModelURL);
  }
  // If the ranker URL is still not defined, use the default.
  if (raw_url.empty())
    raw_url = kDefaultTranslateRankerModelURL;

  DVLOG(3) << switches::kTranslateRankerModelURL << " = " << raw_url;

  return GURL(raw_url);
}

void TranslateRankerImpl::EnableLogging(bool value) {
  if (value != is_logging_enabled_) {
    DVLOG(3) << "Cleared translate events cache.";
    event_cache_.clear();
  }
  is_logging_enabled_ = value;
}

uint32_t TranslateRankerImpl::GetModelVersion() const {
  return model_ ? model_->proto().translate().version() : 0;
}

bool TranslateRankerImpl::ShouldOfferTranslation(
    metrics::TranslateEventProto* translate_event) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  // The ranker is a gate in the "show a translation prompt" flow. To retain
  // the pre-existing functionality, it defaults to returning true in the
  // absence of a model or if enforcement is disabled. As this is ranker is
  // subsumed into a more general assist ranker, this default will go away
  // (or become False).
  const bool kDefaultResponse = true;

  translate_event->set_ranker_request_timestamp_sec(
      (base::TimeTicks::Now() - base::TimeTicks()).InSeconds());
  translate_event->set_ranker_version(GetModelVersion());

  if (!is_query_enabled_ && !is_enforcement_enabled_) {
    translate_event->set_ranker_response(
        metrics::TranslateEventProto::NOT_QUERIED);
    return kDefaultResponse;
  }

  if (model_loader_)
    model_loader_->NotifyOfRankerActivity();

  if (model_ == nullptr) {
    translate_event->set_ranker_response(
        metrics::TranslateEventProto::NOT_QUERIED);
    return kDefaultResponse;
  }

  SCOPED_UMA_HISTOGRAM_TIMER("Translate.Ranker.Timer.ShouldOfferTranslation");

  bool result = GetModelDecision(*translate_event);

  UMA_HISTOGRAM_BOOLEAN("Translate.Ranker.QueryResult", result);

  translate_event->set_ranker_response(
      result ? metrics::TranslateEventProto::SHOW
             : metrics::TranslateEventProto::DONT_SHOW);

  if (!is_enforcement_enabled_) {
    return kDefaultResponse;
  }

  return result;
}

bool TranslateRankerImpl::GetModelDecision(
    const metrics::TranslateEventProto& translate_event) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  SCOPED_UMA_HISTOGRAM_TIMER("Translate.Ranker.Timer.CalculateScore");
  DCHECK(model_ != nullptr);

  // TODO(hamelphi): consider deprecating TranslateRankerFeatures and move the
  // logic here.
  const TranslateRankerFeatures features(translate_event);

  const TranslateRankerModel::TranslateLogisticRegressionModel& lr_model =
      model_->proto().translate().translate_logistic_regression_model();

  double dot_product =
      (features.accepted_count * lr_model.accept_count_weight()) +
      (features.denied_count * lr_model.decline_count_weight()) +
      (features.ignored_count * lr_model.ignore_count_weight()) +
      (features.accepted_ratio * lr_model.accept_ratio_weight()) +
      (features.denied_ratio * lr_model.decline_ratio_weight()) +
      (features.ignored_ratio * lr_model.ignore_ratio_weight()) +
      ScoreComponent(lr_model.source_language_weight(), features.src_lang) +
      ScoreComponent(lr_model.target_language_weight(), features.dst_lang) +
      ScoreComponent(lr_model.country_weight(), features.country) +
      ScoreComponent(lr_model.locale_weight(), features.app_locale);

  double score = Sigmoid(dot_product + lr_model.bias());

  DVLOG(2) << "TranslateRankerImpl::GetModelDecision: "
           << "Score = " << score << ", Features=[" << features << "]";

  float decision_threshold = kTranslationOfferDefaultThreshold;
  if (lr_model.threshold() > 0) {
    decision_threshold = lr_model.threshold();
  }

  return score >= decision_threshold;
}

void TranslateRankerImpl::FlushTranslateEvents(
    std::vector<metrics::TranslateEventProto>* events) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  DVLOG(3) << "Flushing translate ranker events.";
  events->swap(event_cache_);
  event_cache_.clear();
}

void TranslateRankerImpl::SendEventToUKM(
    const metrics::TranslateEventProto& event,
    const GURL& url) {
  if (!ukm_recorder_) {
    DVLOG(3) << "No UKM service.";
    return;
  }
  DVLOG(3) << "Sending event for url: " << url.spec();
  ukm::SourceId source_id = ukm_recorder_->GetNewSourceID();
  ukm_recorder_->UpdateSourceURL(source_id, url);
  // TODO(hamelphi): Remove hashing functions once UKM accepts strings metrics.
  ukm::builders::Translate(source_id)
      .SetSourceLanguage(base::HashMetricName(event.source_language()))
      .SetTargetLanguage(base::HashMetricName(event.target_language()))
      .SetCountry(base::HashMetricName(event.country()))
      .SetAcceptCount(event.accept_count())
      .SetDeclineCount(event.decline_count())
      .SetIgnoreCount(event.ignore_count())
      .SetRankerVersion(event.ranker_version())
      .SetRankerResponse(event.ranker_response())
      .SetEventType(event.event_type())
      .Record(ukm_recorder_);
}

void TranslateRankerImpl::AddTranslateEvent(
    const metrics::TranslateEventProto& event,
    const GURL& url) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  if (is_logging_enabled_) {
    DVLOG(3) << "Adding translate ranker event.";
    if (url.is_valid()) {
      SendEventToUKM(event, url);
    }
    event_cache_.push_back(event);
  }
}

void TranslateRankerImpl::OnModelAvailable(std::unique_ptr<RankerModel> model) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  model_ = std::move(model);
}

bool TranslateRankerImpl::CheckModelLoaderForTesting() {
  return model_loader_ != nullptr;
}

void TranslateRankerImpl::RecordTranslateEvent(
    int event_type,
    const GURL& url,
    metrics::TranslateEventProto* translate_event) {
  DCHECK(metrics::TranslateEventProto::EventType_IsValid(event_type));
  translate_event->set_event_type(
      static_cast<metrics::TranslateEventProto::EventType>(event_type));
  translate_event->set_event_timestamp_sec(
      (base::TimeTicks::Now() - base::TimeTicks()).InSeconds());
  AddTranslateEvent(*translate_event, url);
}

bool TranslateRankerImpl::ShouldOverrideDecision(
    int event_type,
    const GURL& url,
    metrics::TranslateEventProto* translate_event) {
  DCHECK(metrics::TranslateEventProto::EventType_IsValid(event_type));
  if ((event_type == metrics::TranslateEventProto::MATCHES_PREVIOUS_LANGUAGE &&
       is_previous_language_matches_override_enabled_) ||
      (event_type ==
           metrics::TranslateEventProto::LANGUAGE_DISABLED_BY_AUTO_BLACKLIST &&
       is_auto_blacklist_override_enabled_)) {
    translate_event->add_decision_overrides(
        static_cast<metrics::TranslateEventProto::EventType>(event_type));
    DVLOG(3) << "Overriding decision of type: " << event_type;
    return true;
  } else {
    RecordTranslateEvent(event_type, url, translate_event);
    return false;
  }
}

}  // namespace translate

std::ostream& operator<<(std::ostream& stream,
                         const translate::TranslateRankerFeatures& features) {
  features.WriteTo(stream);
  return stream;
}
