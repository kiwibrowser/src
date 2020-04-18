// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_ANTIVIRUS_METRICS_PROVIDER_WIN_H_
#define CHROME_BROWSER_METRICS_ANTIVIRUS_METRICS_PROVIDER_WIN_H_

#include "components/metrics/metrics_provider.h"

#include <iwscapi.h>
#include <stddef.h>

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/feature_list.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "third_party/metrics_proto/system_profile.pb.h"

// AntiVirusMetricsProvider is responsible for adding antivirus information to
// the UMA system profile proto.
class AntiVirusMetricsProvider : public metrics::MetricsProvider {
 public:
  static constexpr base::Feature kReportNamesFeature = {
      "ReportFullAVProductDetails", base::FEATURE_DISABLED_BY_DEFAULT};

  AntiVirusMetricsProvider();

  ~AntiVirusMetricsProvider() override;

  // metrics::MetricsDataProvider:
  void AsyncInit(const base::Closure& done_callback) override;
  void ProvideSystemProfileMetrics(
      metrics::SystemProfileProto* system_profile_proto) override;

 private:
  // This enum is reported via a histogram so new values should always be added
  // at the end.
  enum ResultCode {
    RESULT_SUCCESS = 0,
    RESULT_GENERIC_FAILURE = 1,
    RESULT_FAILED_TO_INITIALIZE_COM = 2,
    RESULT_FAILED_TO_CREATE_INSTANCE = 3,
    RESULT_FAILED_TO_INITIALIZE_PRODUCT_LIST = 4,
    RESULT_FAILED_TO_GET_PRODUCT_COUNT = 5,
    RESULT_FAILED_TO_GET_ITEM = 6,
    RESULT_FAILED_TO_GET_PRODUCT_STATE = 7,
    RESULT_PRODUCT_STATE_INVALID = 8,
    RESULT_FAILED_TO_GET_PRODUCT_NAME = 9,
    RESULT_FAILED_TO_GET_REMEDIATION_PATH = 10,
    RESULT_FAILED_TO_CONNECT_TO_WMI = 11,
    RESULT_FAILED_TO_SET_SECURITY_BLANKET = 12,
    RESULT_FAILED_TO_EXEC_WMI_QUERY = 13,
    RESULT_FAILED_TO_ITERATE_RESULTS = 14,
    RESULT_WSC_NOT_AVAILABLE = 15,
    RESULT_COUNT = 16
  };

  typedef metrics::SystemProfileProto::AntiVirusProduct AvProduct;

  // Query COM interface IWSCProductList for installed AV products. This
  // interface is only available on Windows 8 and above.
  static ResultCode FillAntiVirusProductsFromWSC(
      std::vector<AvProduct>* products);

  // Query WMI ROOT\SecurityCenter2 for installed AV products. This interface is
  // only available on Windows Vista and above.
  static ResultCode FillAntiVirusProductsFromWMI(
      std::vector<AvProduct>* products);

  // Query local machine configuration for other products that might not be
  // registered in WMI or Security Center and add them to the product vector.
  static void MaybeAddUnregisteredAntiVirusProducts(
      std::vector<AvProduct>* products);

  static std::vector<AvProduct> GetAntiVirusProductsOnCOMSTAThread();

  // Removes anything extraneous from the end of the product name such as
  // versions, years, or anything containing numbers to make it more constant.
  static std::string TrimVersionOfAvProductName(const std::string& av_product);

  // Called when metrics are done being gathered from the FILE thread.
  // |done_callback| is the callback that should be called once all metrics are
  // gathered.
  void GotAntiVirusProducts(const base::Closure& done_callback,
                            const std::vector<AvProduct>& av_products);

  // Information on installed AntiVirus gathered.
  std::vector<AvProduct> av_products_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<AntiVirusMetricsProvider> weak_ptr_factory_;

  FRIEND_TEST_ALL_PREFIXES(AntiVirusMetricsProviderTest, GetMetricsFullName);
  FRIEND_TEST_ALL_PREFIXES(AntiVirusMetricsProviderSimpleTest,
                           StripProductVersion);

  DISALLOW_COPY_AND_ASSIGN(AntiVirusMetricsProvider);
};

#endif  // CHROME_BROWSER_METRICS_ANTIVIRUS_METRICS_PROVIDER_WIN_H_
