// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/android_affiliation/affiliation_fetcher.h"

#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "components/password_manager/core/browser/android_affiliation/affiliation_api.pb.h"
#include "components/password_manager/core/browser/android_affiliation/affiliation_utils.h"
#include "components/password_manager/core/browser/android_affiliation/test_affiliation_fetcher_factory.h"
#include "google_apis/google_api_keys.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace password_manager {

namespace {

// Enumeration listing the possible outcomes of fetching affiliation information
// from the Affiliation API. This is used in UMA histograms, so do not change
// existing values, only add new values at the end.
enum AffiliationFetchResult {
  AFFILIATION_FETCH_RESULT_SUCCESS,
  AFFILIATION_FETCH_RESULT_FAILURE,
  AFFILIATION_FETCH_RESULT_MALFORMED,
  AFFILIATION_FETCH_RESULT_MAX
};

// Records the given fetch |result| into the respective UMA histogram, as well
// as the response and error codes of |fetcher| if it is non-null.
void ReportStatistics(AffiliationFetchResult result,
                      const net::URLFetcher* fetcher) {
  UMA_HISTOGRAM_ENUMERATION("PasswordManager.AffiliationFetcher.FetchResult",
                            result, AFFILIATION_FETCH_RESULT_MAX);
  if (fetcher) {
    base::UmaHistogramSparse(
        "PasswordManager.AffiliationFetcher.FetchHttpResponseCode",
        fetcher->GetResponseCode());
    // Network error codes are negative. See: src/net/base/net_error_list.h.
    base::UmaHistogramSparse(
        "PasswordManager.AffiliationFetcher.FetchErrorCode",
        -fetcher->GetStatus().error());
  }
}

}  // namespace

static TestAffiliationFetcherFactory* g_testing_factory = nullptr;

AffiliationFetcher::AffiliationFetcher(
    net::URLRequestContextGetter* request_context_getter,
    const std::vector<FacetURI>& facet_uris,
    AffiliationFetcherDelegate* delegate)
    : request_context_getter_(request_context_getter),
      requested_facet_uris_(facet_uris),
      delegate_(delegate) {
  for (const FacetURI& uri : requested_facet_uris_) {
    DCHECK(uri.is_valid());
  }
}

AffiliationFetcher::~AffiliationFetcher() {
}

// static
AffiliationFetcher* AffiliationFetcher::Create(
    net::URLRequestContextGetter* context_getter,
    const std::vector<FacetURI>& facet_uris,
    AffiliationFetcherDelegate* delegate) {
  if (g_testing_factory) {
    return g_testing_factory->CreateInstance(context_getter, facet_uris,
                                             delegate);
  }
  return new AffiliationFetcher(context_getter, facet_uris, delegate);
}

// static
void AffiliationFetcher::SetFactoryForTesting(
    TestAffiliationFetcherFactory* factory) {
  g_testing_factory = factory;
}

void AffiliationFetcher::StartRequest() {
  DCHECK(!fetcher_);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("affiliation_lookup", R"(
        semantics {
          sender: "Android Credentials Affiliation Fetcher"
          description:
            "Users syncing their passwords may have credentials stored for "
            "Android apps. Unless synced data is encrypted with a custom "
            "passphrase, this service downloads the associations between "
            "Android apps and the corresponding websites. Thus, the Android "
            "credentials can be used while browsing the web. "
          trigger: "Periodically in the background."
          data:
            "List of Android apps the user has credentials for. The passwords "
            "and usernames aren't sent."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Users can enable or disable this feature either by stoping "
            "syncing passwords to Google (via unchecking 'Passwords' in "
            "Chromium's settings under 'Sign In', 'Advanced sync settings') or "
            "by introducing a custom passphrase to disable this service. The "
            "feature is enabled by default."
          chrome_policy {
            SyncDisabled {
              policy_options {mode: MANDATORY}
              SyncDisabled: true
            }
          }
        })");
  fetcher_ = net::URLFetcher::Create(BuildQueryURL(), net::URLFetcher::POST,
                                     this, traffic_annotation);
  fetcher_->SetRequestContext(request_context_getter_.get());
  fetcher_->SetUploadData("application/x-protobuf", PreparePayload());
  fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                         net::LOAD_DO_NOT_SEND_COOKIES |
                         net::LOAD_DO_NOT_SEND_AUTH_DATA |
                         net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE);
  fetcher_->SetAutomaticallyRetryOn5xx(false);
  fetcher_->SetAutomaticallyRetryOnNetworkChanges(0);
  fetcher_->Start();
}

GURL AffiliationFetcher::BuildQueryURL() const {
  return net::AppendQueryParameter(
      GURL("https://www.googleapis.com/affiliation/v1/affiliation:lookup"),
      "key", google_apis::GetAPIKey());
}

std::string AffiliationFetcher::PreparePayload() const {
  affiliation_pb::LookupAffiliationRequest lookup_request;
  for (const FacetURI& uri : requested_facet_uris_)
    lookup_request.add_facet(uri.canonical_spec());

  // Enable request for branding information.
  auto mask = std::make_unique<affiliation_pb::LookupAffiliationMask>();
  mask->set_branding_info(true);
  lookup_request.set_allocated_mask(mask.release());

  std::string serialized_request;
  bool success = lookup_request.SerializeToString(&serialized_request);
  DCHECK(success);
  return serialized_request;
}

bool AffiliationFetcher::ParseResponse(
    AffiliationFetcherDelegate::Result* result) const {
  // This function parses the response protocol buffer message for a list of
  // equivalence classes, and stores them into |results| after performing some
  // validation and sanitization steps to make sure that the contract of
  // AffiliationFetcherDelegate is fulfilled. Possible discrepancies are:
  //   * The server response will not have anything for facets that are not
  //     affiliated with any other facet, while |result| must have them.
  //   * The server response might contain future, unknown kinds of facet URIs,
  //     while |result| must contain only those that are FacetURI::is_valid().
  //   * The server response being ill-formed or self-inconsistent (in the sense
  //     that there are overlapping equivalence classes) is indicative of server
  //     side issues likely not remedied by re-fetching. Report failure in this
  //     case so the caller can be notified and it can act accordingly.
  //   * The |result| will be free of duplicate or empty equivalence classes.

  std::string serialized_response;
  if (!fetcher_->GetResponseAsString(&serialized_response)) {
    NOTREACHED();
  }

  affiliation_pb::LookupAffiliationResponse response;
  if (!response.ParseFromString(serialized_response))
    return false;

  result->reserve(requested_facet_uris_.size());

  std::map<FacetURI, size_t> facet_uri_to_class_index;
  for (int i = 0; i < response.affiliation_size(); ++i) {
    const affiliation_pb::Affiliation& equivalence_class(
        response.affiliation(i));

    AffiliatedFacets affiliated_facets;
    for (int j = 0; j < equivalence_class.facet_size(); ++j) {
      const affiliation_pb::Facet& facet(equivalence_class.facet(j));
      const std::string& uri_spec(facet.id());
      FacetURI uri = FacetURI::FromPotentiallyInvalidSpec(uri_spec);
      // Ignore potential future kinds of facet URIs (e.g. for new platforms).
      if (!uri.is_valid())
        continue;
      affiliated_facets.push_back(
          {uri, FacetBrandingInfo{facet.branding_info().name(),
                                  GURL(facet.branding_info().icon_url())}});
    }

    // Be lenient and ignore empty (after filtering) equivalence classes.
    if (affiliated_facets.empty())
      continue;

    // Ignore equivalence classes that are duplicates of earlier ones. However,
    // fail in the case of a partial overlap, which violates the invariant that
    // affiliations must form an equivalence relation.
    for (const Facet& facet : affiliated_facets) {
      if (!facet_uri_to_class_index.count(facet.uri))
        facet_uri_to_class_index[facet.uri] = result->size();
      if (facet_uri_to_class_index[facet.uri] !=
          facet_uri_to_class_index[affiliated_facets[0].uri]) {
        return false;
      }
    }

    // Filter out duplicate equivalence classes in the response.
    if (facet_uri_to_class_index[affiliated_facets[0].uri] == result->size())
      result->push_back(affiliated_facets);
  }

  // Synthesize an equivalence class (of size one) for each facet that did not
  // appear in the server response due to not being affiliated with any others.
  for (const FacetURI& uri : requested_facet_uris_) {
    if (!facet_uri_to_class_index.count(uri))
      result->push_back({{uri}});
  }

  return true;
}

void AffiliationFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(source, fetcher_.get());

  // Note that invoking the |delegate_| may destroy |this| synchronously, so the
  // invocation must happen last.
  std::unique_ptr<AffiliationFetcherDelegate::Result> result_data(
      new AffiliationFetcherDelegate::Result);
  if (fetcher_->GetStatus().status() == net::URLRequestStatus::SUCCESS &&
      fetcher_->GetResponseCode() == net::HTTP_OK) {
    if (ParseResponse(result_data.get())) {
      ReportStatistics(AFFILIATION_FETCH_RESULT_SUCCESS, nullptr);
      delegate_->OnFetchSucceeded(std::move(result_data));
    } else {
      ReportStatistics(AFFILIATION_FETCH_RESULT_MALFORMED, nullptr);
      delegate_->OnMalformedResponse();
    }
  } else {
    ReportStatistics(AFFILIATION_FETCH_RESULT_FAILURE, fetcher_.get());
    delegate_->OnFetchFailed();
  }
}

}  // namespace password_manager
