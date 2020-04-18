// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_usage/android/traffic_stats_amortizer.h"

#include <algorithm>  // For std::min.
#include <cmath>      // For std::modf.
#include <set>
#include <utility>

#include "base/location.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/default_tick_clock.h"
#include "base/timer/timer.h"
#include "components/data_usage/core/data_use.h"
#include "components/variations/variations_associated_data.h"
#include "net/android/traffic_stats.h"

namespace data_usage {
namespace android {

namespace {

// Convenience typedef.
typedef std::vector<std::pair<std::unique_ptr<DataUse>,
                              DataUseAmortizer::AmortizationCompleteCallback>>
    DataUseBuffer;

// Name of the field trial.
const char kExternalDataUseObserverFieldTrial[] = "ExternalDataUseObserver";

// The delay between receiving DataUse and querying TrafficStats byte counts for
// amortization.
const int64_t kDefaultTrafficStatsQueryDelayMs = 50;

// The longest amount of time that an amortization run can be delayed for.
const int64_t kDefaultMaxAmortizationDelayMs = 500;

// The maximum allowed size of the DataUse buffer. If the buffer ever exceeds
// this size, then DataUse will be amortized immediately and the buffer will be
// flushed.
const size_t kDefaultMaxDataUseBufferSize = 128;

base::TimeDelta GetTrafficStatsQueryDelay() {
  int64_t duration_ms = kDefaultTrafficStatsQueryDelayMs;
  std::string variation_value = variations::GetVariationParamValue(
      kExternalDataUseObserverFieldTrial, "traffic_stats_query_delay_ms");
  if (!variation_value.empty() &&
      base::StringToInt64(variation_value, &duration_ms) && duration_ms >= 0) {
    return base::TimeDelta::FromMilliseconds(duration_ms);
  }
  return base::TimeDelta::FromMilliseconds(kDefaultTrafficStatsQueryDelayMs);
}

base::TimeDelta GetMaxAmortizationDelay() {
  int64_t duration_ms = kDefaultMaxAmortizationDelayMs;
  std::string variation_value = variations::GetVariationParamValue(
      kExternalDataUseObserverFieldTrial, "max_amortization_delay_ms");
  if (!variation_value.empty() &&
      base::StringToInt64(variation_value, &duration_ms) && duration_ms >= 0) {
    return base::TimeDelta::FromMilliseconds(duration_ms);
  }
  return base::TimeDelta::FromMilliseconds(kDefaultMaxAmortizationDelayMs);
}

size_t GetMaxDataUseBufferSize() {
  size_t max_buffer_size = kDefaultMaxDataUseBufferSize;
  std::string variation_value = variations::GetVariationParamValue(
      kExternalDataUseObserverFieldTrial, "max_data_use_buffer_size");
  if (!variation_value.empty() &&
      base::StringToSizeT(variation_value, &max_buffer_size)) {
    return max_buffer_size;
  }
  return kDefaultMaxDataUseBufferSize;
}

// Returns |byte_count| as a histogram sample capped at the maximum histogram
// sample value that's suitable for being recorded without overflowing.
base::HistogramBase::Sample GetByteCountAsHistogramSample(int64_t byte_count) {
  DCHECK_GE(byte_count, 0);
  if (byte_count >= base::HistogramBase::kSampleType_MAX) {
    // Return kSampleType_MAX - 1 because it's invalid to record
    // kSampleType_MAX, which would cause a CHECK to fail in the histogram code.
    return base::HistogramBase::kSampleType_MAX - 1;
  }
  return static_cast<base::HistogramBase::Sample>(byte_count);
}

// Scales |bytes| by |ratio|, using |remainder| to hold the running rounding
// error. |bytes| must be non-negative, and multiplying |bytes| by |ratio| must
// yield a number that's representable within the bounds of a non-negative
// int64_t.
int64_t ScaleByteCount(int64_t bytes, double ratio, double* remainder) {
  DCHECK_GE(bytes, 0);
  DCHECK_GE(ratio, 0.0);
  DCHECK_LE(ratio, static_cast<double>(INT64_MAX));
  DCHECK_GE(*remainder, 0.0);
  DCHECK_LT(*remainder, 1.0);

  double intpart;
  *remainder =
      std::modf(static_cast<double>(bytes) * ratio + (*remainder), &intpart);

  DCHECK_GE(intpart, 0.0);
  DCHECK_LE(intpart, static_cast<double>(INT64_MAX));
  DCHECK_GE(*remainder, 0.0);
  DCHECK_LT(*remainder, 1.0);

  // Due to floating point error, casting the double |intpart| to an int64_t
  // could cause it to overflow, even though it's already been checked to be
  // less than the double representation of INT64_MAX. If this happens, cap the
  // scaled value at INT64_MAX.
  uint64_t scaled_bytes = std::min(static_cast<uint64_t>(intpart),
                                   static_cast<uint64_t>(INT64_MAX));
  return static_cast<int64_t>(scaled_bytes);
}

// Amortizes the difference between |desired_post_amortization_total| and
// |pre_amortization_total| into each of the DataUse objects in
// |data_use_sequence| by scaling the byte counts determined by the
// |get_byte_count_fn| function (e.g. tx_bytes, rx_bytes) for each DataUse
// appropriately. |pre_amortization_total| must not be 0.
void AmortizeByteCountSequence(DataUseBuffer* data_use_sequence,
                               int64_t* (*get_byte_count_fn)(DataUse*),
                               int64_t pre_amortization_total,
                               int64_t desired_post_amortization_total) {
  DCHECK_GT(pre_amortization_total, 0);
  DCHECK_GE(desired_post_amortization_total, 0);

  const double ratio = static_cast<double>(desired_post_amortization_total) /
                       static_cast<double>(pre_amortization_total);

  double remainder = 0.0;
  for (auto& data_use_buffer_pair : *data_use_sequence) {
    int64_t* byte_count = get_byte_count_fn(data_use_buffer_pair.first.get());
    *byte_count = ScaleByteCount(*byte_count, ratio, &remainder);
  }
}

int64_t* GetTxBytes(DataUse* data_use) {
  return &data_use->tx_bytes;
}
int64_t* GetRxBytes(DataUse* data_use) {
  return &data_use->rx_bytes;
}

// Returns the total transmitted bytes contained in |data_use_sequence|.
int64_t GetTotalTxBytes(const DataUseBuffer& data_use_sequence) {
  int64_t sum = 0;
  for (const auto& data_use_buffer_pair : data_use_sequence)
    sum += data_use_buffer_pair.first->tx_bytes;
  return sum;
}

// Returns the total received bytes contained in |data_use_sequence|.
int64_t GetTotalRxBytes(const DataUseBuffer& data_use_sequence) {
  int64_t sum = 0;
  for (const auto& data_use_buffer_pair : data_use_sequence)
    sum += data_use_buffer_pair.first->rx_bytes;
  return sum;
}

void RecordConcurrentTabsHistogram(const DataUseBuffer& data_use_buffer) {
  std::set<SessionID> unique_tabs;
  for (const auto& data_use_buffer_pair : data_use_buffer)
    unique_tabs.insert(data_use_buffer_pair.first->tab_id);
  UMA_HISTOGRAM_COUNTS_100("TrafficStatsAmortizer.ConcurrentTabs",
                           unique_tabs.size());
}

}  // namespace

TrafficStatsAmortizer::TrafficStatsAmortizer()
    : TrafficStatsAmortizer(
          base::DefaultTickClock::GetInstance(),
          std::unique_ptr<base::Timer>(new base::Timer(false, false)),
          GetTrafficStatsQueryDelay(),
          GetMaxAmortizationDelay(),
          GetMaxDataUseBufferSize()) {}

TrafficStatsAmortizer::~TrafficStatsAmortizer() {}

void TrafficStatsAmortizer::AmortizeDataUse(
    std::unique_ptr<DataUse> data_use,
    const AmortizationCompleteCallback& callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!callback.is_null());
  int64_t tx_bytes = data_use->tx_bytes, rx_bytes = data_use->rx_bytes;

  // As an optimization, combine consecutive buffered DataUse objects that are
  // identical except for byte counts and have the same callback.
  if (!buffered_data_use_.empty() &&
      buffered_data_use_.back().first->CanCombineWith(*data_use) &&
      buffered_data_use_.back().second.Equals(callback)) {
    buffered_data_use_.back().first->tx_bytes += data_use->tx_bytes;
    buffered_data_use_.back().first->rx_bytes += data_use->rx_bytes;
  } else {
    buffered_data_use_.push_back(
        std::pair<std::unique_ptr<DataUse>, AmortizationCompleteCallback>(
            std::move(data_use), callback));
  }

  AddPreAmortizationBytes(tx_bytes, rx_bytes);
}

void TrafficStatsAmortizer::OnExtraBytes(int64_t extra_tx_bytes,
                                         int64_t extra_rx_bytes) {
  DCHECK(thread_checker_.CalledOnValidThread());
  AddPreAmortizationBytes(extra_tx_bytes, extra_rx_bytes);
}

base::WeakPtr<TrafficStatsAmortizer> TrafficStatsAmortizer::GetWeakPtr() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return weak_ptr_factory_.GetWeakPtr();
}

TrafficStatsAmortizer::TrafficStatsAmortizer(
    const base::TickClock* tick_clock,
    std::unique_ptr<base::Timer> traffic_stats_query_timer,
    const base::TimeDelta& traffic_stats_query_delay,
    const base::TimeDelta& max_amortization_delay,
    size_t max_data_use_buffer_size)
    : tick_clock_(tick_clock),
      traffic_stats_query_timer_(std::move(traffic_stats_query_timer)),
      traffic_stats_query_delay_(traffic_stats_query_delay),
      max_amortization_delay_(max_amortization_delay),
      max_data_use_buffer_size_(max_data_use_buffer_size),
      is_amortization_in_progress_(false),
      are_last_amortization_traffic_stats_available_(false),
      last_amortization_traffic_stats_tx_bytes_(-1),
      last_amortization_traffic_stats_rx_bytes_(-1),
      pre_amortization_tx_bytes_(0),
      pre_amortization_rx_bytes_(0),
      weak_ptr_factory_(this) {}

bool TrafficStatsAmortizer::QueryTrafficStats(int64_t* tx_bytes,
                                              int64_t* rx_bytes) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return net::android::traffic_stats::GetCurrentUidTxBytes(tx_bytes) &&
         net::android::traffic_stats::GetCurrentUidRxBytes(rx_bytes);
}

void TrafficStatsAmortizer::AddPreAmortizationBytes(int64_t tx_bytes,
                                                    int64_t rx_bytes) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_GE(tx_bytes, 0);
  DCHECK_GE(rx_bytes, 0);
  base::TimeTicks now_ticks = tick_clock_->NowTicks();

  if (!is_amortization_in_progress_) {
    is_amortization_in_progress_ = true;
    current_amortization_run_start_time_ = now_ticks;
  }

  pre_amortization_tx_bytes_ += tx_bytes;
  pre_amortization_rx_bytes_ += rx_bytes;

  if (buffered_data_use_.size() > max_data_use_buffer_size_) {
    // Enforce a maximum limit on the size of |buffered_data_use_| to avoid
    // hogging memory. Note that this will likely cause the post-amortization
    // byte counts calculated here to be less accurate than if the amortizer
    // waited to perform amortization.
    traffic_stats_query_timer_->Stop();
    AmortizeNow();
    return;
  }

  // Cap any amortization delay to |max_amortization_delay_|. Note that if
  // |max_amortization_delay_| comes earlier, then this will likely cause the
  // post-amortization byte counts calculated here to be less accurate than if
  // the amortizer waited to perform amortization.
  base::TimeDelta query_delay = std::min(
      traffic_stats_query_delay_, current_amortization_run_start_time_ +
                                      max_amortization_delay_ - now_ticks);

  // Set the timer to query TrafficStats and amortize after a delay, so that
  // it's more likely that TrafficStats will be queried when the network is
  // idle. If the timer was already set, then this overrides the previous delay.
  traffic_stats_query_timer_->Start(
      FROM_HERE, query_delay,
      base::Bind(&TrafficStatsAmortizer::AmortizeNow, GetWeakPtr()));
}

void TrafficStatsAmortizer::AmortizeNow() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(is_amortization_in_progress_);

  if (!buffered_data_use_.empty()) {
    // Record histograms for the pre-amortization byte counts of the DataUse
    // objects.
    UMA_HISTOGRAM_COUNTS(
        "TrafficStatsAmortizer.PreAmortizationRunDataUseBytes.Tx",
        GetByteCountAsHistogramSample(GetTotalTxBytes(buffered_data_use_)));
    UMA_HISTOGRAM_COUNTS(
        "TrafficStatsAmortizer.PreAmortizationRunDataUseBytes.Rx",
        GetByteCountAsHistogramSample(GetTotalRxBytes(buffered_data_use_)));
  }

  int64_t current_traffic_stats_tx_bytes = -1;
  int64_t current_traffic_stats_rx_bytes = -1;
  bool are_current_traffic_stats_available = QueryTrafficStats(
      &current_traffic_stats_tx_bytes, &current_traffic_stats_rx_bytes);

  if (are_current_traffic_stats_available &&
      are_last_amortization_traffic_stats_available_ &&
      !buffered_data_use_.empty()) {
    // These TrafficStats byte counts are guaranteed to increase monotonically
    // since device boot.
    DCHECK_GE(current_traffic_stats_tx_bytes,
              last_amortization_traffic_stats_tx_bytes_);
    DCHECK_GE(current_traffic_stats_rx_bytes,
              last_amortization_traffic_stats_rx_bytes_);

    // Only attempt to amortize network overhead from TrafficStats if any of
    // those bytes are reflected in the pre-amortization byte totals. Otherwise,
    // that network overhead will be amortized in a later amortization run.
    if (pre_amortization_tx_bytes_ != 0) {
      AmortizeByteCountSequence(&buffered_data_use_, &GetTxBytes,
                                pre_amortization_tx_bytes_,
                                current_traffic_stats_tx_bytes -
                                    last_amortization_traffic_stats_tx_bytes_);
    }
    if (pre_amortization_rx_bytes_ != 0) {
      AmortizeByteCountSequence(&buffered_data_use_, &GetRxBytes,
                                pre_amortization_rx_bytes_,
                                current_traffic_stats_rx_bytes -
                                    last_amortization_traffic_stats_rx_bytes_);
    }
  }

  if (!buffered_data_use_.empty()) {
    // Record histograms for the post-amortization byte counts of the DataUse
    // objects.
    UMA_HISTOGRAM_COUNTS(
        "TrafficStatsAmortizer.PostAmortizationRunDataUseBytes.Tx",
        GetByteCountAsHistogramSample(GetTotalTxBytes(buffered_data_use_)));
    UMA_HISTOGRAM_COUNTS(
        "TrafficStatsAmortizer.PostAmortizationRunDataUseBytes.Rx",
        GetByteCountAsHistogramSample(GetTotalRxBytes(buffered_data_use_)));
    RecordConcurrentTabsHistogram(buffered_data_use_);
  }

  UMA_HISTOGRAM_TIMES(
      "TrafficStatsAmortizer.AmortizationDelay",
      tick_clock_->NowTicks() - current_amortization_run_start_time_);

  UMA_HISTOGRAM_COUNTS_1000("TrafficStatsAmortizer.BufferSizeOnFlush",
                            buffered_data_use_.size());

  // Reset state now that the amortization run has finished.
  is_amortization_in_progress_ = false;
  current_amortization_run_start_time_ = base::TimeTicks();

  // Don't update the previous amortization run's TrafficStats byte counts if
  // none of the bytes since then are reflected in the pre-amortization byte
  // totals. This way, the overhead that wasn't handled in this amortization run
  // can be handled in a later amortization run that actually has bytes in that
  // direction. This mitigates the problem of losing TrafficStats overhead bytes
  // on slow networks due to TrafficStats seeing the bytes much earlier than the
  // network stack reports them, or vice versa.
  if (!are_last_amortization_traffic_stats_available_ ||
      pre_amortization_tx_bytes_ != 0) {
    last_amortization_traffic_stats_tx_bytes_ = current_traffic_stats_tx_bytes;
  }
  if (!are_last_amortization_traffic_stats_available_ ||
      pre_amortization_rx_bytes_ != 0) {
    last_amortization_traffic_stats_rx_bytes_ = current_traffic_stats_rx_bytes;
  }

  are_last_amortization_traffic_stats_available_ =
      are_current_traffic_stats_available;

  pre_amortization_tx_bytes_ = 0;
  pre_amortization_rx_bytes_ = 0;

  DataUseBuffer data_use_sequence;
  data_use_sequence.swap(buffered_data_use_);

  // Pass post-amortization DataUse objects to their respective callbacks.
  for (auto& data_use_buffer_pair : data_use_sequence)
    data_use_buffer_pair.second.Run(std::move(data_use_buffer_pair.first));
}

}  // namespace android
}  // namespace data_usage
