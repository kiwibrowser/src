// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/content/previews_optimization_guide.h"

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "components/optimization_guide/proto/hints.pb.h"
#include "components/previews/core/previews_user_data.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace previews {

namespace {

// Enumerates the possible outcomes of processing previews hints. Used in UMA
// histograms, so the order of enumerators should not be changed.
//
// Keep in sync with PreviewsProcessHintsResult in
// tools/metrics/histograms/enums.xml.
enum class PreviewsProcessHintsResult {
  PROCESSED_NO_PREVIEWS_HINTS = 0,
  PROCESSED_PREVIEWS_HINTS = 1,
  FAILED_FINISH_PROCESSING = 2,

  // Insert new values before this line.
  MAX,
};

void RecordProcessHintsResult(PreviewsProcessHintsResult result) {
  UMA_HISTOGRAM_ENUMERATION("Previews.ProcessHintsResult",
                            static_cast<int>(result),
                            static_cast<int>(PreviewsProcessHintsResult::MAX));
}

// Name of sentinel file to guard potential crash loops while processing
// the config into hints. It holds the version of the config that is/was
// being processed into hints.
const base::FilePath::CharType kSentinelFileName[] =
    FILE_PATH_LITERAL("previews_config_sentinel.txt");

// Creates the sentinel file (at |sentinel_path|) to persistently mark the
// beginning of processing the configuration data for Previews hints. It
// records the configuration version in the file. Returns true when the
// sentinel file is successfully created and processing should continue.
// Returns false if the processing should not continue because the
// file exists with the same version (indicating that processing that version
// failed previously (possibly crash or shutdown). Should be run in the
// background (e.g., same task as Hints.CreateFromConfig).
bool CreateSentinelFile(const base::FilePath& sentinel_path,
                        const base::Version& version) {
  DCHECK(version.IsValid());

  if (base::PathExists(sentinel_path)) {
    // Processing apparently did not complete previously, check its version.
    std::string content;
    if (!base::ReadFileToString(sentinel_path, &content)) {
      DLOG(WARNING) << "Error reading previews config sentinel file";
      // Attempt to delete sentinel for fresh start next time.
      base::DeleteFile(sentinel_path, false /* recursive */);
      return false;
    }
    base::Version previous_attempted_version(content);
    if (!previous_attempted_version.IsValid()) {
      DLOG(ERROR) << "Bad contents in previews config sentinel file";
      // Attempt to delete sentinel for fresh start next time.
      base::DeleteFile(sentinel_path, false /* recursive */);
      return false;
    }
    if (previous_attempted_version.CompareTo(version) == 0) {
      // Previously attempted same version without completion.
      return false;
    }
  }

  // Write config version in the sentinel file.
  std::string new_sentinel_value = version.GetString();
  if (base::WriteFile(sentinel_path, new_sentinel_value.data(),
                      new_sentinel_value.length()) <= 0) {
    DLOG(ERROR) << "Failed to create sentinel file " << sentinel_path;
    return false;
  }
  return true;
}

// Deletes the sentinel file. This should be done once processing the
// configuration is complete and should be done in the background (e.g.,
// same task as Hints.CreateFromConfig).
void DeleteSentinelFile(const base::FilePath& sentinel_path) {
  if (!base::DeleteFile(sentinel_path, false /* recursive */))
    DLOG(ERROR) << "Error deleting sentinel file";
}

}  // namespace

// Holds previews hints extracted from the configuration sent by the
// Optimization Guide Service.
class PreviewsOptimizationGuide::Hints {
 public:
  ~Hints();

  // Creates a Hints instance from the provided configuration.
  static std::unique_ptr<Hints> CreateFromConfig(
      const optimization_guide::proto::Configuration& config,
      const optimization_guide::ComponentInfo& info);

  // Whether the URL is whitelisted for the given previews type. If so,
  // |out_inflation_percent| will be populated if meta data available for it.
  bool IsWhitelisted(const GURL& url,
                     PreviewsType type,
                     int* out_inflation_percent);

 private:
  Hints();

  // The URLMatcher used to match whether a URL has any hints associated with
  // it.
  url_matcher::URLMatcher url_matcher_;

  // A map from the condition set ID to associated whitelist Optimization
  // details.
  std::map<url_matcher::URLMatcherConditionSet::ID,
           std::set<std::pair<PreviewsType, int>>>
      whitelist_;
};

PreviewsOptimizationGuide::Hints::Hints() {}

PreviewsOptimizationGuide::Hints::~Hints() {}

// static
std::unique_ptr<PreviewsOptimizationGuide::Hints>
PreviewsOptimizationGuide::Hints::CreateFromConfig(
    const optimization_guide::proto::Configuration& config,
    const optimization_guide::ComponentInfo& info) {
  base::FilePath sentinel_path(
      info.hints_path.DirName().Append(kSentinelFileName));
  if (!CreateSentinelFile(sentinel_path, info.hints_version)) {
    std::unique_ptr<Hints> no_hints;
    RecordProcessHintsResult(
        PreviewsProcessHintsResult::FAILED_FINISH_PROCESSING);
    return no_hints;
  }

  std::unique_ptr<Hints> hints(new Hints());

  // The condition set ID is a simple increasing counter that matches the
  // order of hints in the config (where earlier hints in the config take
  // precendence over later hints in the config if there are multiple matches).
  url_matcher::URLMatcherConditionSet::ID id = 0;
  url_matcher::URLMatcherConditionFactory* condition_factory =
      hints->url_matcher_.condition_factory();
  url_matcher::URLMatcherConditionSet::Vector all_conditions;
  std::set<std::string> seen_host_suffixes;

  // Process hint configuration.
  for (const auto hint : config.hints()) {
    // We only support host suffixes at the moment. Skip anything else.
    if (hint.key_representation() != optimization_guide::proto::HOST_SUFFIX)
      continue;

    // Validate configuration keys.
    DCHECK(!hint.key().empty());
    if (hint.key().empty())
      continue;

    auto seen_host_suffixes_iter = seen_host_suffixes.find(hint.key());
    DCHECK(seen_host_suffixes_iter == seen_host_suffixes.end());
    if (seen_host_suffixes_iter != seen_host_suffixes.end()) {
      DLOG(WARNING) << "Received config with duplicate key";
      continue;
    }
    seen_host_suffixes.insert(hint.key());

    // Create whitelist condition set out of the optimizations that are
    // whitelisted for the host suffix.
    std::set<std::pair<PreviewsType, int>> whitelisted_optimizations;
    for (const auto optimization : hint.whitelisted_optimizations()) {
      if (optimization.optimization_type() ==
          optimization_guide::proto::NOSCRIPT) {
        whitelisted_optimizations.insert(std::make_pair(
            PreviewsType::NOSCRIPT, optimization.inflation_percent()));
      }
    }
    url_matcher::URLMatcherCondition condition =
        condition_factory->CreateHostSuffixCondition(hint.key());
    all_conditions.push_back(new url_matcher::URLMatcherConditionSet(
        id, std::set<url_matcher::URLMatcherCondition>{condition}));
    hints->whitelist_[id] = whitelisted_optimizations;
    id++;
  }
  hints->url_matcher_.AddConditionSets(all_conditions);
  // Completed processing hints data without crashing so clear sentinel.
  DeleteSentinelFile(sentinel_path);
  RecordProcessHintsResult(
      all_conditions.empty()
          ? PreviewsProcessHintsResult::PROCESSED_NO_PREVIEWS_HINTS
          : PreviewsProcessHintsResult::PROCESSED_PREVIEWS_HINTS);
  return hints;
}

bool PreviewsOptimizationGuide::Hints::IsWhitelisted(
    const GURL& url,
    PreviewsType type,
    int* out_inflation_percent) {
  std::set<url_matcher::URLMatcherConditionSet::ID> matches =
      url_matcher_.MatchURL(url);

  // Only consider the first match in iteration order as it takes precendence
  // if there are multiple matches.
  const auto& first_match = matches.begin();
  if (first_match == matches.end()) {
    return false;
  }

  const auto whitelist_iter = whitelist_.find(*first_match);
  if (whitelist_iter == whitelist_.end()) {
    return false;
  }

  const auto& whitelisted_optimizations = whitelist_iter->second;
  for (auto optimization_iter = whitelisted_optimizations.begin();
       optimization_iter != whitelisted_optimizations.end();
       ++optimization_iter) {
    if (optimization_iter->first == type) {
      *out_inflation_percent = optimization_iter->second;
      return true;
    }
  }
  return false;
}

PreviewsOptimizationGuide::PreviewsOptimizationGuide(
    optimization_guide::OptimizationGuideService* optimization_guide_service,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : optimization_guide_service_(optimization_guide_service),
      io_task_runner_(io_task_runner),
      background_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND})),
      io_weak_ptr_factory_(this) {
  DCHECK(optimization_guide_service_);
  optimization_guide_service_->AddObserver(this);
}

PreviewsOptimizationGuide::~PreviewsOptimizationGuide() {
  optimization_guide_service_->RemoveObserver(this);
}

bool PreviewsOptimizationGuide::IsWhitelisted(const net::URLRequest& request,
                                              PreviewsType type) const {
  if (!hints_)
    return false;

  int inflation_percent = 0;
  if (!hints_->IsWhitelisted(request.url(), type, &inflation_percent))
    return false;

  previews::PreviewsUserData* previews_user_data =
      previews::PreviewsUserData::GetData(request);
  if (inflation_percent != 0 && previews_user_data)
    previews_user_data->SetDataSavingsInflationPercent(inflation_percent);

  return true;
}

void PreviewsOptimizationGuide::OnHintsProcessed(
    const optimization_guide::proto::Configuration& config,
    const optimization_guide::ComponentInfo& info) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  base::PostTaskAndReplyWithResult(
      background_task_runner_.get(), FROM_HERE,
      base::BindOnce(&PreviewsOptimizationGuide::Hints::CreateFromConfig,
                     config, info),
      base::BindOnce(&PreviewsOptimizationGuide::UpdateHints,
                     io_weak_ptr_factory_.GetWeakPtr()));
}

void PreviewsOptimizationGuide::UpdateHints(std::unique_ptr<Hints> hints) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  hints_ = std::move(hints);
}

}  // namespace previews
