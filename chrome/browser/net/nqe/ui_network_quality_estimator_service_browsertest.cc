// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/net/nqe/ui_network_quality_estimator_service.h"
#include "chrome/browser/net/nqe/ui_network_quality_estimator_service_factory.h"
#include "chrome/browser/net/nqe/ui_network_quality_estimator_service_test_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_test.h"
#include "net/base/network_change_notifier.h"
#include "net/nqe/cached_network_quality.h"
#include "net/nqe/effective_connection_type.h"
#include "net/nqe/effective_connection_type_observer.h"
#include "net/nqe/network_id.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/nqe/rtt_throughput_estimates_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

class Profile;

namespace {

class TestEffectiveConnectionTypeObserver
    : public net::EffectiveConnectionTypeObserver {
 public:
  TestEffectiveConnectionTypeObserver()
      : effective_connection_type_(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN) {}
  ~TestEffectiveConnectionTypeObserver() override {}

  // net::EffectiveConnectionTypeObserver implementation:
  void OnEffectiveConnectionTypeChanged(
      net::EffectiveConnectionType type) override {
    effective_connection_type_ = type;
  }

  // The most recently set EffectiveConnectionType.
  net::EffectiveConnectionType effective_connection_type() const {
    return effective_connection_type_;
  }

 private:
  net::EffectiveConnectionType effective_connection_type_;

  DISALLOW_COPY_AND_ASSIGN(TestEffectiveConnectionTypeObserver);
};

class TestRTTAndThroughputEstimatesObserver
    : public net::RTTAndThroughputEstimatesObserver {
 public:
  TestRTTAndThroughputEstimatesObserver()
      : http_rtt_(base::TimeDelta::FromMilliseconds(-1)),
        transport_rtt_(base::TimeDelta::FromMilliseconds(-1)),
        downstream_throughput_kbps_(-1) {}
  ~TestRTTAndThroughputEstimatesObserver() override {}

  // net::RTTAndThroughputEstimatesObserver implementation:
  void OnRTTOrThroughputEstimatesComputed(
      base::TimeDelta http_rtt,
      base::TimeDelta transport_rtt,
      int32_t downstream_throughput_kbps) override {
    http_rtt_ = http_rtt;
    transport_rtt_ = transport_rtt;
    downstream_throughput_kbps_ = downstream_throughput_kbps;
  }

  base::TimeDelta http_rtt() const { return http_rtt_; }
  base::TimeDelta transport_rtt() const { return transport_rtt_; }
  int32_t downstream_throughput_kbps() const {
    return downstream_throughput_kbps_;
  }

 private:
  base::TimeDelta http_rtt_;
  base::TimeDelta transport_rtt_;
  int32_t downstream_throughput_kbps_;

  DISALLOW_COPY_AND_ASSIGN(TestRTTAndThroughputEstimatesObserver);
};

class UINetworkQualityEstimatorServiceBrowserTest
    : public InProcessBrowserTest {
 public:
  UINetworkQualityEstimatorServiceBrowserTest() {}

  // Verifies that the network quality prefs are written amd read correctly.
  void VerifyWritingReadingPrefs() {
    variations::testing::ClearAllVariationParams();
    std::map<std::string, std::string> variation_params;

    variations::AssociateVariationParams("NetworkQualityEstimator", "Enabled",
                                         variation_params);
    base::FieldTrialList::CreateFieldTrial("NetworkQualityEstimator",
                                           "Enabled");

    // Verifies that NQE notifying EffectiveConnectionTypeObservers causes the
    // UINetworkQualityEstimatorService to receive an updated
    // EffectiveConnectionType.
    Profile* profile = ProfileManager::GetActiveUserProfile();

    UINetworkQualityEstimatorService* nqe_service =
        UINetworkQualityEstimatorServiceFactory::GetForProfile(profile);
    ASSERT_NE(nullptr, nqe_service);
    // NetworkQualityEstimator must be notified of the read prefs at startup.
    EXPECT_FALSE(histogram_tester_.GetAllSamples("NQE.Prefs.ReadSize").empty());

    {
      base::HistogramTester histogram_tester;
      nqe_test_util::OverrideEffectiveConnectionTypeAndWait(
          net::EFFECTIVE_CONNECTION_TYPE_OFFLINE);
      EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_OFFLINE,
                nqe_service->GetEffectiveConnectionType());

      // Prefs are written only if persistent caching was enabled.
      EXPECT_FALSE(
          histogram_tester.GetAllSamples("NQE.Prefs.WriteCount").empty());
      histogram_tester.ExpectTotalCount("NQE.Prefs.ReadCount", 0);

      // NetworkQualityEstimator should not be notified of change in prefs.
      histogram_tester.ExpectTotalCount("NQE.Prefs.ReadSize", 0);
    }

    {
      base::HistogramTester histogram_tester;
      nqe_test_util::OverrideEffectiveConnectionTypeAndWait(
          net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G);

      EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G,
                nqe_service->GetEffectiveConnectionType());

      // Prefs are written even if the network id was unavailable.
      EXPECT_FALSE(
          histogram_tester.GetAllSamples("NQE.Prefs.WriteCount").empty());
      histogram_tester.ExpectTotalCount("NQE.Prefs.ReadCount", 0);

      // NetworkQualityEstimator should not be notified of change in prefs.
      histogram_tester.ExpectTotalCount("NQE.Prefs.ReadSize", 0);
    }

    // Verify the contents of the prefs by reading them again.
    std::map<net::nqe::internal::NetworkID,
             net::nqe::internal::CachedNetworkQuality>
        read_prefs = nqe_service->ForceReadPrefsForTesting();
    // Number of entries must be between 1 and 2. It's possible that 2 entries
    // are added if the connection type is unknown to network quality estimator
    // at the time of startup, and shortly after it receives a notification
    // about the change in the connection type.
    EXPECT_LE(1u, read_prefs.size());
    EXPECT_GE(2u, read_prefs.size());

    // Verify that the cached network quality was written correctly.
    EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G,
              read_prefs.begin()->second.effective_connection_type());
    if (net::NetworkChangeNotifier::GetConnectionType() ==
        net::NetworkChangeNotifier::CONNECTION_ETHERNET) {
      // Verify that the network ID was written correctly.
      net::nqe::internal::NetworkID ethernet_network_id(
          net::NetworkChangeNotifier::CONNECTION_ETHERNET, std::string(),
          INT32_MIN);
      EXPECT_EQ(ethernet_network_id, read_prefs.begin()->first);
      }
  }

 private:
  base::HistogramTester histogram_tester_;

  DISALLOW_COPY_AND_ASSIGN(UINetworkQualityEstimatorServiceBrowserTest);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(UINetworkQualityEstimatorServiceBrowserTest,
                       VerifyNQEState) {
  // Verifies that NQE notifying EffectiveConnectionTypeObservers causes the
  // UINetworkQualityEstimatorService to receive an updated
  // EffectiveConnectionType.
  Profile* profile = ProfileManager::GetActiveUserProfile();
  UINetworkQualityEstimatorService* nqe_service =
      UINetworkQualityEstimatorServiceFactory::GetForProfile(profile);
  TestEffectiveConnectionTypeObserver nqe_observer;
  nqe_service->AddEffectiveConnectionTypeObserver(&nqe_observer);

  nqe_test_util::OverrideEffectiveConnectionTypeAndWait(
      net::EFFECTIVE_CONNECTION_TYPE_OFFLINE);
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_OFFLINE,
            nqe_service->GetEffectiveConnectionType());
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_OFFLINE,
            nqe_observer.effective_connection_type());

  nqe_test_util::OverrideEffectiveConnectionTypeAndWait(
      net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G);
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G,
            nqe_service->GetEffectiveConnectionType());
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G,
            nqe_observer.effective_connection_type());

  nqe_service->RemoveEffectiveConnectionTypeObserver(&nqe_observer);

  nqe_test_util::OverrideEffectiveConnectionTypeAndWait(
      net::EFFECTIVE_CONNECTION_TYPE_OFFLINE);
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_OFFLINE,
            nqe_service->GetEffectiveConnectionType());
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G,
            nqe_observer.effective_connection_type());

  // Observer should be notified on addition.
  TestEffectiveConnectionTypeObserver nqe_observer_2;
  nqe_service->AddEffectiveConnectionTypeObserver(&nqe_observer_2);
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN,
            nqe_observer_2.effective_connection_type());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_OFFLINE,
            nqe_observer_2.effective_connection_type());

  // |nqe_observer_3| should be not notified since it unregisters before the
  // message loop is run.
  TestEffectiveConnectionTypeObserver nqe_observer_3;
  nqe_service->AddEffectiveConnectionTypeObserver(&nqe_observer_3);
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN,
            nqe_observer_3.effective_connection_type());
  nqe_service->RemoveEffectiveConnectionTypeObserver(&nqe_observer_3);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN,
            nqe_observer_3.effective_connection_type());
}

IN_PROC_BROWSER_TEST_F(UINetworkQualityEstimatorServiceBrowserTest,
                       VerifyRTTs) {
  // Verifies that NQE notifying RTTAndThroughputEstimatesObserver causes the
  // UINetworkQualityEstimatorService to receive an update.
  Profile* profile = ProfileManager::GetActiveUserProfile();
  UINetworkQualityEstimatorService* nqe_service =
      UINetworkQualityEstimatorServiceFactory::GetForProfile(profile);
  TestRTTAndThroughputEstimatesObserver nqe_observer;
  nqe_service->AddRTTAndThroughputEstimatesObserver(&nqe_observer);

  base::TimeDelta rtt_1 = base::TimeDelta::FromMilliseconds(100);

  nqe_test_util::OverrideRTTsAndWait(rtt_1);
  EXPECT_EQ(rtt_1, nqe_observer.http_rtt());

  EXPECT_EQ(rtt_1, nqe_service->GetHttpRTT());
  EXPECT_EQ(rtt_1, nqe_service->GetTransportRTT());
  EXPECT_FALSE(nqe_service->GetDownstreamThroughputKbps().has_value());

  base::TimeDelta rtt_2 = base::TimeDelta::FromMilliseconds(200);

  nqe_test_util::OverrideRTTsAndWait(rtt_2);
  EXPECT_EQ(rtt_2, nqe_observer.http_rtt());

  EXPECT_EQ(rtt_2, nqe_service->GetHttpRTT());
  EXPECT_EQ(rtt_2, nqe_service->GetTransportRTT());
  EXPECT_FALSE(nqe_service->GetDownstreamThroughputKbps().has_value());

  nqe_service->RemoveRTTAndThroughputEstimatesObserver(&nqe_observer);

  base::TimeDelta rtt_3 = base::TimeDelta::FromMilliseconds(300);

  nqe_test_util::OverrideRTTsAndWait(rtt_3);
  EXPECT_EQ(rtt_2, nqe_observer.http_rtt());

  EXPECT_EQ(rtt_3, nqe_service->GetHttpRTT());
  EXPECT_EQ(rtt_3, nqe_service->GetTransportRTT());
  EXPECT_FALSE(nqe_service->GetDownstreamThroughputKbps().has_value());

  // Observer should be notified on addition.
  TestRTTAndThroughputEstimatesObserver nqe_observer_2;
  nqe_service->AddRTTAndThroughputEstimatesObserver(&nqe_observer_2);
  EXPECT_GT(0, nqe_observer_2.http_rtt().InMilliseconds());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(rtt_3, nqe_observer_2.http_rtt());

  // |nqe_observer_3| should be not notified since it unregisters before the
  // message loop is run.
  TestRTTAndThroughputEstimatesObserver nqe_observer_3;
  nqe_service->AddRTTAndThroughputEstimatesObserver(&nqe_observer_3);
  EXPECT_GT(0, nqe_observer_3.http_rtt().InMilliseconds());
  nqe_service->RemoveRTTAndThroughputEstimatesObserver(&nqe_observer_3);
  base::RunLoop().RunUntilIdle();
  EXPECT_GT(0, nqe_observer_3.http_rtt().InMilliseconds());
}

// Verify that prefs are writen and read correctly.
IN_PROC_BROWSER_TEST_F(UINetworkQualityEstimatorServiceBrowserTest,
                       WritingReadingToPrefsEnabled) {
  VerifyWritingReadingPrefs();
}
