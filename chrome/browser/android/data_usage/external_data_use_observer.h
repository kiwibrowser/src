// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DATA_USAGE_EXTERNAL_DATA_USE_OBSERVER_H_
#define CHROME_BROWSER_ANDROID_DATA_USAGE_EXTERNAL_DATA_USE_OBSERVER_H_

#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/data_usage/core/data_use_aggregator.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace data_usage {
struct DataUse;
}

namespace android {

class DataUseTabModel;
class ExternalDataUseObserverBridge;
class ExternalDataUseReporter;

// This class allows platform APIs that are external to Chromium to observe how
// much data is used by Chromium on the current Android device. This class
// registers as a data use observer with DataUseAggregator (as long as there is
// at least one valid matching rule is present), collects a batch of data
// use observations and passes them to ExternalDataUseReporter for labeling the
// data usage and reporting to the platform. This class also fetches the
// matching rules from external platform APIs, on demand and periodically. This
// class is not thread safe, and must only be accessed on IO thread.
class ExternalDataUseObserver : public data_usage::DataUseAggregator::Observer {
 public:
  // External data use observer field trial name.
  static const char kExternalDataUseObserverFieldTrial[];

  ExternalDataUseObserver(
      data_usage::DataUseAggregator* data_use_aggregator,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner);
  ~ExternalDataUseObserver() override;

  // Returns the pointer to the DataUseTabModel object owned by |this|. The
  // caller does not owns the returned pointer.
  DataUseTabModel* GetDataUseTabModel() const;

  // Called by ExternalDataUseObserverBridge. |should_register| is true if
  // |this| should register as a data use observer.
  void ShouldRegisterAsDataUseObserver(bool should_register);

  // Fetches the matching rules asynchronously.
  void FetchMatchingRules();

  base::WeakPtr<ExternalDataUseObserver> GetWeakPtr();

  // Called by ExternalDataUseObserverBridge::OnReportDataUseDone when a data
  // use report has been submitted. |success| is true if the request was
  // successfully submitted to the external data use observer by Java.
  // TODO(rajendrant): Move this callback to ExternalDataUseReporter. In order
  // to move this, ExternalDataUseObserverBridge needs to hold a pointer to
  // ExternalDataUseReporter. This is currently not doable, since this creates
  // a circular dependency between ExternalDataUseReporter, DataUseTabModel and
  // ExternalDataUseObserverBridge, which creates issues during desctruction.
  void OnReportDataUseDone(bool success);

  ExternalDataUseReporter* GetExternalDataUseReporterForTesting() const {
    return external_data_use_reporter_;
  }

  void SetProfileSigninStatus(bool signin_status);

 private:
  friend class DataUseTabModelTest;
  friend class DataUseUITabModelTest;
  friend class ExternalDataUseObserverTest;
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseObserverTest,
                           MatchingRuleFetchOnControlAppInstall);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseObserverTest,
                           PeriodicFetchMatchingRules);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseObserverTest,
                           RegisteredAsDataUseObserver);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseObserverTest, Variations);
  FRIEND_TEST_ALL_PREFIXES(DataUseUITabModelTest, ReportTabEventsTest);

  // data_usage::DataUseAggregator::Observer implementation:
  void OnDataUse(const data_usage::DataUse& data_use) override;

  // Called when a batch of data use objects are added to |data_use_list_|.
  void OnDataUseBatchComplete();

  // Aggregator that sends data use observations to |this|.
  data_usage::DataUseAggregator* data_use_aggregator_;

  // |external_data_use_observer_bridge_| is owned by |this|, and interacts with
  // the Java code. It is created on IO thread but afterwards, should only be
  // accessed on UI thread.
  ExternalDataUseObserverBridge* external_data_use_observer_bridge_;

  // Maintains tab sessions and is owned by |this|. It is created on IO thread
  // but afterwards, should only be accessed on UI thread.
  DataUseTabModel* data_use_tab_model_;

  // Labels, buffers and reports the data usage. It is owned by |this|. It is
  // created on IO thread but afterwards, should only be accessed on UI thread.
  ExternalDataUseReporter* external_data_use_reporter_;

  // Batches the data use objects reported by DataUseAggregator. This will be
  // created when data use batching starts and released when the batching ends.
  // This will be null if there is no ongoing batching of data use objects.
  std::unique_ptr<std::vector<const data_usage::DataUse>> data_use_list_;

  // |io_task_runner_| is used to call methods on IO thread.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // |ui_task_runner_| is used to call methods on UI thread.
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

  // Time when the matching rules were last fetched.
  base::TimeTicks last_matching_rules_fetch_time_;

  // Duration after which matching rules are periodically fetched.
  const base::TimeDelta fetch_matching_rules_duration_;

  // True if |this| is currently registered as a data use observer.
  bool registered_as_data_use_observer_;

  // True if profile is signed in and authenticated.
  bool profile_signin_status_;

  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<ExternalDataUseObserver> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExternalDataUseObserver);
};

}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_DATA_USAGE_EXTERNAL_DATA_USE_OBSERVER_H_
