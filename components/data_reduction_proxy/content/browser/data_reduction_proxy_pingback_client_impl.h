// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_DATA_REDUCTION_PROXY_PINGBACK_CLIENT_IMPL_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_DATA_REDUCTION_PROXY_PINGBACK_CLIENT_IMPL_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_pingback_client.h"
#include "components/data_reduction_proxy/proto/pageload_metrics.pb.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

#if defined(OS_ANDROID)
#include "components/crash/content/browser/crash_dump_manager_android.h"
#endif

namespace base {
class Time;
}

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}  // namespace net

namespace data_reduction_proxy {
class DataReductionProxyData;
struct DataReductionProxyPageLoadTiming;

// Manages pingbacks about page load timing information to the data saver proxy
// server. This class is not thread safe.
class DataReductionProxyPingbackClientImpl
    : public DataReductionProxyPingbackClient,
      public net::URLFetcherDelegate
#if defined(OS_ANDROID)
    ,
      public breakpad::CrashDumpManager::Observer
#endif
{
 public:
  // The caller must ensure that |url_request_context| remains alive for the
  // lifetime of the |DataReductionProxyPingbackClientImpl| instance.
  DataReductionProxyPingbackClientImpl(
      net::URLRequestContextGetter* url_request_context,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner);
  ~DataReductionProxyPingbackClientImpl() override;

 protected:
  // Generates a float in the range [0, 1). Virtualized in testing.
  virtual float GenerateRandomFloat() const;

  // Returns the current time. Virtualized in testing.
  virtual base::Time CurrentTime() const;

 private:
  // DataReductionProxyPingbackClient:
  void SendPingback(const DataReductionProxyData& data,
                    const DataReductionProxyPageLoadTiming& timing) override;
  void SetPingbackReportingFraction(float pingback_reporting_fraction) override;

  // URLFetcherDelegate implmentation:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Whether a pingback should be sent.
  bool ShouldSendPingback() const;

#if defined(OS_ANDROID)
  // CrashDumpManager::Observer:
  void OnCrashDumpProcessed(
      const breakpad::CrashDumpManager::CrashDumpDetails& details) override;

  // Creates a pending pingback report that waits for the crash dump to be
  // processed. If the dump is not processed in 5 seconds, the report is sent
  // without the cause of the crash.
  void AddRequestToCrashMap(const DataReductionProxyData& request_data,
                            const DataReductionProxyPageLoadTiming& timing);

  // Reports the crashed renderer page load information without the cause of the
  // crash.
  void RemoveFromCrashMap(int process_host_id);
#endif

  // Creates the proto page load report and adds it to the current pending batch
  // of reports. If there is no outstanding request, sends the batched report.
  void CreateReport(const DataReductionProxyData& request_data,
                    const DataReductionProxyPageLoadTiming& timing,
                    PageloadMetrics_RendererCrashType crash_type);

  // Creates an URLFetcher that will POST to |secure_proxy_url_| using
  // |url_request_context_|. The max retries is set to 5.
  // |data_to_send_| will be used to fill the body of the Fetcher, and will be
  // reset to an empty RecordPageloadMetricsRequest.
  void CreateFetcherForDataAndStart();

  net::URLRequestContextGetter* url_request_context_;

  // The URL for the data saver proxy's ping back service.
  const GURL pingback_url_;

  // The currently running fetcher.
  std::unique_ptr<net::URLFetcher> current_fetcher_;

  // Serialized data to send to the data saver proxy server.
  RecordPageloadMetricsRequest metrics_request_;

  // The probability of sending a pingback to the server.
  float pingback_reporting_fraction_;

  // The number of pageload messages in the current fetcher.
  size_t current_fetcher_message_count_;

  // The number of pageload crash messages in the current fetcher.
  size_t current_fetcher_crash_count_;

  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

#if defined(OS_ANDROID)
  typedef std::tuple<DataReductionProxyData, DataReductionProxyPageLoadTiming>
      CrashPageLoadInformation;

  // Maps host process ID to information for the pingback. Items are added to
  // the crash map when the renderer process crashes. If OnCrashDumpProcessed is
  // not called within 5 seconds, the report is sent without the cause of the
  // crash.
  std::map<int, CrashPageLoadInformation> crash_map_;

  ScopedObserver<breakpad::CrashDumpManager,
                 breakpad::CrashDumpManager::Observer>
      scoped_observer_;
#endif

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<DataReductionProxyPingbackClientImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyPingbackClientImpl);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_DATA_REDUCTION_PROXY_PINGBACK_CLIENT_IMPL_H_
