// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/stale_host_resolver.h"

#include "base/callback_helpers.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "net/base/net_errors.h"
#include "net/dns/dns_util.h"
#include "net/dns/host_resolver_impl.h"

namespace cronet {

namespace {

// Used in histograms; do not modify existing values.
enum RequestOutcome {
  // Served from (valid) cache, hosts file, IP literal, etc.
  SYNCHRONOUS = 0,

  // Network responded; there was no usable stale data.
  NETWORK_WITHOUT_STALE = 1,

  // Network responded before stale delay; there was usable stale data.
  NETWORK_WITH_STALE = 2,

  // Stale data returned; network didn't respond before the stale delay.
  STALE_BEFORE_NETWORK = 3,

  // Request canceled; there was no usable stale data.
  CANCELED_WITHOUT_STALE = 4,

  // Request canceled; there was usable stale data.
  CANCELED_WITH_STALE = 5,

  MAX_REQUEST_OUTCOME
};

void RecordRequestOutcome(RequestOutcome outcome) {
  UMA_HISTOGRAM_ENUMERATION("DNS.StaleHostResolver.RequestOutcome", outcome,
                            MAX_REQUEST_OUTCOME);
}

void RecordCacheSizes(size_t restored, size_t current) {
  UMA_HISTOGRAM_COUNTS_1000("DNS.StaleHostResolver.RestoreSizeOnCacheMiss",
                            restored);
  UMA_HISTOGRAM_COUNTS_1000("DNS.StaleHostResolver.SizeOnCacheMiss", current);
}

void RecordAddressListDelta(net::AddressListDeltaType delta) {
  UMA_HISTOGRAM_ENUMERATION("DNS.StaleHostResolver.StaleAddressListDelta",
                            delta, net::MAX_DELTA_TYPE);
}

void RecordTimeDelta(base::TimeTicks network_time, base::TimeTicks stale_time) {
  if (network_time <= stale_time) {
    UMA_HISTOGRAM_LONG_TIMES_100("DNS.StaleHostResolver.NetworkEarly",
                                 stale_time - network_time);
  } else {
    UMA_HISTOGRAM_LONG_TIMES_100("DNS.StaleHostResolver.NetworkLate",
                                 network_time - stale_time);
  }
}

bool StaleEntryIsUsable(const StaleHostResolver::StaleOptions& options,
                        const net::HostCache::EntryStaleness& entry) {
  if (options.max_expired_time != base::TimeDelta() &&
      entry.expired_by > options.max_expired_time) {
    return false;
  }
  if (options.max_stale_uses > 0 && entry.stale_hits > options.max_stale_uses)
    return false;
  if (!options.allow_other_network && entry.network_changes > 0)
    return false;
  return true;
}

}  // namespace

// A request made by the StaleHostResolver. May return fresh cached data,
// network data, or stale cached data.
class StaleHostResolver::RequestImpl {
 public:
  RequestImpl();
  ~RequestImpl();

  // A callback for the caller to decide whether a stale entry is usable or not.
  typedef base::Callback<bool(const net::HostCache::EntryStaleness&)>
      StaleEntryUsableCallback;

  // Starts the request. May call |usable_callback| inline if |resolver| returns
  // stale data to let the caller decide whether the data is usable.
  //
  // Returns the result if the request finishes synchronously. Returns
  // ERR_IO_PENDING and calls |result_callback| with the result if it finishes
  // asynchronously (unless destroyed first).
  //
  // |addresses| must remain valid until the Request completes (synchronously or
  // via |result_callback|) or is canceled by destroying the Request.
  int Start(net::HostResolverImpl* resolver,
            const RequestInfo& info,
            net::RequestPriority priority,
            net::AddressList* addresses,
            const net::CompletionCallback& result_callback,
            std::unique_ptr<net::HostResolver::Request>* out_req,
            const net::NetLogWithSource& net_log,
            const StaleEntryUsableCallback& usable_callback,
            base::TimeDelta stale_delay);

  void ChangeRequestPriority(net::RequestPriority priority);

 private:
  class Handle : public net::HostResolver::Request {
   public:
    Handle(RequestImpl* request) : request_(request) {}
    ~Handle() override { request_->OnHandleDestroyed(); }

    void ChangeRequestPriority(net::RequestPriority priority) override {
      request_->ChangeRequestPriority(priority);
    }

   private:
    RequestImpl* request_;
  };

  bool have_network_request() const { return network_request_ != nullptr; }
  bool have_stale_data() const {
    return stale_error_ != net::ERR_DNS_CACHE_MISS;
  }
  bool have_returned() const { return result_callback_.is_null(); }
  bool have_handle() const { return handle_ != nullptr; }

  // Callback for |stale_timer_| that returns stale results.
  void OnStaleDelayElapsed();
  // Callback for network request that returns fresh results if the request
  // hasn't already returned stale results, and completes the request.
  void OnNetworkRequestComplete(int error);
  void OnHandleDestroyed();

  // Fills |*result_addresses_| if rv is OK and returns rv.
  int HandleResult(int rv, const net::AddressList& addresses);
  // Fills |*result_addresses_| if rv is OK and calls |result_callback_| with
  // rv.
  void ReturnResult(int rv, const net::AddressList& addresses);

  void MaybeDeleteThis();

  void RecordSynchronousRequest();
  void RecordNetworkRequest(int error, bool returned_stale_data);
  void RecordCanceledRequest();

  // The address list passed into |Start()| to be filled in when the request
  // returns.
  net::AddressList* result_addresses_;
  // The callback passed into |Start()| to be called when the request returns.
  net::CompletionCallback result_callback_;
  // Set when |result_callback_| is being called so |OnHandleDestroyed()|
  // doesn't delete the request.
  bool returning_result_;

  // The error from the stale cache entry, if there was one.
  // If not, net::ERR_DNS_CACHE_MISS.
  int stale_error_;
  // The address list from the stale cache entry, if there was one.
  net::AddressList stale_addresses_;
  // A timer that fires when the |Request| should return stale results, if the
  // underlying network request has not finished yet.
  base::OneShotTimer stale_timer_;

  // The address list the underlying network request will fill in. (Can't be the
  // one passed to |Start()|, or else the network request would overwrite stale
  // results after they are returned.)
  net::AddressList network_addresses_;
  // The underlying network request, so the priority can be changed.
  std::unique_ptr<net::HostResolver::Request> network_request_;

  // Statistics used in histograms:
  // Number of HostCache entries that were restored from prefs, recorded at the
  // time the cache was checked.
  size_t restore_size_;
  // Current HostCache size at the time the cache was checked.
  size_t current_size_;

  // Handle that caller can use to cancel the request before it returns.
  // Owned by the caller; cleared via |OnHandleDestroyed()| when destroyed.
  Handle* handle_;
};

StaleHostResolver::RequestImpl::RequestImpl()
    : result_addresses_(nullptr),
      returning_result_(false),
      stale_error_(net::ERR_DNS_CACHE_MISS),
      restore_size_(0),
      current_size_(0),
      handle_(nullptr) {}

StaleHostResolver::RequestImpl::~RequestImpl() {}

int StaleHostResolver::RequestImpl::Start(
    net::HostResolverImpl* resolver,
    const RequestInfo& info,
    net::RequestPriority priority,
    net::AddressList* addresses,
    const net::CompletionCallback& result_callback,
    std::unique_ptr<net::HostResolver::Request>* out_req,
    const net::NetLogWithSource& net_log,
    const StaleEntryUsableCallback& usable_callback,
    base::TimeDelta stale_delay) {
  DCHECK(resolver);
  DCHECK(addresses);
  DCHECK(!result_callback.is_null());
  DCHECK(out_req);
  DCHECK(!usable_callback.is_null());

  result_addresses_ = addresses;
  restore_size_ = resolver->LastRestoredCacheSize();
  current_size_ = resolver->CacheSize();

  net::AddressList cache_addresses;
  net::HostCache::EntryStaleness stale_info;
  int cache_rv = resolver->ResolveStaleFromCache(info, &cache_addresses,
                                                 &stale_info, net_log);
  // If it's a fresh cache hit (or literal), return it synchronously.
  if (cache_rv != net::ERR_DNS_CACHE_MISS && !stale_info.is_stale()) {
    RecordSynchronousRequest();
    return HandleResult(cache_rv, cache_addresses);
  }

  result_callback_ = result_callback;
  handle_ = new Handle(this);
  *out_req = std::unique_ptr<net::HostResolver::Request>(handle_);

  if (cache_rv == net::OK && usable_callback.Run(stale_info)) {
    stale_error_ = cache_rv;
    stale_addresses_ = cache_addresses;
    // |stale_timer_| is deleted when the Request is deleted, so it's safe to
    // use Unretained here.
    base::Callback<void()> stale_callback =
        base::Bind(&StaleHostResolver::RequestImpl::OnStaleDelayElapsed,
                   base::Unretained(this));
    stale_timer_.Start(FROM_HERE, stale_delay, stale_callback);
  }

  // Don't check the cache again.
  net::HostResolver::RequestInfo no_cache_info(info);
  no_cache_info.set_allow_cached_response(false);
  int network_rv = resolver->Resolve(
      no_cache_info, priority, &network_addresses_,
      base::Bind(&StaleHostResolver::RequestImpl::OnNetworkRequestComplete,
                 base::Unretained(this)),
      &network_request_, net_log);
  // Network resolver has returned synchronously (for example by resolving from
  // /etc/hosts).
  if (network_rv != net::ERR_IO_PENDING) {
    RecordSynchronousRequest();
    return HandleResult(network_rv, network_addresses_);
  }
  return network_rv;
}

void StaleHostResolver::RequestImpl::ChangeRequestPriority(
    net::RequestPriority priority) {
  DCHECK(have_network_request());

  network_request_->ChangeRequestPriority(priority);
}

void StaleHostResolver::RequestImpl::OnStaleDelayElapsed() {
  DCHECK(!have_returned());
  DCHECK(have_stale_data());

  ReturnResult(stale_error_, stale_addresses_);

  // The request needs to wait for both the network request to complete (to
  // backfill the cache) and the caller to delete the handle before deleting
  // itself.
  DCHECK(have_network_request());
}

void StaleHostResolver::RequestImpl::OnNetworkRequestComplete(int error) {
  DCHECK(have_network_request());
  network_request_.reset();

  RecordNetworkRequest(error, /* returned_stale_data = */ have_returned());

  if (!have_returned()) {
    if (have_stale_data())
      stale_timer_.Stop();
    ReturnResult(error, network_addresses_);
  }

  if (!have_handle())
    delete this;
}

void StaleHostResolver::RequestImpl::OnHandleDestroyed() {
  DCHECK(have_handle());
  handle_ = nullptr;

  // If the caller deletes the handle *before* the request has returned, treat
  // it as a cancel.
  if (!have_returned()) {
    network_request_.reset();
    result_callback_ = net::CompletionCallback();
    RecordCanceledRequest();
  }

  if (!returning_result_ && !have_handle() && !have_network_request())
    delete this;
}

int StaleHostResolver::RequestImpl::HandleResult(
    int rv,
    const net::AddressList& addresses) {
  DCHECK(result_addresses_);

  if (rv == net::OK)
    *result_addresses_ = addresses;
  result_addresses_ = nullptr;
  return rv;
}

void StaleHostResolver::RequestImpl::ReturnResult(
    int rv,
    const net::AddressList& addresses) {
  DCHECK(result_callback_);
  returning_result_ = true;
  base::ResetAndReturn(&result_callback_).Run(HandleResult(rv, addresses));
  returning_result_ = false;
}

void StaleHostResolver::RequestImpl::RecordSynchronousRequest() {
  RecordRequestOutcome(SYNCHRONOUS);
}

void StaleHostResolver::RequestImpl::RecordNetworkRequest(
    int error,
    bool returned_stale_data) {
  if (have_stale_data())
    RecordTimeDelta(base::TimeTicks::Now(), stale_timer_.desired_run_time());

  if (returned_stale_data && stale_error_ == net::OK && error == net::OK) {
    RecordAddressListDelta(
        FindAddressListDeltaType(stale_addresses_, network_addresses_));
  }

  if (returned_stale_data) {
    RecordRequestOutcome(STALE_BEFORE_NETWORK);
  } else if (have_stale_data()) {
    RecordRequestOutcome(NETWORK_WITH_STALE);
    RecordCacheSizes(restore_size_, current_size_);
  } else {
    RecordRequestOutcome(NETWORK_WITHOUT_STALE);
  }
}

void StaleHostResolver::RequestImpl::RecordCanceledRequest() {
  if (have_stale_data())
    RecordRequestOutcome(CANCELED_WITH_STALE);
  else
    RecordRequestOutcome(CANCELED_WITHOUT_STALE);
}

StaleHostResolver::StaleOptions::StaleOptions()
    : allow_other_network(false), max_stale_uses(0) {}

StaleHostResolver::StaleHostResolver(
    std::unique_ptr<net::HostResolverImpl> inner_resolver,
    const StaleOptions& stale_options)
    : inner_resolver_(std::move(inner_resolver)), options_(stale_options) {
  DCHECK_LE(0, stale_options.max_expired_time.InMicroseconds());
  DCHECK_LE(0, stale_options.max_stale_uses);
}

StaleHostResolver::~StaleHostResolver() {}

int StaleHostResolver::Resolve(const RequestInfo& info,
                               net::RequestPriority priority,
                               net::AddressList* addresses,
                               const net::CompletionCallback& callback,
                               std::unique_ptr<Request>* out_req,
                               const net::NetLogWithSource& net_log) {
  StaleHostResolver::RequestImpl::StaleEntryUsableCallback usable_callback =
      base::Bind(&StaleEntryIsUsable, options_);

  RequestImpl* request = new RequestImpl();
  int rv =
      request->Start(inner_resolver_.get(), info, priority, addresses, callback,
                     out_req, net_log, usable_callback, options_.delay);
  if (rv != net::ERR_IO_PENDING)
    delete request;

  return rv;
}

int StaleHostResolver::ResolveFromCache(const RequestInfo& info,
                                        net::AddressList* addresses,
                                        const net::NetLogWithSource& net_log) {
  return inner_resolver_->ResolveFromCache(info, addresses, net_log);
}

int StaleHostResolver::ResolveStaleFromCache(
    const RequestInfo& info,
    net::AddressList* addresses,
    net::HostCache::EntryStaleness* stale_info,
    const net::NetLogWithSource& net_log) {
  return inner_resolver_->ResolveStaleFromCache(info, addresses, stale_info,
                                                net_log);
}

bool StaleHostResolver::HasCached(
    base::StringPiece hostname,
    net::HostCache::Entry::Source* source_out,
    net::HostCache::EntryStaleness* stale_out) const {
  return inner_resolver_->HasCached(hostname, source_out, stale_out);
}

void StaleHostResolver::SetDnsClientEnabled(bool enabled) {
  inner_resolver_->SetDnsClientEnabled(enabled);
}

net::HostCache* StaleHostResolver::GetHostCache() {
  return inner_resolver_->GetHostCache();
}

std::unique_ptr<base::Value> StaleHostResolver::GetDnsConfigAsValue() const {
  return inner_resolver_->GetDnsConfigAsValue();
}

}  // namespace net
