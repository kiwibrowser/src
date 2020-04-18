// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestions_fetcher_impl.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind_helpers.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind_test_util.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_task_environment.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_result.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_test_utils.h"
#include "components/ntp_snippets/contextual/proto/chrome_search_api_request_context.pb.h"
#include "components/ntp_snippets/contextual/proto/get_pivots_request.pb.h"
#include "components/ntp_snippets/contextual/proto/get_pivots_response.pb.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace contextual_suggestions {

using contextual_suggestions::AutoPeekConditions;
using contextual_suggestions::ClusterBuilder;
using contextual_suggestions::ContextualSuggestionsEvent;
using contextual_suggestions::ExploreContext;
using contextual_suggestions::GetPivotsQuery;
using contextual_suggestions::GetPivotsRequest;
using contextual_suggestions::GetPivotsResponse;
using contextual_suggestions::ImageId;
using contextual_suggestions::MockClustersCallback;
using contextual_suggestions::MockMetricsCallback;
using contextual_suggestions::PeekConditions;
using contextual_suggestions::PivotCluster;
using contextual_suggestions::PivotClusteringParams;
using contextual_suggestions::PivotDocument;
using contextual_suggestions::PivotDocumentParams;
using contextual_suggestions::PivotItem;
using contextual_suggestions::Pivots;
using network::SharedURLLoaderFactory;
using network::TestURLLoaderFactory;
using testing::ElementsAre;

namespace {

// TODO(pnoland): de-dupe this and the identical class in
// feed_networking_host_unittest.cc
class TestSharedURLLoaderFactory : public SharedURLLoaderFactory {
 public:
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& url_request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    test_factory_.CreateLoaderAndStart(std::move(request), routing_id,
                                       request_id, options, url_request,
                                       std::move(client), traffic_annotation);
  }

  void Clone(network::mojom::URLLoaderFactoryRequest request) override {
    NOTREACHED();
  }

  std::unique_ptr<network::SharedURLLoaderFactoryInfo> Clone() override {
    NOTREACHED();
    return nullptr;
  }

  TestURLLoaderFactory* test_factory() { return &test_factory_; }

 protected:
  ~TestSharedURLLoaderFactory() override = default;

 private:
  TestURLLoaderFactory test_factory_;
};

Cluster DefaultCluster() {
  return ClusterBuilder("Articles")
      .AddSuggestion(SuggestionBuilder(GURL("http://www.foobar.com"))
                         .Title("Title")
                         .PublisherName("cnn.com")
                         .Snippet("Summary")
                         .ImageId("abcdef")
                         .Build())
      .Build();
}

// Returns a single cluster with a single suggestion with preset values.
std::vector<Cluster> DefaultClusters() {
  std::vector<Cluster> clusters;

  clusters.emplace_back(DefaultCluster());
  return clusters;
}

void PopulateDocument(PivotDocument* document,
                      const ContextualSuggestion& suggestion) {
  document->mutable_url()->set_raw_url(suggestion.url.spec());
  document->set_title(suggestion.title);
  document->set_summary(suggestion.snippet);
  document->set_site_name(suggestion.publisher_name);

  ImageId* image_id = document->mutable_image()->mutable_id();
  image_id->set_encrypted_docid(suggestion.image_id);
}

void PopulatePeekConditions(AutoPeekConditions* proto_conditions,
                            const PeekConditions& peek_conditions) {
  proto_conditions->set_confidence(peek_conditions.confidence);
  proto_conditions->set_page_scroll_percentage(
      peek_conditions.page_scroll_percentage);
  proto_conditions->set_minimum_seconds_on_page(
      peek_conditions.minimum_seconds_on_page);
  proto_conditions->set_maximum_number_of_peeks(
      peek_conditions.maximum_number_of_peeks);
}

// Populates a GetPivotsResponse proto using |peek_text| and |clusters| and
// returns that proto as a serialized string.
std::string SerializedResponseProto(
    const std::string& peek_text,
    std::vector<Cluster> clusters,
    PeekConditions peek_conditions = PeekConditions()) {
  GetPivotsResponse response_proto;
  Pivots* pivots = response_proto.mutable_pivots();
  PopulatePeekConditions(pivots->mutable_auto_peek_conditions(),
                         peek_conditions);
  pivots->mutable_peek_text()->set_text(peek_text);

  for (const auto& cluster : clusters) {
    PivotItem* root_item = pivots->add_item();
    PivotCluster* pivot_cluster = root_item->mutable_cluster();
    pivot_cluster->mutable_label()->set_label(cluster.title);
    for (const ContextualSuggestion& suggestion : cluster.suggestions) {
      PopulateDocument(pivot_cluster->add_item()->mutable_document(),
                       suggestion);
    }
  }

  // The fetch parsing logic expects the response to come as (length, bytes)
  // where length is varint32 encoded, but ignores the actual length read.
  // " " is a valid varint32(32) so this works for now.
  // TODO(pnoland): Use a CodedOutputStream to prepend the actual size so that
  // we're not relying on implementation details.
  return " " + response_proto.SerializeAsString();
}

// Populates a GetPivotsResponse proto with a single, flat list of suggestions
// from |single_cluster| and returns that proto as a serialized string.
std::string SerializedResponseProto(const std::string& peek_text,
                                    Cluster single_cluster) {
  GetPivotsResponse response_proto;
  Pivots* pivots = response_proto.mutable_pivots();
  pivots->mutable_peek_text()->set_text(peek_text);

  for (const ContextualSuggestion& suggestion : single_cluster.suggestions) {
    PopulateDocument(pivots->add_item()->mutable_document(), suggestion);
  }

  // See explanation above for why we prepend " ".
  return " " + response_proto.SerializeAsString();
}

}  // namespace

class ContextualSuggestionsFetcherTest : public testing::Test {
 public:
  ContextualSuggestionsFetcherTest() {
    loader_factory_ = base::MakeRefCounted<TestSharedURLLoaderFactory>();
    fetcher_ = std::make_unique<ContextualSuggestionsFetcherImpl>(
        loader_factory_, "en");
  }

  ~ContextualSuggestionsFetcherTest() override {}

  void SetFakeResponse(const std::string& response_data,
                       net::HttpStatusCode response_code = net::HTTP_OK,
                       network::URLLoaderCompletionStatus status =
                           network::URLLoaderCompletionStatus()) {
    GURL fetch_url(ContextualSuggestionsFetch::GetFetchEndpoint());
    network::ResourceResponseHead head;
    if (response_code >= 0) {
      head.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
          "HTTP/1.1 " + base::NumberToString(response_code));
      status.decoded_body_length = response_data.length();
    }

    loader_factory_->test_factory()->AddResponse(fetch_url, head, response_data,
                                                 status);
  }

  void SendAndAwaitResponse(
      const GURL& context_url,
      MockClustersCallback* callback,
      MockMetricsCallback* mock_metrics_callback = nullptr) {
    ReportFetchMetricsCallback metrics_callback =
        mock_metrics_callback
            ? base::BindRepeating(&MockMetricsCallback::Report,
                                  base::Unretained(mock_metrics_callback))
            : base::DoNothing();
    fetcher().FetchContextualSuggestionsClusters(
        context_url, callback->ToOnceCallback(), metrics_callback);
    base::RunLoop().RunUntilIdle();
  }

  ContextualSuggestionsFetcher& fetcher() { return *fetcher_; }

  TestURLLoaderFactory* test_factory() {
    return loader_factory_->test_factory();
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  scoped_refptr<TestSharedURLLoaderFactory> loader_factory_;
  std::unique_ptr<ContextualSuggestionsFetcherImpl> fetcher_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsFetcherTest);
};

TEST_F(ContextualSuggestionsFetcherTest, SingleSuggestionResponse) {
  MockClustersCallback callback;
  MockMetricsCallback metrics_callback;
  SetFakeResponse(SerializedResponseProto("Peek Text", DefaultClusters()));

  SendAndAwaitResponse(GURL("http://www.article.com"), &callback,
                       &metrics_callback);

  EXPECT_TRUE(callback.has_run);
  ExpectResponsesMatch(std::move(callback),
                       ContextualSuggestionsResult(
                           "Peek Text", DefaultClusters(), PeekConditions()));
  EXPECT_EQ(metrics_callback.events,
            std::vector<ContextualSuggestionsEvent>(
                {contextual_suggestions::FETCH_COMPLETED}));
}

TEST_F(ContextualSuggestionsFetcherTest,
       MultipleSuggestionMultipleClusterResponse) {
  std::vector<Cluster> clusters;
  std::vector<Cluster> clusters_copy;
  base::HistogramTester histogram_tester;

  ClusterBuilder cluster1_builder("Category 1");
  cluster1_builder
      .AddSuggestion(SuggestionBuilder(GURL("http://www.test.com"))
                         .Title("Title1")
                         .PublisherName("test.com")
                         .Snippet("Summary 1")
                         .ImageId("abc")
                         .Build())
      .AddSuggestion(SuggestionBuilder(GURL("http://www.foobar.com"))
                         .Title("Title2")
                         .PublisherName("foobar.com")
                         .Snippet("Summary 2")
                         .ImageId("def")
                         .Build());

  ClusterBuilder cluster2_builder("Category 2");
  cluster2_builder
      .AddSuggestion(SuggestionBuilder(GURL("http://www.barbaz.com"))
                         .Title("Title3")
                         .PublisherName("barbaz.com")
                         .Snippet("Summary 3")
                         .ImageId("ghi")
                         .Build())
      .AddSuggestion(SuggestionBuilder(GURL("http://www.cnn.com"))
                         .Title("Title4")
                         .PublisherName("cnn.com")
                         .Snippet("Summary 4")
                         .ImageId("jkl")
                         .Build())
      .AddSuggestion(SuggestionBuilder(GURL("http://www.slate.com"))
                         .Title("Title5")
                         .PublisherName("slate.com")
                         .Snippet("Summary 5")
                         .ImageId("mno")
                         .Build());
  ClusterBuilder c1_copy = cluster1_builder;
  ClusterBuilder c2_copy = cluster2_builder;

  clusters.emplace_back(cluster1_builder.Build());
  clusters.emplace_back(cluster2_builder.Build());

  clusters_copy.emplace_back(c1_copy.Build());
  clusters_copy.emplace_back(c2_copy.Build());

  SetFakeResponse(SerializedResponseProto("Peek Text", std::move(clusters)));
  MockClustersCallback callback;
  SendAndAwaitResponse(GURL("http://www.article.com/"), &callback);

  EXPECT_TRUE(callback.has_run);
  ExpectResponsesMatch(
      std::move(callback),
      ContextualSuggestionsResult("Peek Text", std::move(clusters_copy),
                                  PeekConditions()));

  histogram_tester.ExpectTotalCount("ContextualSuggestions.FetchResponseSizeKB",
                                    1);
}

TEST_F(ContextualSuggestionsFetcherTest, FlatResponse) {
  SetFakeResponse(SerializedResponseProto("Peek Text", DefaultCluster()));
  MockClustersCallback callback;
  SendAndAwaitResponse(GURL("http://www.article.com/"), &callback);

  EXPECT_TRUE(callback.has_run);
  // There's no title for the flat/unclustered response case, since there's no
  // PivotCluster to copy it from. So we clear the expected title.
  std::vector<Cluster> expected_clusters = DefaultClusters();
  expected_clusters[0].title = "";
  ExpectResponsesMatch(
      std::move(callback),
      ContextualSuggestionsResult("Peek Text", std::move(expected_clusters),
                                  PeekConditions()));
}

TEST_F(ContextualSuggestionsFetcherTest, PeekConditionsAreParsed) {
  MockClustersCallback callback;
  MockMetricsCallback metrics_callback;
  PeekConditions peek_conditions;
  peek_conditions.confidence = 0.7;
  peek_conditions.page_scroll_percentage = 35.0;
  peek_conditions.minimum_seconds_on_page = 4.5;
  peek_conditions.maximum_number_of_peeks = 5.0;

  SetFakeResponse(
      SerializedResponseProto("Peek Text", DefaultClusters(), peek_conditions));

  SendAndAwaitResponse(GURL("http://www.article.com"), &callback,
                       &metrics_callback);

  EXPECT_TRUE(callback.has_run);
  ExpectResponsesMatch(std::move(callback),
                       ContextualSuggestionsResult(
                           "Peek Text", DefaultClusters(), peek_conditions));
}

TEST_F(ContextualSuggestionsFetcherTest, HtmlEntitiesAreUnescaped) {
  ClusterBuilder cluster_builder("Category 1");
  cluster_builder.AddSuggestion(SuggestionBuilder(GURL("http://www.test.com"))
                                    .Title("&quot;foobar&quot;")
                                    .PublisherName("test.com")
                                    .Snippet("&#39;barbaz&#39;")
                                    .ImageId("abc")
                                    .Build());
  ClusterBuilder builder_copy = cluster_builder;
  SetFakeResponse(
      SerializedResponseProto("Peek Text", cluster_builder.Build()));
  MockClustersCallback callback;
  SendAndAwaitResponse(GURL("http://www.article.com/"), &callback);

  EXPECT_TRUE(callback.has_run);
  std::vector<Cluster> expected_clusters;
  expected_clusters.emplace_back(builder_copy.Build());
  // Clear the title since it's a flat response.
  expected_clusters[0].title = "";
  // Adjust the expected title and snippet to manually unescape the html
  // entities we added.
  expected_clusters[0].suggestions[0].title = "\"foobar\"";
  expected_clusters[0].suggestions[0].snippet = "\'barbaz\'";
  ExpectResponsesMatch(
      std::move(callback),
      ContextualSuggestionsResult("Peek Text", std::move(expected_clusters),
                                  PeekConditions()));
}

TEST_F(ContextualSuggestionsFetcherTest, RequestHeaderSetCorrectly) {
  net::HttpRequestHeaders headers;
  base::RunLoop interceptor_run_loop;
  base::HistogramTester histogram_tester;

  test_factory()->SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        headers = request.headers;
        interceptor_run_loop.Quit();
      }));
  SetFakeResponse(SerializedResponseProto("Peek Text", DefaultClusters()));

  MockClustersCallback callback;
  SendAndAwaitResponse(GURL("http://www.article.com/"), &callback);

  interceptor_run_loop.Run();

  std::string protobuf_header;
  ASSERT_TRUE(
      headers.GetHeader("X-Protobuffer-Request-Payload", &protobuf_header));

  std::string decoded_header_value;
  base::Base64Decode(protobuf_header, &decoded_header_value);
  GetPivotsRequest request;
  ASSERT_TRUE(request.ParseFromString(decoded_header_value));

  EXPECT_EQ(request.context().localization_context().language_code(), "en");
  EXPECT_EQ(request.query().context()[0].url(), "http://www.article.com/");
  EXPECT_TRUE(request.query().document_params().enabled());
  EXPECT_EQ(request.query().document_params().num(), 10);
  EXPECT_TRUE(request.query().document_params().enable_images());
  EXPECT_TRUE(request.query().clustering_params().enabled());
  EXPECT_TRUE(request.query().peek_text_params().enabled());

  histogram_tester.ExpectTotalCount(
      "ContextualSuggestions.FetchRequestProtoSizeKB", 1);
}

TEST_F(ContextualSuggestionsFetcherTest, ProtocolError) {
  MockClustersCallback callback;
  MockMetricsCallback metrics_callback;
  base::HistogramTester histogram_tester;

  SetFakeResponse("", net::HTTP_NOT_FOUND);
  SendAndAwaitResponse(GURL("http://www.article.com"), &callback,
                       &metrics_callback);

  EXPECT_TRUE(callback.has_run);
  EXPECT_EQ(callback.response_clusters.size(), 0u);
  EXPECT_THAT(
      histogram_tester.GetAllSamples("ContextualSuggestions.FetchResponseCode"),
      ElementsAre(base::Bucket(/*min=*/net::HTTP_NOT_FOUND, /*count=*/1)));
  EXPECT_EQ(metrics_callback.events,
            std::vector<ContextualSuggestionsEvent>(
                {contextual_suggestions::FETCH_ERROR}));
}

TEST_F(ContextualSuggestionsFetcherTest, ServerUnavailable) {
  MockClustersCallback callback;
  MockMetricsCallback metrics_callback;
  base::HistogramTester histogram_tester;

  SetFakeResponse("", net::HTTP_SERVICE_UNAVAILABLE);
  SendAndAwaitResponse(GURL("http://www.article.com"), &callback,
                       &metrics_callback);

  EXPECT_TRUE(callback.has_run);
  EXPECT_EQ(callback.response_clusters.size(), 0u);
  EXPECT_THAT(
      histogram_tester.GetAllSamples("ContextualSuggestions.FetchResponseCode"),
      ElementsAre(base::Bucket(/*min=*/net::HTTP_SERVICE_UNAVAILABLE,
                               /*count=*/1)));
  EXPECT_EQ(metrics_callback.events,
            std::vector<ContextualSuggestionsEvent>(
                {contextual_suggestions::FETCH_SERVER_BUSY}));
}

TEST_F(ContextualSuggestionsFetcherTest, NetworkError) {
  MockClustersCallback callback;
  MockMetricsCallback metrics_callback;
  base::HistogramTester histogram_tester;

  SetFakeResponse(
      "", net::HTTP_OK,
      network::URLLoaderCompletionStatus(net::ERR_CERT_COMMON_NAME_INVALID));
  SendAndAwaitResponse(GURL("http://www.article.com"), &callback,
                       &metrics_callback);

  EXPECT_TRUE(callback.has_run);
  EXPECT_EQ(callback.response_clusters.size(), 0u);
  EXPECT_THAT(
      histogram_tester.GetAllSamples("ContextualSuggestions.FetchErrorCode"),
      ElementsAre(base::Bucket(
          /*min=*/net::ERR_CERT_COMMON_NAME_INVALID, /*count=*/1)));

  EXPECT_EQ(metrics_callback.events,
            std::vector<ContextualSuggestionsEvent>(
                {contextual_suggestions::FETCH_ERROR}));
}

TEST_F(ContextualSuggestionsFetcherTest, EmptyResponse) {
  MockClustersCallback callback;
  MockMetricsCallback metrics_callback;
  SetFakeResponse(SerializedResponseProto("", Cluster()));
  SendAndAwaitResponse(GURL("http://www.article.com/"), &callback,
                       &metrics_callback);

  EXPECT_TRUE(callback.has_run);
  EXPECT_EQ(callback.response_clusters.size(), 0u);

  EXPECT_EQ(metrics_callback.events,
            std::vector<ContextualSuggestionsEvent>(
                {contextual_suggestions::FETCH_EMPTY}));
}

TEST_F(ContextualSuggestionsFetcherTest, ResponseWithUnsetFields) {
  GetPivotsResponse response;
  Pivots* pivots = response.mutable_pivots();
  pivots->add_item()->mutable_document();
  pivots->add_item();

  SetFakeResponse(" " + response.SerializeAsString());
  MockClustersCallback callback;
  SendAndAwaitResponse(GURL("http://www.article.com/"), &callback);

  std::vector<Cluster> expected_clusters;
  expected_clusters.emplace_back(
      ClusterBuilder("")
          .AddSuggestion(SuggestionBuilder(GURL()).Build())
          .AddSuggestion(SuggestionBuilder(GURL()).Build())
          .Build());

  EXPECT_TRUE(callback.has_run);
  EXPECT_EQ(callback.response_clusters.size(), 1u);
  ExpectResponsesMatch(std::move(callback),
                       ContextualSuggestionsResult(
                           "", std::move(expected_clusters), PeekConditions()));
}

TEST_F(ContextualSuggestionsFetcherTest, CorruptResponse) {
  SetFakeResponse("unparseable proto string");
  MockClustersCallback callback;
  SendAndAwaitResponse(GURL("http://www.article.com/"), &callback);

  EXPECT_TRUE(callback.has_run);
  EXPECT_EQ(callback.response_clusters.size(), 0u);
}

}  // namespace contextual_suggestions
