// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_usage/core/data_use_aggregator.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "build/build_config.h"
#include "components/data_usage/core/data_use.h"
#include "net/base/load_timing_info.h"
#include "net/base/network_change_notifier.h"
#include "net/url_request/url_request.h"

#if defined(OS_ANDROID)
#include "net/android/network_library.h"
#endif  // OS_ANDROID

namespace data_usage {

DataUseAggregator::DataUseAggregator(
    std::unique_ptr<DataUseAnnotator> annotator,
    std::unique_ptr<DataUseAmortizer> amortizer)
    : annotator_(std::move(annotator)),
      amortizer_(std::move(amortizer)),
      connection_type_(net::NetworkChangeNotifier::GetConnectionType()),
      weak_ptr_factory_(this) {
#if defined(OS_ANDROID)
  mcc_mnc_ = net::android::GetTelephonySimOperator();
#endif  // OS_ANDROID
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

DataUseAggregator::~DataUseAggregator() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void DataUseAggregator::AddObserver(Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observer_list_.AddObserver(observer);
}

void DataUseAggregator::RemoveObserver(Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observer_list_.RemoveObserver(observer);
}

void DataUseAggregator::ReportDataUse(net::URLRequest* request,
                                      int64_t tx_bytes,
                                      int64_t rx_bytes) {
  DCHECK(thread_checker_.CalledOnValidThread());

  net::LoadTimingInfo load_timing_info;
  request->GetLoadTimingInfo(&load_timing_info);

  std::unique_ptr<DataUse> data_use(new DataUse(
      request->url(), load_timing_info.request_start,
      request->site_for_cookies(), /*tab_id=*/SessionID::InvalidValue(),
      connection_type_, mcc_mnc_, tx_bytes, rx_bytes));

  if (!annotator_) {
    PassDataUseToAmortizer(std::move(data_use));
    return;
  }

  // As an optimization, re-use a lazily initialized callback object for every
  // call into |annotator_|, so that a new callback object doesn't have to be
  // allocated and held onto every time.
  if (annotation_callback_.is_null()) {
    annotation_callback_ =
        base::Bind(&DataUseAggregator::PassDataUseToAmortizer, GetWeakPtr());
  }
  annotator_->Annotate(request, std::move(data_use), annotation_callback_);
}

void DataUseAggregator::ReportOffTheRecordDataUse(int64_t tx_bytes,
                                                  int64_t rx_bytes) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!amortizer_)
    return;

  amortizer_->OnExtraBytes(tx_bytes, rx_bytes);
}

base::WeakPtr<DataUseAggregator> DataUseAggregator::GetWeakPtr() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return weak_ptr_factory_.GetWeakPtr();
}

void DataUseAggregator::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  DCHECK(thread_checker_.CalledOnValidThread());

  connection_type_ = type;
#if defined(OS_ANDROID)
  mcc_mnc_ = net::android::GetTelephonySimOperator();
#endif  // OS_ANDROID
}

void DataUseAggregator::SetMccMncForTests(const std::string& mcc_mnc) {
  DCHECK(thread_checker_.CalledOnValidThread());
  mcc_mnc_ = mcc_mnc;
}

void DataUseAggregator::PassDataUseToAmortizer(
    std::unique_ptr<DataUse> data_use) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(data_use);

  if (!amortizer_) {
    OnAmortizationComplete(std::move(data_use));
    return;
  }

  // As an optimization, re-use a lazily initialized callback object for every
  // call into |amortizer_|, so that a new callback object doesn't have to be
  // allocated and held onto every time. This also allows the |amortizer_| to
  // combine together similar DataUse objects in its buffer if applicable.
  if (amortization_callback_.is_null()) {
    amortization_callback_ =
        base::Bind(&DataUseAggregator::OnAmortizationComplete, GetWeakPtr());
  }
  amortizer_->AmortizeDataUse(std::move(data_use), amortization_callback_);
}

void DataUseAggregator::OnAmortizationComplete(
    std::unique_ptr<DataUse> amortized_data_use) {
  DCHECK(thread_checker_.CalledOnValidThread());
  for (Observer& observer : observer_list_)
    observer.OnDataUse(*amortized_data_use);
}

}  // namespace data_usage
