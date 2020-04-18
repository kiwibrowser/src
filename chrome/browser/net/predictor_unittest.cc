// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/predictor.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>

#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/net/url_info.h"
#include "chrome/common/chrome_features.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/transport_security_state.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::Time;
using base::TimeDelta;
namespace chrome_browser_net {

class PredictorTest : public testing::Test {
 public:
  PredictorTest() = default;

 private:
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
};

//------------------------------------------------------------------------------
// Functions to help synthesize and test serializations of subresource referrer
// lists.

// Return a motivation_list if we can find one for the given motivating_host (or
// NULL if a match is not found).
static const base::ListValue* FindSerializationMotivation(
    const GURL& motivation,
    const base::ListValue* referral_list) {
  CHECK_LT(0u, referral_list->GetSize());  // Room for version.
  int format_version = -1;
  CHECK(referral_list->GetInteger(0, &format_version));
  CHECK_EQ(Predictor::kPredictorReferrerVersion, format_version);
  const base::ListValue* motivation_list(NULL);
  for (size_t i = 1; i < referral_list->GetSize(); ++i) {
    referral_list->GetList(i, &motivation_list);
    std::string existing_spec;
    EXPECT_TRUE(motivation_list->GetString(0, &existing_spec));
    if (motivation == GURL(existing_spec))
      return motivation_list;
  }
  return NULL;
}

static base::ListValue* FindSerializationMotivation(
    const GURL& motivation,
    base::ListValue* referral_list) {
  return const_cast<base::ListValue*>(FindSerializationMotivation(
      motivation, static_cast<const base::ListValue*>(referral_list)));
}

// Create a new empty serialization list.
static base::ListValue* NewEmptySerializationList() {
  base::ListValue* list = new base::ListValue;
  list->AppendInteger(Predictor::kPredictorReferrerVersion);
  return list;
}

// Add a motivating_url and a subresource_url to a serialized list, using
// this given latency. This is a helper function for quickly building these
// lists.
static void AddToSerializedList(const GURL& motivation,
                                const GURL& subresource,
                                double use_rate,
                                base::ListValue* referral_list) {
  // Find the motivation if it is already used.
  base::ListValue* motivation_list =
      FindSerializationMotivation(motivation, referral_list);
  if (!motivation_list) {
    // This is the first mention of this motivation, so build a list.
    motivation_list = new base::ListValue;
    motivation_list->AppendString(motivation.spec());
    // Provide empty subresource list.
    motivation_list->Append(std::make_unique<base::ListValue>());

    // ...and make it part of the serialized referral_list.
    referral_list->Append(base::WrapUnique(motivation_list));
    // |motivation_list| is invalidated at this point, so it needs to be reset.
    referral_list->GetList(referral_list->GetSize() - 1, &motivation_list);
  }

  base::ListValue* subresource_list(NULL);
  // 0 == url; 1 == subresource_list.
  EXPECT_TRUE(motivation_list->GetList(1, &subresource_list));

  // We won't bother to check for the subresource being there already.  Worst
  // case, during deserialization, the latency value we supply plus the
  // existing value(s) will be added to the referrer.

  subresource_list->AppendString(subresource.spec());
  subresource_list->AppendDouble(use_rate);
}

// For a given motivation, and subresource, find what latency is currently
// listed.  This assume a well formed serialization, which has at most one such
// entry for any pair of names.  If no such pair is found, then return false.
// Data is written into use_rate arguments.
static bool GetDataFromSerialization(const GURL& motivation,
                                     const GURL& subresource,
                                     const base::ListValue& referral_list,
                                     double* use_rate) {
  const base::ListValue* motivation_list =
      FindSerializationMotivation(motivation, &referral_list);
  if (!motivation_list)
    return false;
  const base::ListValue* subresource_list;
  EXPECT_TRUE(motivation_list->GetList(1, &subresource_list));
  for (size_t i = 0; i < subresource_list->GetSize();) {
    std::string url_spec;
    EXPECT_TRUE(subresource_list->GetString(i++, &url_spec));
    EXPECT_TRUE(subresource_list->GetDouble(i++, use_rate));
    if (subresource == GURL(url_spec)) {
      return true;
    }
  }
  return false;
}

TEST_F(PredictorTest, StartupShutdownTest) {
  Predictor testing_master(true);
  testing_master.Shutdown();
}

// Make sure nil referral lists really have no entries, and no latency listed.
TEST_F(PredictorTest, ReferrerSerializationNilTest) {
  Predictor predictor(true);

  std::unique_ptr<base::ListValue> referral_list(new base::ListValue);
  predictor.SerializeReferrers(referral_list.get());
  EXPECT_EQ(1U, referral_list->GetSize());
  EXPECT_FALSE(GetDataFromSerialization(
    GURL("http://a.com:79"), GURL("http://b.com:78"),
      *referral_list.get(), NULL));

  predictor.Shutdown();
}

// Make sure that when a serialization list includes a value, that it can be
// deserialized into the database, and can be extracted back out via
// serialization without being changed.
TEST_F(PredictorTest, ReferrerSerializationSingleReferrerTest) {
  Predictor predictor(true);
  const GURL motivation_url("http://www.google.com:91");
  const GURL subresource_url("http://icons.google.com:90");
  const double kUseRate = 23.4;
  std::unique_ptr<base::ListValue> referral_list(NewEmptySerializationList());

  AddToSerializedList(motivation_url, subresource_url,
      kUseRate, referral_list.get());

  predictor.DeserializeReferrers(*referral_list.get());

  base::ListValue recovered_referral_list;
  predictor.SerializeReferrers(&recovered_referral_list);
  EXPECT_EQ(2U, recovered_referral_list.GetSize());
  double rate;
  EXPECT_TRUE(GetDataFromSerialization(
      motivation_url, subresource_url, recovered_referral_list, &rate));
  EXPECT_EQ(rate, kUseRate);

  predictor.Shutdown();
}

// Test that the referrers are sorted in MRU order in the HTML UI.
TEST_F(PredictorTest, GetHtmlReferrerLists) {
  SimplePredictor predictor(true);

  predictor.LearnFromNavigation(GURL("http://www.source_b.test"),
                                GURL("http://www.target_b.test"));
  predictor.LearnFromNavigation(GURL("http://www.source_a.test"),
                                GURL("http://www.target_a.test"));
  predictor.LearnFromNavigation(GURL("http://www.source_c.test"),
                                GURL("http://www.target_c.test"));

  std::string html;
  predictor.GetHtmlReferrerLists(&html);

  size_t pos[] = {
      html.find("<td rowspan=1>http://www.source_c.test"),
      html.find("<td rowspan=1>http://www.source_a.test"),
      html.find("<td rowspan=1>http://www.source_b.test"),
  };

  // Make sure things appeared in the expected order.
  for (size_t i = 1; i < arraysize(pos); ++i) {
    EXPECT_LT(pos[i - 1], pos[i]) << "Mismatch for pos[" << i << "]";
  }

  predictor.Shutdown();
}

// Expect the exact same HTML when the predictor's referrers are serialized and
// deserialized (implies ordering remains the same).
TEST_F(PredictorTest, SerializeAndDeserialize) {
  SimplePredictor predictor(true);

  for (int i = 0; i < Predictor::kMaxReferrers * 2; ++i) {
    predictor.LearnFromNavigation(
        GURL(base::StringPrintf("http://www.source_%d.test", i)),
        GURL(base::StringPrintf("http://www.target_%d.test", i)));
  }
  std::string html;
  predictor.GetHtmlReferrerLists(&html);

  base::ListValue referral_list;
  predictor.SerializeReferrers(&referral_list);
  predictor.DeserializeReferrers(referral_list);

  std::string html2;
  predictor.GetHtmlReferrerLists(&html2);

  EXPECT_EQ(html, html2);

  predictor.Shutdown();
}

// Filling the MRU cache should evict entries that were used less recently.
TEST_F(PredictorTest, FillMRUCache) {
  SimplePredictor predictor(true);

  for (int i = 0; i < Predictor::kMaxReferrers * 2; ++i) {
    predictor.LearnFromNavigation(
        GURL(base::StringPrintf("http://www.source_%d.test", i)),
        GURL(base::StringPrintf("http://www.target_%d.test", i)));
  }

  std::string html;
  predictor.GetHtmlReferrerLists(&html);

  for (int i = 0; i < Predictor::kMaxReferrers; ++i) {
    EXPECT_EQ(html.find(base::StringPrintf("http://www.source_%d.test", i)),
              std::string::npos);
  }
  for (int i = Predictor::kMaxReferrers; i < Predictor::kMaxReferrers * 2;
       ++i) {
    EXPECT_NE(html.find(base::StringPrintf("http://www.source_%d.test", i)),
              std::string::npos);
  }

  predictor.Shutdown();
}

TEST_F(PredictorTest, PriorityQueuePushPopTest) {
  Predictor::HostNameQueue queue;

  GURL first("http://first:80"), second("http://second:90");

  // First check high priority queue FIFO functionality.
  EXPECT_TRUE(queue.IsEmpty());
  queue.Push(first, UrlInfo::LEARNED_REFERAL_MOTIVATED);
  EXPECT_FALSE(queue.IsEmpty());
  queue.Push(second, UrlInfo::MOUSE_OVER_MOTIVATED);
  EXPECT_FALSE(queue.IsEmpty());
  EXPECT_EQ(queue.Pop(), first);
  EXPECT_FALSE(queue.IsEmpty());
  EXPECT_EQ(queue.Pop(), second);
  EXPECT_TRUE(queue.IsEmpty());

  // Then check low priority queue FIFO functionality.
  queue.Push(first, UrlInfo::PAGE_SCAN_MOTIVATED);
  EXPECT_FALSE(queue.IsEmpty());
  queue.Push(second, UrlInfo::OMNIBOX_MOTIVATED);
  EXPECT_FALSE(queue.IsEmpty());
  EXPECT_EQ(queue.Pop(), first);
  EXPECT_FALSE(queue.IsEmpty());
  EXPECT_EQ(queue.Pop(), second);
  EXPECT_TRUE(queue.IsEmpty());
}

TEST_F(PredictorTest, PriorityQueueReorderTest) {
  Predictor::HostNameQueue queue;

  // Push all the low priority items.
  GURL low1("http://low1:80"),
      low2("http://low2:80"),
      low3("http://low3:443"),
      low4("http://low4:80"),
      low5("http://low5:80"),
      hi1("http://hi1:80"),
      hi2("http://hi2:80"),
      hi3("http://hi3:80");

  EXPECT_TRUE(queue.IsEmpty());
  queue.Push(low1, UrlInfo::PAGE_SCAN_MOTIVATED);
  queue.Push(low2, UrlInfo::UNIT_TEST_MOTIVATED);
  queue.Push(low3, UrlInfo::LINKED_MAX_MOTIVATED);
  queue.Push(low4, UrlInfo::OMNIBOX_MOTIVATED);
  queue.Push(low5, UrlInfo::STARTUP_LIST_MOTIVATED);
  queue.Push(low4, UrlInfo::OMNIBOX_MOTIVATED);

  // Push all the high prority items
  queue.Push(hi1, UrlInfo::LEARNED_REFERAL_MOTIVATED);
  queue.Push(hi2, UrlInfo::STATIC_REFERAL_MOTIVATED);
  queue.Push(hi3, UrlInfo::MOUSE_OVER_MOTIVATED);

  // Check that high priority stuff comes out first, and in FIFO order.
  EXPECT_EQ(queue.Pop(), hi1);
  EXPECT_EQ(queue.Pop(), hi2);
  EXPECT_EQ(queue.Pop(), hi3);

  // ...and then low priority strings.
  EXPECT_EQ(queue.Pop(), low1);
  EXPECT_EQ(queue.Pop(), low2);
  EXPECT_EQ(queue.Pop(), low3);
  EXPECT_EQ(queue.Pop(), low4);
  EXPECT_EQ(queue.Pop(), low5);
  EXPECT_EQ(queue.Pop(), low4);

  EXPECT_TRUE(queue.IsEmpty());
}

TEST_F(PredictorTest, CanonicalizeUrl) {
  // Base case, only handles HTTP and HTTPS.
  EXPECT_EQ(GURL(), Predictor::CanonicalizeUrl(GURL("ftp://anything")));

  // Remove path testing.
  GURL long_url("http://host:999/path?query=value");
  EXPECT_EQ(Predictor::CanonicalizeUrl(long_url), long_url.GetWithEmptyPath());

  // Default port cannoncalization.
  GURL implied_port("http://test");
  GURL explicit_port("http://test:80");
  EXPECT_EQ(Predictor::CanonicalizeUrl(implied_port),
            Predictor::CanonicalizeUrl(explicit_port));

  // Port is still maintained.
  GURL port_80("http://test:80");
  GURL port_90("http://test:90");
  EXPECT_NE(Predictor::CanonicalizeUrl(port_80),
            Predictor::CanonicalizeUrl(port_90));

  // Host is still maintained.
  GURL host_1("http://test_1");
  GURL host_2("http://test_2");
  EXPECT_NE(Predictor::CanonicalizeUrl(host_1),
            Predictor::CanonicalizeUrl(host_2));

  // Scheme is maintained (mismatch identified).
  GURL http("http://test");
  GURL https("https://test");
  EXPECT_NE(Predictor::CanonicalizeUrl(http),
            Predictor::CanonicalizeUrl(https));

  // Https works fine.
  GURL long_https("https://host:999/path?query=value");
  EXPECT_EQ(Predictor::CanonicalizeUrl(long_https),
            long_https.GetWithEmptyPath());
}

TEST_F(PredictorTest, DiscardPredictorResults) {
  SimplePredictor predictor(true);
  base::ListValue referral_list;
  predictor.SerializeReferrers(&referral_list);
  EXPECT_EQ(1U, referral_list.GetSize());

  GURL host_1("http://test_1");
  GURL host_2("http://test_2");
  predictor.LearnFromNavigation(host_1, host_2);

  referral_list.Clear();
  predictor.SerializeReferrers(&referral_list);
  EXPECT_EQ(2U, referral_list.GetSize());

  predictor.DiscardAllResults();
  referral_list.Clear();
  predictor.SerializeReferrers(&referral_list);
  EXPECT_EQ(1U, referral_list.GetSize());

  predictor.Shutdown();
}

class TestPredictorObserver : public PredictorObserver {
 public:
  // PredictorObserver implementation:
  void OnPreconnectUrl(const GURL& url,
                       const GURL& site_for_cookies,
                       UrlInfo::ResolutionMotivation motivation,
                       int count) override {
    preconnected_urls_.push_back(url);
  }

  std::vector<GURL> preconnected_urls_;
};

// Tests that preconnects apply the HSTS list.
TEST_F(PredictorTest, HSTSRedirect) {
  const GURL kHttpUrl("http://example.com");
  const GURL kHttpsUrl("https://example.com");

  const base::Time expiry =
      base::Time::Now() + base::TimeDelta::FromSeconds(1000);
  net::TransportSecurityState state;
  state.AddHSTS(kHttpUrl.host(), expiry, false);

  Predictor predictor(true);
  TestPredictorObserver observer;
  predictor.SetObserver(&observer);
  predictor.SetTransportSecurityState(&state);

  predictor.PreconnectUrl(kHttpUrl, GURL(), UrlInfo::OMNIBOX_MOTIVATED, true,
                          2);
  ASSERT_EQ(1u, observer.preconnected_urls_.size());
  EXPECT_EQ(kHttpsUrl, observer.preconnected_urls_[0]);

  predictor.Shutdown();
}

// Tests that preconnecting a URL on the HSTS list preconnects the subresources
// for the SSL version.
TEST_F(PredictorTest, HSTSRedirectSubresources) {
  const GURL kHttpUrl("http://example.com");
  const GURL kHttpsUrl("https://example.com");
  const GURL kSubresourceUrl("https://images.example.com");
  const double kUseRate = 23.4;

  const base::Time expiry =
      base::Time::Now() + base::TimeDelta::FromSeconds(1000);
  net::TransportSecurityState state;
  state.AddHSTS(kHttpUrl.host(), expiry, false);

  SimplePredictor predictor(true);
  TestPredictorObserver observer;
  predictor.SetObserver(&observer);
  predictor.SetTransportSecurityState(&state);

  std::unique_ptr<base::ListValue> referral_list(NewEmptySerializationList());
  AddToSerializedList(
      kHttpsUrl, kSubresourceUrl, kUseRate, referral_list.get());
  predictor.DeserializeReferrers(*referral_list.get());

  predictor.PreconnectUrlAndSubresources(kHttpUrl, GURL());
  ASSERT_EQ(2u, observer.preconnected_urls_.size());
  EXPECT_EQ(kHttpsUrl, observer.preconnected_urls_[0]);
  EXPECT_EQ(kSubresourceUrl, observer.preconnected_urls_[1]);

  predictor.Shutdown();
}

TEST_F(PredictorTest, HSTSRedirectLearnedSubresource) {
  const GURL kHttpUrl("http://example.com");
  const GURL kHttpsUrl("https://example.com");
  const GURL kSubresourceUrl("https://images.example.com");

  const base::Time expiry =
      base::Time::Now() + base::TimeDelta::FromSeconds(1000);
  net::TransportSecurityState state;
  state.AddHSTS(kHttpUrl.host(), expiry, false);

  SimplePredictor predictor(true);
  TestPredictorObserver observer;
  predictor.SetObserver(&observer);
  predictor.SetTransportSecurityState(&state);

  // Note that the predictor would also learn the HSTS redirect from kHttpUrl to
  // kHttpsUrl during the navigation.
  predictor.LearnFromNavigation(kHttpUrl, kSubresourceUrl);

  predictor.PreconnectUrlAndSubresources(kHttpUrl, GURL());
  ASSERT_EQ(2u, observer.preconnected_urls_.size());
  EXPECT_EQ(kHttpsUrl, observer.preconnected_urls_[0]);
  EXPECT_EQ(kSubresourceUrl, observer.preconnected_urls_[1]);

  predictor.Shutdown();
}

TEST_F(PredictorTest, NoProxyService) {
  // Don't actually try to resolve names.
  Predictor::set_max_parallel_resolves(0);

  Predictor testing_master(true);

  GURL goog("http://www.google.com:80");
  testing_master.Resolve(goog, UrlInfo::OMNIBOX_MOTIVATED);
  EXPECT_FALSE(testing_master.work_queue_.IsEmpty());

  testing_master.Shutdown();
}

TEST_F(PredictorTest, ProxyDefinitelyEnabled) {
  // Don't actually try to resolve names.
  Predictor::set_max_parallel_resolves(0);

  Predictor testing_master(true);

  net::ProxyConfig config;
  config.proxy_rules().ParseFromString("http=socks://localhost:12345");
  std::unique_ptr<net::ProxyResolutionService> proxy_resolution_service(
      net::ProxyResolutionService::CreateFixed(net::ProxyConfigWithAnnotation(
          config, TRAFFIC_ANNOTATION_FOR_TESTS)));
  testing_master.proxy_resolution_service_ = proxy_resolution_service.get();

  GURL goog("http://www.google.com:80");
  testing_master.Resolve(goog, UrlInfo::OMNIBOX_MOTIVATED);

  // Proxy is definitely in use, so there is no need to pre-resolve the domain.
  EXPECT_TRUE(testing_master.work_queue_.IsEmpty());

  testing_master.Shutdown();
}

TEST_F(PredictorTest, ProxyDefinitelyNotEnabled) {
  // Don't actually try to resolve names.
  Predictor::set_max_parallel_resolves(0);

  Predictor testing_master(true);
  net::ProxyConfigWithAnnotation config =
      net::ProxyConfigWithAnnotation::CreateDirect();
  std::unique_ptr<net::ProxyResolutionService> proxy_resolution_service(
      net::ProxyResolutionService::CreateFixed(config));
  testing_master.proxy_resolution_service_ = proxy_resolution_service.get();

  GURL goog("http://www.google.com:80");
  testing_master.Resolve(goog, UrlInfo::OMNIBOX_MOTIVATED);

  // Proxy is not in use, so the name has been registered for pre-resolve.
  EXPECT_FALSE(testing_master.work_queue_.IsEmpty());

  testing_master.Shutdown();
}

TEST_F(PredictorTest, ProxyMaybeEnabled) {
  // Don't actually try to resolve names.
  Predictor::set_max_parallel_resolves(0);

  Predictor testing_master(true);
  net::ProxyConfig config = net::ProxyConfig::CreateFromCustomPacURL(GURL(
      "http://foopy/proxy.pac"));
  std::unique_ptr<net::ProxyResolutionService> proxy_resolution_service(
      net::ProxyResolutionService::CreateFixed(net::ProxyConfigWithAnnotation(
          config, TRAFFIC_ANNOTATION_FOR_TESTS)));
  testing_master.proxy_resolution_service_ = proxy_resolution_service.get();

  GURL goog("http://www.google.com:80");
  testing_master.Resolve(goog, UrlInfo::OMNIBOX_MOTIVATED);

  // Proxy may not be in use (the PAC script has not yet been evaluated), so the
  // name has been registered for pre-resolve.
  EXPECT_FALSE(testing_master.work_queue_.IsEmpty());

  testing_master.Shutdown();
}

TEST_F(PredictorTest, PredictorDisabledByNetworkPredictionFeature) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(features::kNetworkPrediction);

  std::unique_ptr<Predictor> testing_master =
      base::WrapUnique(Predictor::CreatePredictor(true));
  EXPECT_FALSE(testing_master->PredictorEnabled());
  testing_master->Shutdown();
}

}  // namespace chrome_browser_net
