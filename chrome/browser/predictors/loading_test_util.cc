// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/loading_test_util.h"

#include <cmath>
#include <memory>
#include <utility>

#include "content/public/browser/resource_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"

namespace {

class EmptyURLRequestDelegate : public net::URLRequest::Delegate {
  void OnResponseStarted(net::URLRequest* request, int net_error) override {}
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override {}
};

EmptyURLRequestDelegate g_empty_url_request_delegate;

bool AlmostEqual(const double x, const double y) {
  return std::fabs(x - y) <= 1e-6;  // Arbitrary but close enough.
}

}  // namespace

namespace predictors {

MockResourcePrefetchPredictor::MockResourcePrefetchPredictor(
    const LoadingPredictorConfig& config,
    Profile* profile)
    : ResourcePrefetchPredictor(config, profile) {}

MockResourcePrefetchPredictor::~MockResourcePrefetchPredictor() = default;

void InitializeRedirectStat(RedirectStat* redirect,
                            const std::string& url,
                            int number_of_hits,
                            int number_of_misses,
                            int consecutive_misses) {
  redirect->set_url(url);
  redirect->set_number_of_hits(number_of_hits);
  redirect->set_number_of_misses(number_of_misses);
  redirect->set_consecutive_misses(consecutive_misses);
}

void InitializeOriginStat(OriginStat* origin_stat,
                          const std::string& origin,
                          int number_of_hits,
                          int number_of_misses,
                          int consecutive_misses,
                          double average_position,
                          bool always_access_network,
                          bool accessed_network) {
  origin_stat->set_origin(origin);
  origin_stat->set_number_of_hits(number_of_hits);
  origin_stat->set_number_of_misses(number_of_misses);
  origin_stat->set_consecutive_misses(consecutive_misses);
  origin_stat->set_average_position(average_position);
  origin_stat->set_always_access_network(always_access_network);
  origin_stat->set_accessed_network(accessed_network);
}

RedirectData CreateRedirectData(const std::string& primary_key,
                                uint64_t last_visit_time) {
  RedirectData data;
  data.set_primary_key(primary_key);
  data.set_last_visit_time(last_visit_time);
  return data;
}

OriginData CreateOriginData(const std::string& host, uint64_t last_visit_time) {
  OriginData data;
  data.set_host(host);
  data.set_last_visit_time(last_visit_time);
  return data;
}

NavigationID CreateNavigationID(SessionID tab_id,
                                const std::string& main_frame_url) {
  NavigationID navigation_id;
  navigation_id.tab_id = tab_id;
  navigation_id.main_frame_url = GURL(main_frame_url);
  navigation_id.creation_time = base::TimeTicks::Now();
  return navigation_id;
}

PageRequestSummary CreatePageRequestSummary(
    const std::string& main_frame_url,
    const std::string& initial_url,
    const std::vector<URLRequestSummary>& subresource_requests) {
  GURL main_frame_gurl(main_frame_url);
  PageRequestSummary summary(main_frame_gurl);
  summary.initial_url = GURL(initial_url);
  summary.UpdateOrAddToOrigins(CreateURLRequestSummary(
      SessionID::FromSerializedValue(1), main_frame_url));
  for (auto& request_summary : subresource_requests)
    summary.UpdateOrAddToOrigins(request_summary);
  return summary;
}

URLRequestSummary CreateURLRequestSummary(SessionID tab_id,
                                          const std::string& main_frame_url,
                                          const std::string& request_url,
                                          content::ResourceType resource_type,
                                          const std::string& redirect_url,
                                          bool always_revalidate) {
  URLRequestSummary summary;
  summary.navigation_id = CreateNavigationID(tab_id, main_frame_url);
  summary.request_url =
      request_url.empty() ? GURL(main_frame_url) : GURL(request_url);
  if (!redirect_url.empty())
    summary.redirect_url = GURL(redirect_url);
  summary.resource_type = resource_type;
  summary.always_revalidate = always_revalidate;
  summary.is_no_store = false;
  summary.network_accessed = true;
  return summary;
}

URLRequestSummary CreateRedirectRequestSummary(
    SessionID session_id,
    const std::string& main_frame_url,
    const std::string& redirect_url) {
  URLRequestSummary summary =
      CreateURLRequestSummary(session_id, main_frame_url);
  summary.redirect_url = GURL(redirect_url);
  return summary;
}

PreconnectPrediction CreatePreconnectPrediction(
    std::string host,
    bool is_redirected,
    const std::vector<PreconnectRequest>& requests) {
  PreconnectPrediction prediction;
  prediction.host = host;
  prediction.is_redirected = is_redirected;
  prediction.requests = requests;
  return prediction;
}

void PopulateTestConfig(LoadingPredictorConfig* config, bool small_db) {
  if (small_db) {
    config->max_hosts_to_track = 2;
    config->max_origins_per_entry = 5;
    config->max_consecutive_misses = 2;
    config->max_redirect_consecutive_misses = 2;
  }
  config->is_origin_learning_enabled = true;
  config->mode = LoadingPredictorConfig::LEARNING;
  config->flush_data_to_disk_delay_seconds = 0;
}

scoped_refptr<net::HttpResponseHeaders> MakeResponseHeaders(
    const char* headers) {
  return base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(headers, strlen(headers)));
}

MockURLRequestJob::MockURLRequestJob(
    net::URLRequest* request,
    const net::HttpResponseInfo& response_info,
    const net::LoadTimingInfo& load_timing_info,
    const std::string& mime_type)
    : net::URLRequestJob(request, nullptr),
      response_info_(response_info),
      load_timing_info_(load_timing_info),
      mime_type_(mime_type) {}

bool MockURLRequestJob::GetMimeType(std::string* mime_type) const {
  *mime_type = mime_type_;
  return true;
}

void MockURLRequestJob::Start() {
  NotifyHeadersComplete();
}

void MockURLRequestJob::GetResponseInfo(net::HttpResponseInfo* info) {
  *info = response_info_;
}

void MockURLRequestJob::GetLoadTimingInfo(net::LoadTimingInfo* info) const {
  *info = load_timing_info_;
}

MockURLRequestJobFactory::MockURLRequestJobFactory() {}
MockURLRequestJobFactory::~MockURLRequestJobFactory() {}

void MockURLRequestJobFactory::Reset() {
  response_info_ = net::HttpResponseInfo();
  mime_type_ = std::string();
}

net::URLRequestJob* MockURLRequestJobFactory::MaybeCreateJobWithProtocolHandler(
    const std::string& scheme,
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return new MockURLRequestJob(request, response_info_, load_timing_info_,
                               mime_type_);
}

net::URLRequestJob* MockURLRequestJobFactory::MaybeInterceptRedirect(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const GURL& location) const {
  return nullptr;
}

net::URLRequestJob* MockURLRequestJobFactory::MaybeInterceptResponse(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return nullptr;
}

bool MockURLRequestJobFactory::IsHandledProtocol(
    const std::string& scheme) const {
  return true;
}

bool MockURLRequestJobFactory::IsSafeRedirectTarget(
    const GURL& location) const {
  return true;
}

std::unique_ptr<net::URLRequest> CreateURLRequest(
    const net::TestURLRequestContext& url_request_context,
    const GURL& url,
    net::RequestPriority priority,
    content::ResourceType resource_type,
    bool is_main_frame) {
  std::unique_ptr<net::URLRequest> request = url_request_context.CreateRequest(
      url, priority, &g_empty_url_request_delegate,
      TRAFFIC_ANNOTATION_FOR_TESTS);
  request->set_site_for_cookies(url);
  content::ResourceRequestInfo::AllocateForTesting(
      request.get(), resource_type, nullptr, -1, -1, -1, is_main_frame, false,
      true, content::PREVIEWS_OFF, nullptr);
  request->Start();
  return request;
}

std::ostream& operator<<(std::ostream& os, const RedirectData& data) {
  os << "[" << data.primary_key() << "," << data.last_visit_time() << "]"
     << std::endl;
  for (const RedirectStat& redirect : data.redirect_endpoints())
    os << "\t\t" << redirect << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const RedirectStat& redirect) {
  return os << "[" << redirect.url() << "," << redirect.number_of_hits() << ","
            << redirect.number_of_misses() << ","
            << redirect.consecutive_misses() << "]";
}

std::ostream& operator<<(std::ostream& os, const OriginData& data) {
  os << "[" << data.host() << "," << data.last_visit_time() << "]" << std::endl;
  for (const OriginStat& origin : data.origins())
    os << "\t\t" << origin << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const OriginStat& origin) {
  return os << "[" << origin.origin() << "," << origin.number_of_hits() << ","
            << origin.number_of_misses() << "," << origin.consecutive_misses()
            << "," << origin.average_position() << ","
            << origin.always_access_network() << ","
            << origin.accessed_network() << "]";
}

std::ostream& operator<<(std::ostream& os,
                         const OriginRequestSummary& summary) {
  return os << "[" << summary.origin << "," << summary.always_access_network
            << "," << summary.accessed_network << ","
            << summary.first_occurrence << "]";
}

std::ostream& operator<<(std::ostream& os, const PageRequestSummary& summary) {
  os << "[" << summary.main_frame_url << "," << summary.initial_url << "]"
     << std::endl;
  for (const auto& pair : summary.origins)
    os << "\t\t" << pair.first << ":" << pair.second << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const URLRequestSummary& summary) {
  return os << "[" << summary.navigation_id << "," << summary.request_url << ","
            << summary.redirect_url << "," << summary.resource_type << ","
            << summary.always_revalidate << "," << summary.is_no_store << ","
            << summary.network_accessed << "]";
}

std::ostream& operator<<(std::ostream& os, const NavigationID& navigation_id) {
  return os << navigation_id.tab_id << "," << navigation_id.main_frame_url;
}

std::ostream& operator<<(std::ostream& os, const PreconnectRequest& request) {
  return os << "[" << request.origin << "," << request.num_sockets << ","
            << request.allow_credentials << "]";
}

std::ostream& operator<<(std::ostream& os,
                         const PreconnectPrediction& prediction) {
  os << "[" << prediction.host << "," << prediction.is_redirected << "]"
     << std::endl;

  for (const auto& request : prediction.requests)
    os << "\t\t" << request << std::endl;

  return os;
}

bool operator==(const RedirectData& lhs, const RedirectData& rhs) {
  bool equal = lhs.primary_key() == rhs.primary_key() &&
               lhs.redirect_endpoints_size() == rhs.redirect_endpoints_size();

  if (!equal)
    return false;

  for (int i = 0; i < lhs.redirect_endpoints_size(); ++i)
    equal = equal && lhs.redirect_endpoints(i) == rhs.redirect_endpoints(i);

  return equal;
}

bool operator==(const RedirectStat& lhs, const RedirectStat& rhs) {
  return lhs.url() == rhs.url() &&
         lhs.number_of_hits() == rhs.number_of_hits() &&
         lhs.number_of_misses() == rhs.number_of_misses() &&
         lhs.consecutive_misses() == rhs.consecutive_misses();
}

bool operator==(const PageRequestSummary& lhs, const PageRequestSummary& rhs) {
  return lhs.main_frame_url == rhs.main_frame_url &&
         lhs.initial_url == rhs.initial_url &&
         lhs.origins == rhs.origins;
}

bool operator==(const URLRequestSummary& lhs, const URLRequestSummary& rhs) {
  return lhs.navigation_id == rhs.navigation_id &&
         lhs.request_url == rhs.request_url &&
         lhs.redirect_url == rhs.redirect_url &&
         lhs.resource_type == rhs.resource_type &&
         lhs.always_revalidate == rhs.always_revalidate &&
         lhs.is_no_store == rhs.is_no_store &&
         lhs.network_accessed == rhs.network_accessed;
}

bool operator==(const OriginRequestSummary& lhs,
                const OriginRequestSummary& rhs) {
  return lhs.origin == rhs.origin &&
         lhs.always_access_network == rhs.always_access_network &&
         lhs.accessed_network == rhs.accessed_network &&
         lhs.first_occurrence == rhs.first_occurrence;
}

bool operator==(const OriginData& lhs, const OriginData& rhs) {
  bool equal =
      lhs.host() == rhs.host() && lhs.origins_size() == rhs.origins_size();
  if (!equal)
    return false;

  for (int i = 0; i < lhs.origins_size(); ++i)
    equal = equal && lhs.origins(i) == rhs.origins(i);

  return equal;
}

bool operator==(const OriginStat& lhs, const OriginStat& rhs) {
  return lhs.origin() == rhs.origin() &&
         lhs.number_of_hits() == rhs.number_of_hits() &&
         lhs.number_of_misses() == rhs.number_of_misses() &&
         lhs.consecutive_misses() == rhs.consecutive_misses() &&
         AlmostEqual(lhs.average_position(), rhs.average_position()) &&
         lhs.always_access_network() == rhs.always_access_network() &&
         lhs.accessed_network() == rhs.accessed_network();
}

bool operator==(const PreconnectRequest& lhs, const PreconnectRequest& rhs) {
  return lhs.origin == rhs.origin && lhs.num_sockets == rhs.num_sockets &&
         lhs.allow_credentials == rhs.allow_credentials;
}

bool operator==(const PreconnectPrediction& lhs,
                const PreconnectPrediction& rhs) {
  return lhs.is_redirected == rhs.is_redirected && lhs.host == rhs.host &&
         lhs.requests == rhs.requests;
}

}  // namespace predictors
