// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_USAGE_ANDROID_TRAFFIC_STATS_AMORTIZER_H_
#define COMPONENTS_DATA_USAGE_ANDROID_TRAFFIC_STATS_AMORTIZER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/data_usage/core/data_use_amortizer.h"

namespace base {
class TickClock;
class Timer;
}

namespace data_usage {

struct DataUse;

namespace android {

// Class that uses Android TrafficStats to amortize any unincluded overhead
// (e.g. network layer, TLS, DNS) into the data usage reported by the network
// stack. Should only be used on the IO thread. Since TrafficStats measurements
// are global for the entire application, a TrafficStatsAmortizer should be
// notified of every byte possible, or else it might mistakenly classify the
// corresponding additional TrafficStats bytes for those as overhead. The
// TrafficStats API has been available in Android since API level 8 (Android
// 2.2).
class TrafficStatsAmortizer : public DataUseAmortizer {
 public:
  TrafficStatsAmortizer();
  ~TrafficStatsAmortizer() override;

  // Amortizes any unincluded network bytes overhead for |data_use| into
  // |data_use|, and passes the updated |data_use| to |callback| once
  // amortization is complete. The TrafficStatsAmortizer may combine together
  // consecutive |data_use| objects that have the same |callback| if the
  // |data_use| objects are identical in all ways but their byte counts, such
  // that |callback| will only be called once with the single combined DataUse
  // object.
  void AmortizeDataUse(std::unique_ptr<DataUse> data_use,
                       const AmortizationCompleteCallback& callback) override;

  // Notifies the amortizer that some extra bytes have been transferred that
  // aren't associated with any DataUse objects (e.g. off-the-record traffic),
  // so that the TrafficStatsAmortizer can avoid mistakenly counting these bytes
  // as overhead.
  void OnExtraBytes(int64_t extra_tx_bytes, int64_t extra_rx_bytes) override;

  base::WeakPtr<TrafficStatsAmortizer> GetWeakPtr();

 protected:
  // Constructor for testing purposes, allowing for tests to take full control
  // over the timing of the TrafficStatsAmortizer and the byte counts returned
  // from TrafficStats. |traffic_stats_query_timer| must not be a repeating
  // timer.
  TrafficStatsAmortizer(const base::TickClock* tick_clock,
                        std::unique_ptr<base::Timer> traffic_stats_query_timer,
                        const base::TimeDelta& traffic_stats_query_delay,
                        const base::TimeDelta& max_amortization_delay,
                        size_t max_data_use_buffer_size);

  // Queries the total transmitted and received bytes for the application from
  // TrafficStats. Stores the byte counts in |tx_bytes| and |rx_bytes|
  // respectively and returns true if both values are available from
  // TrafficStats, otherwise returns false. |tx_bytes| and |rx_bytes| must not
  // be NULL.
  // Virtual for testing.
  virtual bool QueryTrafficStats(int64_t* tx_bytes, int64_t* rx_bytes) const;

 private:
  // Adds |tx_bytes| and |rx_bytes| as data usage that should not be counted as
  // overhead (i.e. bytes from DataUse objects and extra bytes reported to this
  // TrafficStatsAmortizer), and schedules amortization to happen later.
  void AddPreAmortizationBytes(int64_t tx_bytes, int64_t rx_bytes);

  // Amortizes any additional overhead from TrafficStats byte counts into the
  // |buffered_data_use_|, then passes the post-amortization DataUse objects to
  // their respective callbacks, flushing |buffered_data_use_|. Overhead is
  // calculated as the difference between the TrafficStats byte counts and the
  // pre-amortization byte counts.
  void AmortizeNow();

  base::ThreadChecker thread_checker_;

  // TickClock for determining the current time tick.
  const base::TickClock* tick_clock_;

  // One-shot timer used to wait a short time after receiving DataUse before
  // querying TrafficStats, to give TrafficStats time to update and give the
  // network stack time to finish reporting multiple DataUse objects that happen
  // in rapid succession. This must not be a repeating timer.
  // |traffic_stats_query_timer_| is owned as a scoped_ptr so that fake timers
  // can be passed in for tests.
  std::unique_ptr<base::Timer> traffic_stats_query_timer_;

  // The delay between data usage being reported to the amortizer before
  // querying TrafficStats. Used with |traffic_stats_query_timer_|.
  const base::TimeDelta traffic_stats_query_delay_;

  // The maximum amount of time that the TrafficStatsAmortizer is allowed to
  // spend waiting to perform amortization. Used with
  // |traffic_stats_query_timer_|.
  const base::TimeDelta max_amortization_delay_;

  // The maximum allowed size of the |buffered_data_use_| buffer, to prevent the
  // buffer from hogging memory.
  const size_t max_data_use_buffer_size_;

  // Indicates whether or not the TrafficStatsAmortizer currently has
  // pre-amortization bytes waiting for amortization to be performed.
  bool is_amortization_in_progress_;

  // The time when the first pre-amortization bytes for the current amortization
  // run were given to this TrafficStatsAmortizer.
  base::TimeTicks current_amortization_run_start_time_;

  // Buffer of pre-amortization data use that has accumulated since the last
  // time amortization was performed, paired with the callbacks for each DataUse
  // object.
  std::vector<std::pair<std::unique_ptr<DataUse>, AmortizationCompleteCallback>>
      buffered_data_use_;

  // Indicates if TrafficStats byte counts were available during the last time
  // amortization was performed.
  bool are_last_amortization_traffic_stats_available_;

  // The total transmitted bytes according to TrafficStats during the last time
  // amortization was performed, if they were available.
  int64_t last_amortization_traffic_stats_tx_bytes_;

  // The total received bytes according to TrafficStats during the last time
  // amortization was performed, if they were available.
  int64_t last_amortization_traffic_stats_rx_bytes_;

  // Total pre-amortization transmitted bytes since the last time amortization
  // was performed, including bytes from |buffered_data_use_| and any extra
  // bytes that were added.
  int64_t pre_amortization_tx_bytes_;

  // Total pre-amortization received bytes since the last time amortization was
  // performed, including bytes from |buffered_data_use_| and any extra bytes
  // that were added.
  int64_t pre_amortization_rx_bytes_;

  base::WeakPtrFactory<TrafficStatsAmortizer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrafficStatsAmortizer);
};

}  // namespace android

}  // namespace data_usage

#endif  // COMPONENTS_DATA_USAGE_ANDROID_TRAFFIC_STATS_AMORTIZER_H_
