// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/protocol_manager.h"

#include <utility>

#include "base/environment.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/timer/timer.h"
#include "chrome/browser/safe_browsing/protocol_parser.h"
#include "chrome/common/env_vars.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/safe_browsing/db/util.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/google_api_keys.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

using base::Time;
using base::TimeDelta;

namespace {

// UpdateResult indicates what happened with the primary and/or backup update
// requests. The ordering of the values must stay the same for UMA consistency,
// and is also ordered in this way to match ProtocolManager::BackupUpdateReason.
enum UpdateResult {
  UPDATE_RESULT_FAIL,
  UPDATE_RESULT_SUCCESS,
  UPDATE_RESULT_BACKUP_CONNECT_FAIL,
  UPDATE_RESULT_BACKUP_CONNECT_SUCCESS,
  UPDATE_RESULT_BACKUP_HTTP_FAIL,
  UPDATE_RESULT_BACKUP_HTTP_SUCCESS,
  UPDATE_RESULT_BACKUP_NETWORK_FAIL,
  UPDATE_RESULT_BACKUP_NETWORK_SUCCESS,
  UPDATE_RESULT_MAX,
  UPDATE_RESULT_BACKUP_START = UPDATE_RESULT_BACKUP_CONNECT_FAIL,
};

void RecordUpdateResult(UpdateResult result) {
  DCHECK(result >= 0 && result < UPDATE_RESULT_MAX);
  UMA_HISTOGRAM_ENUMERATION("SB2.UpdateResult", result, UPDATE_RESULT_MAX);
}

constexpr char kSBUpdateFrequencyFinchExperiment[] =
    "SafeBrowsingUpdateFrequency";
constexpr char kSBUpdateFrequencyFinchParam[] = "NextUpdateIntervalInMinutes";

// This will be used for experimenting on a small subset of the population to
// better estimate the benefit of updating the safe browsing hashes more
// frequently.
base::TimeDelta GetNextUpdateIntervalFromFinch() {
  std::string num_str = variations::GetVariationParamValue(
      kSBUpdateFrequencyFinchExperiment, kSBUpdateFrequencyFinchParam);
  int finch_next_update_interval_minutes = 0;
  if (!base::StringToInt(num_str, &finch_next_update_interval_minutes)) {
    finch_next_update_interval_minutes = 0;  // Defaults to 0.
  }
  return base::TimeDelta::FromMinutes(finch_next_update_interval_minutes);
}

constexpr net::NetworkTrafficAnnotationTag
    kChunkBackupRequestTrafficAnnotation = net::DefineNetworkTrafficAnnotation(
        "safe_browsing_chunk_backup_request",
        R"(
        semantics {
          sender: "Safe Browsing"
          description:
            "Safe Browsing updates its local database of bad sites every 30 "
            "minutes or so. It aims to keep all users up-to-date with the same "
            "set of hash-prefixes of bad URLs."
          trigger:
            "On a timer, approximately every 30 minutes."
          data:
            "The state of the local DB is sent so the server can send just the "
            "changes. This doesn't include any user data."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "Safe Browsing cookie store"
          setting:
            "Users can disable Safe Browsing by unchecking 'Protect you and "
            "your device from dangerous sites' in Chromium settings under "
            "Privacy. The feature is enabled by default."
          chrome_policy {
            SafeBrowsingEnabled {
              policy_options {mode: MANDATORY}
              SafeBrowsingEnabled: false
            }
          }
        })");
}  // namespace

namespace safe_browsing {

// Minimum time, in seconds, from start up before we must issue an update query.
constexpr int kSbTimerStartIntervalSecMin = 60;

// Maximum time, in seconds, from start up before we must issue an update query.
constexpr int kSbTimerStartIntervalSecMax = 300;

// The maximum time to wait for a response to an update request.
constexpr base::TimeDelta kSbMaxUpdateWait = base::TimeDelta::FromSeconds(30);

// Maximum back off multiplier.
constexpr size_t kSbMaxBackOff = 8;

constexpr char kGetHashUmaResponseMetricName[] =
    "SB2.GetHashResponseOrErrorCode";
constexpr char kGetChunkUmaResponseMetricName[] =
    "SB2.GetChunkResponseOrErrorCode";

// The default SBProtocolManagerFactory.
class SBProtocolManagerFactoryImpl : public SBProtocolManagerFactory {
 public:
  SBProtocolManagerFactoryImpl() {}
  ~SBProtocolManagerFactoryImpl() override {}

  std::unique_ptr<SafeBrowsingProtocolManager> CreateProtocolManager(
      SafeBrowsingProtocolManagerDelegate* delegate,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const SafeBrowsingProtocolConfig& config) override {
    return base::WrapUnique(
        new SafeBrowsingProtocolManager(delegate, url_loader_factory, config));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SBProtocolManagerFactoryImpl);
};

// SafeBrowsingProtocolManager implementation ----------------------------------

// static
SBProtocolManagerFactory* SafeBrowsingProtocolManager::factory_ = nullptr;

// static
std::unique_ptr<SafeBrowsingProtocolManager>
SafeBrowsingProtocolManager::Create(
    SafeBrowsingProtocolManagerDelegate* delegate,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const SafeBrowsingProtocolConfig& config) {
  if (!factory_)
    factory_ = new SBProtocolManagerFactoryImpl();
  return factory_->CreateProtocolManager(delegate, url_loader_factory, config);
}

SafeBrowsingProtocolManager::SafeBrowsingProtocolManager(
    SafeBrowsingProtocolManagerDelegate* delegate,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const SafeBrowsingProtocolConfig& config)
    : delegate_(delegate),
      request_type_(NO_REQUEST),
      update_error_count_(0),
      gethash_error_count_(0),
      update_back_off_mult_(1),
      gethash_back_off_mult_(1),
      next_update_interval_(base::TimeDelta::FromSeconds(
          base::RandInt(kSbTimerStartIntervalSecMin,
                        kSbTimerStartIntervalSecMax))),
      chunk_pending_to_write_(false),
      version_(config.version),
      update_size_(0),
      client_name_(config.client_name),
      url_loader_factory_(url_loader_factory),
      url_prefix_(config.url_prefix),
      backup_update_reason_(BACKUP_UPDATE_REASON_MAX),
      disable_auto_update_(config.disable_auto_update) {
  DCHECK(!url_prefix_.empty());

  backup_url_prefixes_[BACKUP_UPDATE_REASON_CONNECT] =
      config.backup_connect_error_url_prefix;
  backup_url_prefixes_[BACKUP_UPDATE_REASON_HTTP] =
      config.backup_http_error_url_prefix;
  backup_url_prefixes_[BACKUP_UPDATE_REASON_NETWORK] =
      config.backup_network_error_url_prefix;

  // Set the backoff multiplier fuzz to a random value between 0 and 1.
  back_off_fuzz_ = static_cast<float>(base::RandDouble());
  if (version_.empty())
    version_ = ProtocolManagerHelper::Version();
}

// static
void SafeBrowsingProtocolManager::RecordGetHashResult(bool is_download,
                                                      ResultType result_type) {
  if (is_download) {
    UMA_HISTOGRAM_ENUMERATION("SB2.GetHashResultDownload", result_type,
                              GET_HASH_RESULT_MAX);
  } else {
    UMA_HISTOGRAM_ENUMERATION("SB2.GetHashResult", result_type,
                              GET_HASH_RESULT_MAX);
  }
}

void SafeBrowsingProtocolManager::RecordHttpResponseOrErrorCode(
    const char* metric_name,
    int net_error,
    int response_code) {
  base::UmaHistogramSparse(metric_name,
                           net_error == net::OK ? response_code : net_error);
}

bool SafeBrowsingProtocolManager::IsUpdateScheduled() const {
  return update_timer_.IsRunning();
}

// static
base::TimeDelta SafeBrowsingProtocolManager::GetUpdateTimeoutForTesting() {
  return kSbMaxUpdateWait;
}

SafeBrowsingProtocolManager::~SafeBrowsingProtocolManager() {}

// We can only have one update or chunk request outstanding, but there may be
// multiple GetHash requests pending since we don't want to serialize them and
// slow down the user.
void SafeBrowsingProtocolManager::GetFullHash(
    const std::vector<SBPrefix>& prefixes,
    FullHashCallback callback,
    bool is_download,
    ExtendedReportingLevel reporting_level) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // If we are in GetHash backoff, we need to check if we're past the next
  // allowed time. If we are, we can proceed with the request. If not, we are
  // required to return empty results (i.e. treat the page as safe).
  if (gethash_error_count_ && Time::Now() <= next_gethash_time_) {
    RecordGetHashResult(is_download, GET_HASH_BACKOFF_ERROR);
    std::vector<SBFullHashResult> full_hashes;
    callback.Run(full_hashes, base::TimeDelta());
    return;
  }
  GURL gethash_url = GetHashUrl(reporting_level);
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("safe_browsing_get_full_hash", R"(
        semantics {
          sender: "Safe Browsing"
          description:
            "When Safe Browsing detects that a URL might be dangerous based on "
            "its local database, it sends a partial hash of that URL to Google "
            "to verify it before showing a warning to the user. This partial "
            "hash does not expose the URL to Google."
          trigger:
            "When a resource URL matches the local hash-prefix database of "
            "potential threats (malware, phishing etc), and the full-hash "
            "result is not already cached, this will be sent."
          data:
             "The 32-bit hash prefix of any potentially bad URLs. The URLs "
             "themselves are not sent."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "Safe Browsing cookie store"
          setting:
            "Users can disable Safe Browsing by unchecking 'Protect you and "
            "your device from dangerous sites' in Chromium settings under "
            "Privacy. The feature is enabled by default."
          chrome_policy {
            SafeBrowsingEnabled {
              policy_options {mode: MANDATORY}
              SafeBrowsingEnabled: false
            }
          }
        })");
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = gethash_url;
  resource_request->method = "POST";
  resource_request->load_flags = net::LOAD_DISABLE_CACHE;
  auto loader_ptr = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  loader_ptr->AttachStringForUpload(FormatGetHash(prefixes), "text/plain");
  auto* loader = loader_ptr.get();
  loader_ptr->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&SafeBrowsingProtocolManager::OnURLLoaderComplete,
                     base::Unretained(this), loader));

  hash_requests_[loader] = {std::move(loader_ptr),
                            FullHashDetails(callback, is_download)};
}

void SafeBrowsingProtocolManager::GetNextUpdate() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (request_.get() || request_type_ != NO_REQUEST)
    return;

  IssueUpdateRequest();
}

// All SafeBrowsing request responses are handled here.
// TODO(paulg): Clarify with the SafeBrowsing team whether a failed parse of a
//              chunk should retry the download and parse of that chunk (and
//              what back off / how many times to try), and if that effects the
//              update back off. For now, a failed parse of the chunk means we
//              drop it. This isn't so bad because the next UPDATE_REQUEST we
//              do will report all the chunks we have. If that chunk is still
//              required, the SafeBrowsing servers will tell us to get it again.
void SafeBrowsingProtocolManager::OnURLLoaderComplete(
    network::SimpleURLLoader* url_loader,
    std::unique_ptr<std::string> response_body) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  int response_code = 0;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();

  std::string data;
  if (response_body)
    data = *response_body.get();

  OnURLLoaderCompleteInternal(url_loader, url_loader->NetError(), response_code,
                              data);
}

void SafeBrowsingProtocolManager::OnURLLoaderCompleteInternal(
    network::SimpleURLLoader* url_loader,
    int net_error,
    int response_code,
    const std::string& data) {
  auto it = hash_requests_.find(url_loader);
  if (it != hash_requests_.end()) {
    // GetHash response.
    RecordHttpResponseOrErrorCode(kGetHashUmaResponseMetricName, net_error,
                                  response_code);
    const FullHashDetails& details = it->second.second;
    std::vector<SBFullHashResult> full_hashes;
    base::TimeDelta cache_lifetime;
    if (net_error == net::OK && (response_code == net::HTTP_OK ||
                                 response_code == net::HTTP_NO_CONTENT)) {
      // For tracking our GetHash false positive (net::HTTP_NO_CONTENT) rate,
      // compared to real (net::HTTP_OK) responses.
      if (response_code == net::HTTP_OK)
        RecordGetHashResult(details.is_download, GET_HASH_STATUS_200);
      else
        RecordGetHashResult(details.is_download, GET_HASH_STATUS_204);

      gethash_error_count_ = 0;
      gethash_back_off_mult_ = 1;
      if (!ParseGetHash(data.data(), data.length(), &cache_lifetime,
                        &full_hashes)) {
        full_hashes.clear();
        RecordGetHashResult(details.is_download, GET_HASH_PARSE_ERROR);
        // TODO(cbentzel): Should cache_lifetime be set to 0 here? (See
        // http://crbug.com/360232.)
      }
    } else {
      HandleGetHashError(Time::Now());
      if (net_error != net::OK) {
        RecordGetHashResult(details.is_download, GET_HASH_NETWORK_ERROR);
        DVLOG(1) << "SafeBrowsing GetHash request for: "
                 << url_loader->GetFinalURL()
                 << " failed with error: " << net_error;
      } else {
        RecordGetHashResult(details.is_download, GET_HASH_HTTP_ERROR);
        DVLOG(1) << "SafeBrowsing GetHash request for: "
                 << url_loader->GetFinalURL()
                 << " failed with error: " << response_code;
      }
    }

    // Invoke the callback with full_hashes, even if there was a parse error or
    // an error response code (in which case full_hashes will be empty). The
    // caller can't be blocked indefinitely.
    details.callback.Run(full_hashes, cache_lifetime);

    hash_requests_.erase(it);
  } else {
    // Update or chunk response.
    RecordHttpResponseOrErrorCode(kGetChunkUmaResponseMetricName, net_error,
                                  response_code);
    std::unique_ptr<network::SimpleURLLoader> loader = std::move(request_);

    if (request_type_ == UPDATE_REQUEST ||
        request_type_ == BACKUP_UPDATE_REQUEST) {
      if (!loader.get()) {
        // We've timed out waiting for an update response, so we've cancelled
        // the update request and scheduled a new one. Ignore this response.
        return;
      }

      // Cancel the update response timeout now that we have the response.
      timeout_timer_.Stop();
    }

    if (net_error == net::OK && response_code == net::HTTP_OK) {
      // We have data from the SafeBrowsing service.
      // TODO(shess): Cleanup the flow of this code so that |parsed_ok| can be
      // removed or omitted.
      const bool parsed_ok = HandleServiceResponse(data.data(), data.length());
      if (!parsed_ok) {
        DVLOG(1) << "SafeBrowsing request for: " << loader->GetFinalURL()
                 << " failed parse.";
        chunk_request_urls_.clear();
        if (request_type_ == UPDATE_REQUEST &&
            IssueBackupUpdateRequest(BACKUP_UPDATE_REASON_HTTP)) {
          return;
        }
        UpdateFinished(false);
      }

      switch (request_type_) {
        case CHUNK_REQUEST:
          if (parsed_ok) {
            chunk_request_urls_.pop_front();
            if (chunk_request_urls_.empty() && !chunk_pending_to_write_)
              UpdateFinished(true);
          }
          break;
        case UPDATE_REQUEST:
        case BACKUP_UPDATE_REQUEST:
          if (chunk_request_urls_.empty() && parsed_ok) {
            // We are up to date since the servers gave us nothing new, so we
            // are done with this update cycle.
            UpdateFinished(true);
          }
          break;
        case NO_REQUEST:
          // This can happen if HandleServiceResponse fails above.
          break;
        default:
          NOTREACHED();
          break;
      }
    } else {
      if (net_error != net::OK) {
        DVLOG(1) << "SafeBrowsing request for: " << loader->GetFinalURL()
                 << " failed with error: " << net_error;
      } else {
        DVLOG(1) << "SafeBrowsing request for: " << loader->GetFinalURL()
                 << " failed with error: " << response_code;
      }
      if (request_type_ == CHUNK_REQUEST) {
        // The SafeBrowsing service error, or very bad response code: back off.
        chunk_request_urls_.clear();
      } else if (request_type_ == UPDATE_REQUEST) {
        BackupUpdateReason backup_update_reason = BACKUP_UPDATE_REASON_MAX;
        if (net_error == net::OK) {
          backup_update_reason = BACKUP_UPDATE_REASON_HTTP;
        } else {
          switch (net_error) {
            case net::ERR_INTERNET_DISCONNECTED:
            case net::ERR_NETWORK_CHANGED:
              backup_update_reason = BACKUP_UPDATE_REASON_NETWORK;
              break;
            default:
              backup_update_reason = BACKUP_UPDATE_REASON_CONNECT;
              break;
          }
        }
        if (backup_update_reason != BACKUP_UPDATE_REASON_MAX &&
            IssueBackupUpdateRequest(backup_update_reason)) {
          return;
        }
      }
      UpdateFinished(false);
    }

    // Get the next chunk if available.
    IssueChunkRequest();
  }
}

bool SafeBrowsingProtocolManager::HandleServiceResponse(const char* data,
                                                        size_t length) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  switch (request_type_) {
    case UPDATE_REQUEST:
    case BACKUP_UPDATE_REQUEST: {
      size_t next_update_sec = 0;
      bool reset = false;
      std::unique_ptr<std::vector<SBChunkDelete>> chunk_deletes(
          new std::vector<SBChunkDelete>);
      std::vector<ChunkUrl> chunk_urls;
      if (!ParseUpdate(data, length, &next_update_sec, &reset,
                       chunk_deletes.get(), &chunk_urls)) {
        return false;
      }

      // New time for the next update.
      base::TimeDelta finch_next_update_interval =
          GetNextUpdateIntervalFromFinch();
      if (finch_next_update_interval > base::TimeDelta()) {
        next_update_interval_ = finch_next_update_interval;
      } else {
        base::TimeDelta next_update_interval =
            base::TimeDelta::FromSeconds(next_update_sec);
        if (next_update_interval > base::TimeDelta()) {
          next_update_interval_ = next_update_interval;
        }
      }
      last_update_ = Time::Now();

      // New chunks to download.
      if (!chunk_urls.empty()) {
        UMA_HISTOGRAM_COUNTS("SB2.UpdateUrls", chunk_urls.size());
        for (size_t i = 0; i < chunk_urls.size(); ++i)
          chunk_request_urls_.push_back(chunk_urls[i]);
      }

      // Handle the case were the SafeBrowsing service tells us to dump our
      // database.
      if (reset) {
        delegate_->ResetDatabase();
        return true;
      }

      // Chunks to delete from our storage.
      if (!chunk_deletes->empty())
        delegate_->DeleteChunks(std::move(chunk_deletes));

      break;
    }
    case CHUNK_REQUEST: {
      UMA_HISTOGRAM_TIMES("SB2.ChunkRequest",
                          base::Time::Now() - chunk_request_start_);

      const ChunkUrl chunk_url = chunk_request_urls_.front();
      std::unique_ptr<std::vector<std::unique_ptr<SBChunkData>>> chunks(
          new std::vector<std::unique_ptr<SBChunkData>>);
      UMA_HISTOGRAM_COUNTS("SB2.ChunkSize", length);
      update_size_ += length;
      if (!ParseChunk(data, length, chunks.get()))
        return false;

      // Chunks to add to storage.  Pass ownership of |chunks|.
      if (!chunks->empty()) {
        chunk_pending_to_write_ = true;
        delegate_->AddChunks(
            chunk_url.list_name, std::move(chunks),
            base::Bind(&SafeBrowsingProtocolManager::OnAddChunksComplete,
                       base::Unretained(this)));
      }

      break;
    }

    default:
      return false;
  }

  return true;
}

void SafeBrowsingProtocolManager::Initialize() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // Don't want to hit the safe browsing servers on build/chrome bots.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (env->HasVar(env_vars::kHeadless))
    return;
  ScheduleNextUpdate(false /* no back off */);
}

void SafeBrowsingProtocolManager::ScheduleNextUpdate(bool back_off) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (disable_auto_update_) {
    // Unschedule any current timer.
    update_timer_.Stop();
    return;
  }
  // Reschedule with the new update.
  base::TimeDelta next_update_interval = GetNextUpdateInterval(back_off);
  ForceScheduleNextUpdate(next_update_interval);
}

void SafeBrowsingProtocolManager::ForceScheduleNextUpdate(
    base::TimeDelta interval) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(interval >= base::TimeDelta());
  // Unschedule any current timer.
  update_timer_.Stop();
  update_timer_.Start(FROM_HERE, interval, this,
                      &SafeBrowsingProtocolManager::GetNextUpdate);
}

// According to section 5 of the SafeBrowsing protocol specification, we must
// back off after a certain number of errors. We only change |next_update_sec_|
// when we receive a response from the SafeBrowsing service.
base::TimeDelta SafeBrowsingProtocolManager::GetNextUpdateInterval(
    bool back_off) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(next_update_interval_ > base::TimeDelta());
  base::TimeDelta next = next_update_interval_;
  if (back_off) {
    next = GetNextBackOffInterval(&update_error_count_, &update_back_off_mult_);
  } else {
    // Successful response means error reset.
    update_error_count_ = 0;
    update_back_off_mult_ = 1;
  }
  return next;
}

base::TimeDelta SafeBrowsingProtocolManager::GetNextBackOffInterval(
    size_t* error_count,
    size_t* multiplier) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(multiplier && error_count);
  (*error_count)++;
  if (*error_count > 1 && *error_count < 6) {
    base::TimeDelta next =
        base::TimeDelta::FromMinutes(*multiplier * (1 + back_off_fuzz_) * 30);
    *multiplier *= 2;
    if (*multiplier > kSbMaxBackOff)
      *multiplier = kSbMaxBackOff;
    return next;
  }
  if (*error_count >= 6)
    return base::TimeDelta::FromHours(8);
  return base::TimeDelta::FromMinutes(1);
}

// This request requires getting a list of all the chunks for each list from the
// database asynchronously. The request will be issued when we're called back in
// OnGetChunksComplete.
// TODO(paulg): We should get this at start up and maintain a ChunkRange cache
//              to avoid hitting the database with each update request. On the
//              otherhand, this request will only occur ~20-30 minutes so there
//              isn't that much overhead. Measure!
void SafeBrowsingProtocolManager::IssueUpdateRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  request_type_ = UPDATE_REQUEST;
  delegate_->UpdateStarted();
  delegate_->GetChunks(
      base::Bind(&SafeBrowsingProtocolManager::OnGetChunksComplete,
                 base::Unretained(this)));
}

// The backup request can run immediately since the chunks have already been
// retrieved from the DB.
bool SafeBrowsingProtocolManager::IssueBackupUpdateRequest(
    BackupUpdateReason backup_update_reason) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK_EQ(request_type_, UPDATE_REQUEST);
  DCHECK(backup_update_reason >= 0 &&
         backup_update_reason < BACKUP_UPDATE_REASON_MAX);
  if (backup_url_prefixes_[backup_update_reason].empty())
    return false;
  request_type_ = BACKUP_UPDATE_REQUEST;
  backup_update_reason_ = backup_update_reason;

  GURL backup_update_url = BackupUpdateUrl(backup_update_reason);
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("safe_browsing_backup_request", R"(
        semantics {
          sender: "Safe Browsing"
          description:
            "Safe Browsing issues multi-step update requests to Google every "
            "30 minutes or so to get the latest database of hashes of bad URLs."
          trigger:
            "On a timer, approximately every 30 minutes."
          data:
            "The state of the local DB is sent so the server can send just the "
            "changes. This doesn't include any user data."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "Safe Browsing cookie store"
          setting:
            "Users can disable Safe Browsing by unchecking 'Protect you and "
            "your device from dangerous sites' in Chromium settings under "
            "Privacy. The feature is enabled by default."
          chrome_policy {
            SafeBrowsingEnabled {
              policy_options {mode: MANDATORY}
              SafeBrowsingEnabled: false
            }
          }
        })");
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = backup_update_url;
  resource_request->method = "POST";
  resource_request->load_flags = net::LOAD_DISABLE_CACHE;
  request_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                              traffic_annotation);
  request_->AttachStringForUpload(update_list_data_, "text/plain");
  request_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&SafeBrowsingProtocolManager::OnURLLoaderComplete,
                     base::Unretained(this), request_.get()));

  // Begin the update request timeout.
  timeout_timer_.Start(FROM_HERE, kSbMaxUpdateWait, this,
                       &SafeBrowsingProtocolManager::UpdateResponseTimeout);

  return true;
}

void SafeBrowsingProtocolManager::IssueChunkRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // We are only allowed to have one request outstanding at any time.  Also,
  // don't get the next url until the previous one has been written to disk so
  // that we don't use too much memory.
  if (request_.get() || chunk_request_urls_.empty() || chunk_pending_to_write_)
    return;

  ChunkUrl next_chunk = chunk_request_urls_.front();
  DCHECK(!next_chunk.url.empty());
  GURL chunk_url = NextChunkUrl(next_chunk.url);
  request_type_ = CHUNK_REQUEST;

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = chunk_url;
  resource_request->load_flags = net::LOAD_DISABLE_CACHE;
  request_ = network::SimpleURLLoader::Create(
      std::move(resource_request), kChunkBackupRequestTrafficAnnotation);
  request_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&SafeBrowsingProtocolManager::OnURLLoaderComplete,
                     base::Unretained(this), request_.get()));

  chunk_request_start_ = base::Time::Now();
}

void SafeBrowsingProtocolManager::OnGetChunksComplete(
    const std::vector<SBListChunkRanges>& lists,
    bool database_error,
    ExtendedReportingLevel extended_reporting_level) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK_EQ(request_type_, UPDATE_REQUEST);
  DCHECK(update_list_data_.empty());
  if (database_error) {
    // The update was not successful, but don't back off.
    UpdateFinished(false, false);
    return;
  }

  // Format our stored chunks:
  bool found_malware = false;
  bool found_phishing = false;
  for (size_t i = 0; i < lists.size(); ++i) {
    update_list_data_.append(FormatList(lists[i]));
    if (lists[i].name == kPhishingList)
      found_phishing = true;

    if (lists[i].name == kMalwareList)
      found_malware = true;
  }

  // If we have an empty database, let the server know we want data for these
  // lists.
  // TODO(shess): These cases never happen because the database fills in the
  // lists in GetChunks().  Refactor the unit tests so that this code can be
  // removed.
  if (!found_phishing) {
    update_list_data_.append(FormatList(SBListChunkRanges(kPhishingList)));
  }
  if (!found_malware) {
    update_list_data_.append(FormatList(SBListChunkRanges(kMalwareList)));
  }

  // Large requests are (probably) a sign of database corruption.
  // Record stats to inform decisions about whether to automate
  // deletion of such databases.  http://crbug.com/120219
  UMA_HISTOGRAM_COUNTS("SB2.UpdateRequestSize", update_list_data_.size());

  GURL update_url = UpdateUrl(extended_reporting_level);

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = update_url;
  resource_request->method = "POST";
  resource_request->load_flags = net::LOAD_DISABLE_CACHE;
  request_ = network::SimpleURLLoader::Create(
      std::move(resource_request), kChunkBackupRequestTrafficAnnotation);
  request_->AttachStringForUpload(update_list_data_, "text/plain");
  request_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&SafeBrowsingProtocolManager::OnURLLoaderComplete,
                     base::Unretained(this), request_.get()));

  // Begin the update request timeout.
  timeout_timer_.Start(FROM_HERE, kSbMaxUpdateWait, this,
                       &SafeBrowsingProtocolManager::UpdateResponseTimeout);
}

// If we haven't heard back from the server with an update response, this method
// will run. Close the current update session and schedule another update.
void SafeBrowsingProtocolManager::UpdateResponseTimeout() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(request_type_ == UPDATE_REQUEST ||
         request_type_ == BACKUP_UPDATE_REQUEST);
  request_.reset();
  if (request_type_ == UPDATE_REQUEST &&
      IssueBackupUpdateRequest(BACKUP_UPDATE_REASON_CONNECT)) {
    return;
  }
  UpdateFinished(false);
}

void SafeBrowsingProtocolManager::OnAddChunksComplete() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  chunk_pending_to_write_ = false;

  if (chunk_request_urls_.empty()) {
    UMA_HISTOGRAM_LONG_TIMES("SB2.Update", Time::Now() - last_update_);
    UpdateFinished(true);
  } else {
    IssueChunkRequest();
  }
}

void SafeBrowsingProtocolManager::HandleGetHashError(const Time& now) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  base::TimeDelta next =
      GetNextBackOffInterval(&gethash_error_count_, &gethash_back_off_mult_);
  next_gethash_time_ = now + next;
}

void SafeBrowsingProtocolManager::UpdateFinished(bool success) {
  UpdateFinished(success, !success);
}

void SafeBrowsingProtocolManager::UpdateFinished(bool success, bool back_off) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  UMA_HISTOGRAM_COUNTS("SB2.UpdateSize", update_size_);
  update_size_ = 0;
  bool update_success = success || request_type_ == CHUNK_REQUEST;
  if (backup_update_reason_ == BACKUP_UPDATE_REASON_MAX) {
    RecordUpdateResult(update_success ? UPDATE_RESULT_SUCCESS
                                      : UPDATE_RESULT_FAIL);
  } else {
    UpdateResult update_result = static_cast<UpdateResult>(
        UPDATE_RESULT_BACKUP_START +
        (static_cast<int>(backup_update_reason_) * 2) + update_success);
    RecordUpdateResult(update_result);
  }
  backup_update_reason_ = BACKUP_UPDATE_REASON_MAX;
  request_type_ = NO_REQUEST;
  update_list_data_.clear();
  delegate_->UpdateFinished(success);
  ScheduleNextUpdate(back_off);
}

GURL SafeBrowsingProtocolManager::UpdateUrl(
    ExtendedReportingLevel reporting_level) const {
  std::string url = ProtocolManagerHelper::ComposeUrl(
      url_prefix_, "downloads", client_name_, version_, additional_query_,
      reporting_level);
  return GURL(url);
}

GURL SafeBrowsingProtocolManager::BackupUpdateUrl(
    BackupUpdateReason backup_update_reason) const {
  DCHECK(backup_update_reason >= 0 &&
         backup_update_reason < BACKUP_UPDATE_REASON_MAX);
  DCHECK(!backup_url_prefixes_[backup_update_reason].empty());
  std::string url = ProtocolManagerHelper::ComposeUrl(
      backup_url_prefixes_[backup_update_reason], "downloads", client_name_,
      version_, additional_query_);
  return GURL(url);
}

GURL SafeBrowsingProtocolManager::GetHashUrl(
    ExtendedReportingLevel reporting_level) const {
  std::string url = ProtocolManagerHelper::ComposeUrl(
      url_prefix_, "gethash", client_name_, version_, additional_query_,
      reporting_level);
  return GURL(url);
}

GURL SafeBrowsingProtocolManager::NextChunkUrl(const std::string& url) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  std::string next_url;
  if (!base::StartsWith(url, "http://", base::CompareCase::INSENSITIVE_ASCII) &&
      !base::StartsWith(url, "https://",
                        base::CompareCase::INSENSITIVE_ASCII)) {
    // Use https if we updated via https, otherwise http (useful for testing).
    if (base::StartsWith(url_prefix_, "https://",
                         base::CompareCase::INSENSITIVE_ASCII))
      next_url.append("https://");
    else
      next_url.append("http://");
    next_url.append(url);
  } else {
    next_url = url;
  }
  if (!additional_query_.empty()) {
    if (next_url.find("?") != std::string::npos) {
      next_url.append("&");
    } else {
      next_url.append("?");
    }
    next_url.append(additional_query_);
  }
  return GURL(next_url);
}

SafeBrowsingProtocolManager::FullHashDetails::FullHashDetails()
    : callback(), is_download(false) {}

SafeBrowsingProtocolManager::FullHashDetails::FullHashDetails(
    FullHashCallback callback,
    bool is_download)
    : callback(callback), is_download(is_download) {}

SafeBrowsingProtocolManager::FullHashDetails::FullHashDetails(
    const FullHashDetails& other) = default;

SafeBrowsingProtocolManager::FullHashDetails::~FullHashDetails() {}

SafeBrowsingProtocolManagerDelegate::~SafeBrowsingProtocolManagerDelegate() {}

}  // namespace safe_browsing
