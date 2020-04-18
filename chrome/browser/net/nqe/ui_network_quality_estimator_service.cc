// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/nqe/ui_network_quality_estimator_service.h"

#include <string>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/browser_thread.h"
#include "net/nqe/effective_connection_type_observer.h"
#include "net/nqe/network_qualities_prefs_manager.h"
#include "net/nqe/network_quality.h"
#include "net/nqe/rtt_throughput_estimates_observer.h"
#include "net/url_request/url_request_context.h"

namespace {

// PrefDelegateImpl writes the provided dictionary value to the network quality
// estimator prefs on the disk.
class PrefDelegateImpl
    : public net::NetworkQualitiesPrefsManager::PrefDelegate {
 public:
  // |pref_service| is used to read and write prefs from/to the disk.
  explicit PrefDelegateImpl(PrefService* pref_service)
      : pref_service_(pref_service), path_(prefs::kNetworkQualities) {
    DCHECK(pref_service_);
  }
  ~PrefDelegateImpl() override {}

  void SetDictionaryValue(const base::DictionaryValue& value) override {
    DCHECK(thread_checker_.CalledOnValidThread());

    pref_service_->Set(path_, value);
    UMA_HISTOGRAM_EXACT_LINEAR("NQE.Prefs.WriteCount", 1, 2);
  }

  std::unique_ptr<base::DictionaryValue> GetDictionaryValue() override {
    DCHECK(thread_checker_.CalledOnValidThread());
    UMA_HISTOGRAM_EXACT_LINEAR("NQE.Prefs.ReadCount", 1, 2);
    return pref_service_->GetDictionary(path_)->CreateDeepCopy();
  }

 private:
  PrefService* pref_service_;

  // |path_| is the location of the network quality estimator prefs.
  const std::string path_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(PrefDelegateImpl);
};

// Initializes |pref_manager| on |io_thread|.
void SetNQEOnIOThread(net::NetworkQualitiesPrefsManager* prefs_manager,
                      IOThread* io_thread) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // Avoid null pointer referencing during browser shutdown, or when the network
  // service is running out of process.
  if (!io_thread->globals()->system_request_context ||
      !io_thread->globals()
           ->system_request_context->network_quality_estimator()) {
    return;
  }

  prefs_manager->InitializeOnNetworkThread(
      io_thread->globals()
          ->system_request_context->network_quality_estimator());
}

}  // namespace

// A class that sets itself as an observer of the EffectiveconnectionType for
// the browser IO thread. It reports any change in EffectiveConnectionType back
// to the UI service.
// It is created on the UI thread, but used and deleted on the IO thread.
class UINetworkQualityEstimatorService::IONetworkQualityObserver
    : public net::EffectiveConnectionTypeObserver,
      public net::RTTAndThroughputEstimatesObserver {
 public:
  explicit IONetworkQualityObserver(
      base::WeakPtr<UINetworkQualityEstimatorService> service)
      : service_(service), network_quality_estimator_(nullptr) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  }

  ~IONetworkQualityObserver() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    if (network_quality_estimator_) {
      network_quality_estimator_->RemoveEffectiveConnectionTypeObserver(this);
      network_quality_estimator_->RemoveRTTAndThroughputEstimatesObserver(this);
    }
  }

  void InitializeOnIOThread(IOThread* io_thread) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

    if (!io_thread->globals()->system_request_context ||
        !io_thread->globals()
             ->system_request_context->network_quality_estimator()) {
      return;
    }
    network_quality_estimator_ =
        io_thread->globals()
            ->system_request_context->network_quality_estimator();
    if (!network_quality_estimator_)
      return;
    network_quality_estimator_->AddEffectiveConnectionTypeObserver(this);
    network_quality_estimator_->AddRTTAndThroughputEstimatesObserver(this);
  }

  // net::NetworkQualityEstimator::EffectiveConnectionTypeObserver
  // implementation:
  void OnEffectiveConnectionTypeChanged(
      net::EffectiveConnectionType type) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &UINetworkQualityEstimatorService::EffectiveConnectionTypeChanged,
            service_, type));
  }

  // net::NetworkQualityEstimator::RTTAndThroughputEstimatesObserver
  // implementation:
  void OnRTTOrThroughputEstimatesComputed(
      base::TimeDelta http_rtt,
      base::TimeDelta transport_rtt,
      int32_t downstream_throughput_kbps) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &UINetworkQualityEstimatorService::RTTOrThroughputComputed,
            service_, http_rtt, transport_rtt, downstream_throughput_kbps));
  }

 private:
  base::WeakPtr<UINetworkQualityEstimatorService> service_;
  net::NetworkQualityEstimator* network_quality_estimator_;

  DISALLOW_COPY_AND_ASSIGN(IONetworkQualityObserver);
};

UINetworkQualityEstimatorService::UINetworkQualityEstimatorService(
    Profile* profile)
    : type_(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN),
      http_rtt_(net::nqe::internal::InvalidRTT()),
      transport_rtt_(net::nqe::internal::InvalidRTT()),
      downstream_throughput_kbps_(net::nqe::internal::INVALID_RTT_THROUGHPUT),
      weak_factory_(this) {
  DCHECK(profile);
  // If this is running in a context without an IOThread, don't try to create
  // the IO object.
  if (!g_browser_process->io_thread())
    return;
  io_observer_ = base::WrapUnique(
      new IONetworkQualityObserver(weak_factory_.GetWeakPtr()));
  std::unique_ptr<PrefDelegateImpl> pref_delegate(
      new PrefDelegateImpl(profile->GetPrefs()));
  prefs_manager_ = base::WrapUnique(
      new net::NetworkQualitiesPrefsManager(std::move(pref_delegate)));

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&IONetworkQualityObserver::InitializeOnIOThread,
                     base::Unretained(io_observer_.get()),
                     g_browser_process->io_thread()));
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&SetNQEOnIOThread, prefs_manager_.get(),
                     g_browser_process->io_thread()));
}

UINetworkQualityEstimatorService::~UINetworkQualityEstimatorService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

void UINetworkQualityEstimatorService::Shutdown() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  weak_factory_.InvalidateWeakPtrs();
  if (io_observer_) {
    bool deleted = content::BrowserThread::DeleteSoon(
        content::BrowserThread::IO, FROM_HERE, io_observer_.release());
    DCHECK(deleted);
    // Silence unused variable warning in release builds.
    (void)deleted;
  }
  if (prefs_manager_) {
    prefs_manager_->ShutdownOnPrefSequence();
    bool deleted = content::BrowserThread::DeleteSoon(
        content::BrowserThread::IO, FROM_HERE, prefs_manager_.release());
    DCHECK(deleted);
    // Silence unused variable warning in release builds.
    (void)deleted;
  }
}

void UINetworkQualityEstimatorService::EffectiveConnectionTypeChanged(
    net::EffectiveConnectionType type) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  type_ = type;
  for (auto& observer : effective_connection_type_observer_list_)
    observer.OnEffectiveConnectionTypeChanged(type);
}

void UINetworkQualityEstimatorService::RTTOrThroughputComputed(
    base::TimeDelta http_rtt,
    base::TimeDelta transport_rtt,
    int32_t downstream_throughput_kbps) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  http_rtt_ = http_rtt;
  transport_rtt_ = transport_rtt;
  downstream_throughput_kbps_ = downstream_throughput_kbps;

  for (auto& observer : rtt_throughput_observer_list_) {
    observer.OnRTTOrThroughputEstimatesComputed(http_rtt, transport_rtt,
                                                downstream_throughput_kbps);
  }
}

void UINetworkQualityEstimatorService::AddEffectiveConnectionTypeObserver(
    net::EffectiveConnectionTypeObserver* observer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  effective_connection_type_observer_list_.AddObserver(observer);

  // Notify the |observer| on the next message pump since |observer| may not
  // be completely set up for receiving the callbacks.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UINetworkQualityEstimatorService::
                         NotifyEffectiveConnectionTypeObserverIfPresent,
                     weak_factory_.GetWeakPtr(), observer));
}

void UINetworkQualityEstimatorService::RemoveEffectiveConnectionTypeObserver(
    net::EffectiveConnectionTypeObserver* observer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  effective_connection_type_observer_list_.RemoveObserver(observer);
}

base::Optional<base::TimeDelta> UINetworkQualityEstimatorService::GetHttpRTT()
    const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (http_rtt_ == net::nqe::internal::InvalidRTT())
    return base::Optional<base::TimeDelta>();
  return http_rtt_;
}

base::Optional<base::TimeDelta>
UINetworkQualityEstimatorService::GetTransportRTT() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (transport_rtt_ == net::nqe::internal::InvalidRTT())
    return base::Optional<base::TimeDelta>();
  return transport_rtt_;
}

base::Optional<int32_t>
UINetworkQualityEstimatorService::GetDownstreamThroughputKbps() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (downstream_throughput_kbps_ == net::nqe::internal::INVALID_RTT_THROUGHPUT)
    return base::Optional<int32_t>();
  return downstream_throughput_kbps_;
}

void UINetworkQualityEstimatorService::AddRTTAndThroughputEstimatesObserver(
    net::RTTAndThroughputEstimatesObserver* observer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  rtt_throughput_observer_list_.AddObserver(observer);

  // Notify the |observer| on the next message pump since |observer| may not
  // be completely set up for receiving the callbacks.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UINetworkQualityEstimatorService::
                         NotifyRTTAndThroughputObserverIfPresent,
                     weak_factory_.GetWeakPtr(), observer));
}

// Removes |observer| from the list of RTT and throughput estimate observers.
// Must be called on the IO thread.
void UINetworkQualityEstimatorService::RemoveRTTAndThroughputEstimatesObserver(
    net::RTTAndThroughputEstimatesObserver* observer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  rtt_throughput_observer_list_.RemoveObserver(observer);
}

void UINetworkQualityEstimatorService::SetEffectiveConnectionTypeForTesting(
    net::EffectiveConnectionType type) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  EffectiveConnectionTypeChanged(type);
}

net::EffectiveConnectionType
UINetworkQualityEstimatorService::GetEffectiveConnectionType() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return type_;
}

void UINetworkQualityEstimatorService::ClearPrefs() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!prefs_manager_)
    return;
  prefs_manager_->ClearPrefs();
}

void UINetworkQualityEstimatorService::
    NotifyEffectiveConnectionTypeObserverIfPresent(
        net::EffectiveConnectionTypeObserver* observer) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!effective_connection_type_observer_list_.HasObserver(observer))
    return;
  if (type_ == net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN)
    return;
  observer->OnEffectiveConnectionTypeChanged(type_);
}

void UINetworkQualityEstimatorService::NotifyRTTAndThroughputObserverIfPresent(
    net::RTTAndThroughputEstimatesObserver* observer) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!rtt_throughput_observer_list_.HasObserver(observer))
    return;
  observer->OnRTTOrThroughputEstimatesComputed(http_rtt_, transport_rtt_,
                                               downstream_throughput_kbps_);
}

// static
void UINetworkQualityEstimatorService::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kNetworkQualities,
                                   PrefRegistry::LOSSY_PREF);
}

std::map<net::nqe::internal::NetworkID,
         net::nqe::internal::CachedNetworkQuality>
UINetworkQualityEstimatorService::ForceReadPrefsForTesting() const {
  if (!prefs_manager_) {
    return std::map<net::nqe::internal::NetworkID,
                    net::nqe::internal::CachedNetworkQuality>();
  }
  return prefs_manager_->ForceReadPrefsForTesting();
}
