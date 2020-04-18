// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/host_resolver_impl.h"

#include <string>
#include <tuple>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/base/address_list.h"
#include "net/base/ip_address.h"
#include "net/base/mock_network_change_notifier.h"
#include "net/base/net_errors.h"
#include "net/dns/dns_client.h"
#include "net/dns/dns_test_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/log/test_net_log.h"
#include "net/test/gtest_util.h"
#include "net/test/test_with_scoped_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::test::IsError;
using net::test::IsOk;
using ::testing::NotNull;

namespace net {

namespace {

const size_t kMaxJobs = 10u;
const size_t kMaxRetryAttempts = 4u;

HostResolver::Options DefaultOptions() {
  HostResolver::Options options;
  options.max_concurrent_resolves = kMaxJobs;
  options.max_retry_attempts = kMaxRetryAttempts;
  options.enable_caching = true;
  return options;
}

HostResolverImpl::ProcTaskParams DefaultParams(
    HostResolverProc* resolver_proc) {
  return HostResolverImpl::ProcTaskParams(resolver_proc, kMaxRetryAttempts);
}

// A HostResolverProc that pushes each host mapped into a list and allows
// waiting for a specific number of requests. Unlike RuleBasedHostResolverProc
// it never calls SystemHostResolverCall. By default resolves all hostnames to
// "127.0.0.1". After AddRule(), it resolves only names explicitly specified.
class MockHostResolverProc : public HostResolverProc {
 public:
  struct ResolveKey {
    ResolveKey(const std::string& hostname, AddressFamily address_family)
        : hostname(hostname), address_family(address_family) {}
    bool operator<(const ResolveKey& other) const {
      return std::tie(address_family, hostname) <
             std::tie(other.address_family, other.hostname);
    }
    std::string hostname;
    AddressFamily address_family;
  };

  typedef std::vector<ResolveKey> CaptureList;

  MockHostResolverProc()
      : HostResolverProc(NULL),
        num_requests_waiting_(0),
        num_slots_available_(0),
        requests_waiting_(&lock_),
        slots_available_(&lock_) {
  }

  // Waits until |count| calls to |Resolve| are blocked. Returns false when
  // timed out.
  bool WaitFor(unsigned count) {
    base::AutoLock lock(lock_);
    base::Time start_time = base::Time::Now();
    while (num_requests_waiting_ < count) {
      requests_waiting_.TimedWait(TestTimeouts::action_timeout());
      if (base::Time::Now() > start_time + TestTimeouts::action_timeout())
        return false;
    }
    return true;
  }

  // Signals |count| waiting calls to |Resolve|. First come first served.
  void SignalMultiple(unsigned count) {
    base::AutoLock lock(lock_);
    num_slots_available_ += count;
    slots_available_.Broadcast();
  }

  // Signals all waiting calls to |Resolve|. Beware of races.
  void SignalAll() {
    base::AutoLock lock(lock_);
    num_slots_available_ = num_requests_waiting_;
    slots_available_.Broadcast();
  }

  void AddRule(const std::string& hostname, AddressFamily family,
               const AddressList& result) {
    base::AutoLock lock(lock_);
    rules_[ResolveKey(hostname, family)] = result;
  }

  void AddRule(const std::string& hostname, AddressFamily family,
               const std::string& ip_list) {
    AddressList result;
    int rv = ParseAddressList(ip_list, std::string(), &result);
    DCHECK_EQ(OK, rv);
    AddRule(hostname, family, result);
  }

  void AddRuleForAllFamilies(const std::string& hostname,
                             const std::string& ip_list) {
    AddressList result;
    int rv = ParseAddressList(ip_list, std::string(), &result);
    DCHECK_EQ(OK, rv);
    AddRule(hostname, ADDRESS_FAMILY_UNSPECIFIED, result);
    AddRule(hostname, ADDRESS_FAMILY_IPV4, result);
    AddRule(hostname, ADDRESS_FAMILY_IPV6, result);
  }

  int Resolve(const std::string& hostname,
              AddressFamily address_family,
              HostResolverFlags host_resolver_flags,
              AddressList* addrlist,
              int* os_error) override {
    base::AutoLock lock(lock_);
    capture_list_.push_back(ResolveKey(hostname, address_family));
    ++num_requests_waiting_;
    requests_waiting_.Broadcast();
    {
      base::ScopedAllowBaseSyncPrimitivesForTesting
          scoped_allow_base_sync_primitives;
      while (!num_slots_available_)
        slots_available_.Wait();
    }
    DCHECK_GT(num_requests_waiting_, 0u);
    --num_slots_available_;
    --num_requests_waiting_;
    if (rules_.empty()) {
      int rv = ParseAddressList("127.0.0.1", std::string(), addrlist);
      DCHECK_EQ(OK, rv);
      return OK;
    }
    ResolveKey key(hostname, address_family);
    if (rules_.count(key) == 0)
      return ERR_NAME_NOT_RESOLVED;
    *addrlist = rules_[key];
    return OK;
  }

  CaptureList GetCaptureList() const {
    CaptureList copy;
    {
      base::AutoLock lock(lock_);
      copy = capture_list_;
    }
    return copy;
  }

  bool HasBlockedRequests() const {
    base::AutoLock lock(lock_);
    return num_requests_waiting_ > num_slots_available_;
  }

 protected:
  ~MockHostResolverProc() override = default;

 private:
  mutable base::Lock lock_;
  std::map<ResolveKey, AddressList> rules_;
  CaptureList capture_list_;
  unsigned num_requests_waiting_;
  unsigned num_slots_available_;
  base::ConditionVariable requests_waiting_;
  base::ConditionVariable slots_available_;

  DISALLOW_COPY_AND_ASSIGN(MockHostResolverProc);
};

bool AddressListContains(const AddressList& list,
                         const std::string& address,
                         uint16_t port) {
  IPAddress ip;
  bool rv = ip.AssignFromIPLiteral(address);
  DCHECK(rv);
  return base::ContainsValue(list, IPEndPoint(ip, port));
}

// A wrapper for requests to a HostResolver.
class Request {
 public:
  // Base class of handlers to be executed on completion of requests.
  struct Handler {
    virtual ~Handler() = default;
    virtual void Handle(Request* request) = 0;
  };

  Request(const HostResolver::RequestInfo& info,
          RequestPriority priority,
          size_t index,
          HostResolverImpl* resolver,
          Handler* handler)
      : info_(info),
        priority_(priority),
        index_(index),
        resolver_(resolver),
        handler_(handler),
        quit_on_complete_(false),
        result_(ERR_UNEXPECTED) {}

  int Resolve() {
    DCHECK(resolver_);
    DCHECK(!request_);
    list_ = AddressList();
    result_ = resolver_->Resolve(
        info_, priority_, &list_,
        base::Bind(&Request::OnComplete, base::Unretained(this)), &request_,
        NetLogWithSource());
    if (!list_.empty())
      EXPECT_THAT(result_, IsOk());
    return result_;
  }

  int ResolveFromCache() {
    DCHECK(resolver_);
    DCHECK(!request_);
    return resolver_->ResolveFromCache(info_, &list_, NetLogWithSource());
  }

  int ResolveStaleFromCache() {
    DCHECK(resolver_);
    DCHECK(!request_);
    return resolver_->ResolveStaleFromCache(info_, &list_, &staleness_,
                                            NetLogWithSource());
  }

  void ChangePriority(RequestPriority priority) {
    DCHECK(resolver_);
    DCHECK(request_);
    request_->ChangeRequestPriority(priority);
    priority_ = priority;
  }

  void Cancel() {
    DCHECK(resolver_);
    DCHECK(request_);
    request_.reset();
  }

  const HostResolver::RequestInfo& info() const { return info_; }
  size_t index() const { return index_; }
  const AddressList& list() const { return list_; }
  int result() const { return result_; }
  const HostCache::EntryStaleness staleness() const { return staleness_; }
  bool completed() const { return result_ != ERR_IO_PENDING; }
  bool pending() const { return request_ != nullptr; }

  bool HasAddress(const std::string& address, uint16_t port) const {
    return AddressListContains(list_, address, port);
  }

  // Returns the number of addresses in |list_|.
  unsigned NumberOfAddresses() const {
    return list_.size();
  }

  bool HasOneAddress(const std::string& address, uint16_t port) const {
    return HasAddress(address, port) && (NumberOfAddresses() == 1u);
  }

  // Returns ERR_UNEXPECTED if timed out.
  int WaitForResult() {
    if (completed())
      return result_;
    base::CancelableClosure closure(
        base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, closure.callback(), TestTimeouts::action_max_timeout());
    quit_on_complete_ = true;
    base::RunLoop().Run();
    bool did_quit = !quit_on_complete_;
    quit_on_complete_ = false;
    closure.Cancel();
    if (did_quit)
      return result_;
    else
      return ERR_UNEXPECTED;
  }

 private:
  void OnComplete(int rv) {
    EXPECT_TRUE(pending());
    EXPECT_THAT(result_, IsError(ERR_IO_PENDING));
    EXPECT_NE(ERR_IO_PENDING, rv);
    result_ = rv;
    request_.reset();
    if (!list_.empty()) {
      EXPECT_THAT(result_, IsOk());
      EXPECT_EQ(info_.port(), list_.front().port());
    }
    if (handler_)
      handler_->Handle(this);
    if (quit_on_complete_) {
      base::RunLoop::QuitCurrentWhenIdleDeprecated();
      quit_on_complete_ = false;
    }
  }

  HostResolver::RequestInfo info_;
  RequestPriority priority_;
  size_t index_;
  HostResolverImpl* resolver_;
  Handler* handler_;
  bool quit_on_complete_;

  AddressList list_;
  int result_;
  std::unique_ptr<HostResolver::Request> request_;
  HostCache::EntryStaleness staleness_;

  DISALLOW_COPY_AND_ASSIGN(Request);
};

// Using LookupAttemptHostResolverProc simulate very long lookups, and control
// which attempt resolves the host.
class LookupAttemptHostResolverProc : public HostResolverProc {
 public:
  LookupAttemptHostResolverProc(HostResolverProc* previous,
                                int attempt_number_to_resolve,
                                int total_attempts)
      : HostResolverProc(previous),
        attempt_number_to_resolve_(attempt_number_to_resolve),
        current_attempt_number_(0),
        total_attempts_(total_attempts),
        total_attempts_resolved_(0),
        resolved_attempt_number_(0),
        all_done_(&lock_) {
  }

  // Test harness will wait for all attempts to finish before checking the
  // results.
  void WaitForAllAttemptsToFinish(const base::TimeDelta& wait_time) {
    base::TimeTicks end_time = base::TimeTicks::Now() + wait_time;
    {
      base::AutoLock auto_lock(lock_);
      while (total_attempts_resolved_ != total_attempts_ &&
          base::TimeTicks::Now() < end_time) {
        all_done_.TimedWait(end_time - base::TimeTicks::Now());
      }
    }
  }

  // All attempts will wait for an attempt to resolve the host.
  void WaitForAnAttemptToComplete() {
    base::TimeDelta wait_time = base::TimeDelta::FromSeconds(60);
    base::TimeTicks end_time = base::TimeTicks::Now() + wait_time;
    {
      base::AutoLock auto_lock(lock_);
      base::ScopedAllowBaseSyncPrimitivesForTesting
          scoped_allow_base_sync_primitives;
      while (resolved_attempt_number_ == 0 && base::TimeTicks::Now() < end_time)
        all_done_.TimedWait(end_time - base::TimeTicks::Now());
    }
    all_done_.Broadcast();  // Tell all waiting attempts to proceed.
  }

  // Returns the number of attempts that have finished the Resolve() method.
  int total_attempts_resolved() { return total_attempts_resolved_; }

  // Returns the first attempt that that has resolved the host.
  int resolved_attempt_number() { return resolved_attempt_number_; }

  // HostResolverProc methods.
  int Resolve(const std::string& host,
              AddressFamily address_family,
              HostResolverFlags host_resolver_flags,
              AddressList* addrlist,
              int* os_error) override {
    bool wait_for_right_attempt_to_complete = true;
    {
      base::AutoLock auto_lock(lock_);
      ++current_attempt_number_;
      if (current_attempt_number_ == attempt_number_to_resolve_) {
        resolved_attempt_number_ = current_attempt_number_;
        wait_for_right_attempt_to_complete = false;
      }
    }

    if (wait_for_right_attempt_to_complete)
      // Wait for the attempt_number_to_resolve_ attempt to resolve.
      WaitForAnAttemptToComplete();

    int result = ResolveUsingPrevious(host, address_family, host_resolver_flags,
                                      addrlist, os_error);

    {
      base::AutoLock auto_lock(lock_);
      ++total_attempts_resolved_;
    }

    all_done_.Broadcast();  // Tell all attempts to proceed.

    // Since any negative number is considered a network error, with -1 having
    // special meaning (ERR_IO_PENDING). We could return the attempt that has
    // resolved the host as a negative number. For example, if attempt number 3
    // resolves the host, then this method returns -4.
    if (result == OK)
      return -1 - resolved_attempt_number_;
    else
      return result;
  }

 protected:
  ~LookupAttemptHostResolverProc() override = default;

 private:
  int attempt_number_to_resolve_;
  int current_attempt_number_;  // Incremented whenever Resolve is called.
  int total_attempts_;
  int total_attempts_resolved_;
  int resolved_attempt_number_;

  // All attempts wait for right attempt to be resolve.
  base::Lock lock_;
  base::ConditionVariable all_done_;
};

// TestHostResolverImpl's sole purpose is to mock the IPv6 reachability test.
// By default, this pretends that IPv6 is globally reachable.
// This class is necessary so unit tests run the same on dual-stack machines as
// well as IPv4 only machines.
class TestHostResolverImpl : public HostResolverImpl {
 public:
  TestHostResolverImpl(const Options& options, NetLog* net_log)
      : TestHostResolverImpl(options, net_log, true) {}

  TestHostResolverImpl(const Options& options,
                       NetLog* net_log,
                       bool ipv6_reachable)
      : HostResolverImpl(options, net_log), ipv6_reachable_(ipv6_reachable) {}

  ~TestHostResolverImpl() override = default;

 private:
  const bool ipv6_reachable_;

  bool IsGloballyReachable(const IPAddress& dest,
                           const NetLogWithSource& net_log) override {
    return ipv6_reachable_;
  }
};

const uint16_t kLocalhostLookupPort = 80;

bool HasEndpoint(const IPEndPoint& endpoint, const AddressList& addresses) {
  for (const auto& address : addresses) {
    if (endpoint == address)
      return true;
  }
  return false;
}

void TestBothLoopbackIPs(const std::string& host) {
  IPEndPoint localhost_ipv4(IPAddress::IPv4Localhost(), kLocalhostLookupPort);
  IPEndPoint localhost_ipv6(IPAddress::IPv6Localhost(), kLocalhostLookupPort);

  AddressList addresses;
  EXPECT_TRUE(ResolveLocalHostname(host, kLocalhostLookupPort, &addresses));
  EXPECT_EQ(2u, addresses.size());
  EXPECT_TRUE(HasEndpoint(localhost_ipv4, addresses));
  EXPECT_TRUE(HasEndpoint(localhost_ipv6, addresses));
}

void TestIPv6LoopbackOnly(const std::string& host) {
  IPEndPoint localhost_ipv6(IPAddress::IPv6Localhost(), kLocalhostLookupPort);

  AddressList addresses;
  EXPECT_TRUE(ResolveLocalHostname(host, kLocalhostLookupPort, &addresses));
  EXPECT_EQ(1u, addresses.size());
  EXPECT_TRUE(HasEndpoint(localhost_ipv6, addresses));
}

}  // namespace

class HostResolverImplTest : public TestWithScopedTaskEnvironment {
 public:
  static const int kDefaultPort = 80;

  HostResolverImplTest() : proc_(new MockHostResolverProc()) {}

  void CreateResolver() {
    CreateResolverWithLimitsAndParams(kMaxJobs,
                                      DefaultParams(proc_.get()));
  }

  // This HostResolverImpl will only allow 1 outstanding resolve at a time and
  // perform no retries.
  void CreateSerialResolver() {
    HostResolverImpl::ProcTaskParams params = DefaultParams(proc_.get());
    params.max_retry_attempts = 0u;
    CreateResolverWithLimitsAndParams(1u, params);
  }

 protected:
  // A Request::Handler which is a proxy to the HostResolverImplTest fixture.
  struct Handler : public Request::Handler {
    ~Handler() override = default;

    // Proxy functions so that classes derived from Handler can access them.
    Request* CreateRequest(const HostResolver::RequestInfo& info,
                           RequestPriority priority) {
      return test->CreateRequest(info, priority);
    }
    Request* CreateRequest(const std::string& hostname, int port) {
      return test->CreateRequest(hostname, port);
    }
    Request* CreateRequest(const std::string& hostname) {
      return test->CreateRequest(hostname);
    }
    std::vector<std::unique_ptr<Request>>& requests() {
      return test->requests_;
    }

    void DeleteResolver() { test->resolver_.reset(); }

    HostResolverImplTest* test;
  };

  // testing::Test implementation:
  void SetUp() override { CreateResolver(); }

  void TearDown() override {
    if (resolver_.get())
      EXPECT_EQ(0u, resolver_->num_running_dispatcher_jobs_for_tests());
    EXPECT_FALSE(proc_->HasBlockedRequests());
  }

  virtual void CreateResolverWithLimitsAndParams(
      size_t max_concurrent_resolves,
      const HostResolverImpl::ProcTaskParams& params) {
    HostResolverImpl::Options options = DefaultOptions();
    options.max_concurrent_resolves = max_concurrent_resolves;
    resolver_.reset(new TestHostResolverImpl(options, NULL));
    resolver_->set_proc_params_for_test(params);
  }

  // The Request will not be made until a call to |Resolve()|, and the Job will
  // not start until released by |proc_->SignalXXX|.
  Request* CreateRequest(const HostResolver::RequestInfo& info,
                         RequestPriority priority) {
    requests_.push_back(std::make_unique<Request>(
        info, priority, requests_.size(), resolver_.get(), handler_.get()));
    return requests_.back().get();
  }

  Request* CreateRequest(const std::string& hostname,
                         int port,
                         RequestPriority priority,
                         AddressFamily family) {
    HostResolver::RequestInfo info(HostPortPair(hostname, port));
    info.set_address_family(family);
    return CreateRequest(info, priority);
  }

  Request* CreateRequest(const std::string& hostname,
                         int port,
                         RequestPriority priority) {
    return CreateRequest(hostname, port, priority, ADDRESS_FAMILY_UNSPECIFIED);
  }

  Request* CreateRequest(const std::string& hostname, int port) {
    return CreateRequest(hostname, port, MEDIUM);
  }

  Request* CreateRequest(const std::string& hostname) {
    return CreateRequest(hostname, kDefaultPort);
  }

  void set_handler(Handler* handler) {
    handler_.reset(handler);
    handler_->test = this;
  }

  // Friendship is not inherited, so use proxies to access those.
  size_t num_running_dispatcher_jobs() const {
    DCHECK(resolver_.get());
    return resolver_->num_running_dispatcher_jobs_for_tests();
  }

  void set_fallback_to_proctask(bool fallback_to_proctask) {
    DCHECK(resolver_.get());
    resolver_->fallback_to_proctask_ = fallback_to_proctask;
  }

  static unsigned maximum_dns_failures() {
    return HostResolverImpl::kMaximumDnsFailures;
  }

  bool IsIPv6Reachable(const NetLogWithSource& net_log) {
    return resolver_->IsIPv6Reachable(net_log);
  }

  const HostCache::Entry* GetCacheEntry(const Request& req) {
    DCHECK(resolver_.get() && resolver_->GetHostCache());
    const HostCache::Key key(req.info().hostname(), req.info().address_family(),
                             req.info().host_resolver_flags());
    return resolver_->GetHostCache()->LookupStale(key, base::TimeTicks(),
                                                  nullptr);
  }

  void MakeCacheStale() {
    DCHECK(resolver_.get());
    resolver_->GetHostCache()->OnNetworkChange();
  }

  scoped_refptr<MockHostResolverProc> proc_;
  std::unique_ptr<HostResolverImpl> resolver_;
  std::vector<std::unique_ptr<Request>> requests_;

  std::unique_ptr<Handler> handler_;
};

TEST_F(HostResolverImplTest, AsynchronousLookup) {
  proc_->AddRuleForAllFamilies("just.testing", "192.168.1.42");
  proc_->SignalMultiple(1u);

  Request* req = CreateRequest("just.testing", 80);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(req->WaitForResult(), IsOk());

  EXPECT_TRUE(req->HasOneAddress("192.168.1.42", 80));
  EXPECT_EQ("just.testing", proc_->GetCaptureList()[0].hostname);
}

TEST_F(HostResolverImplTest, LocalhostIPV4IPV6Lookup) {
  Request* req1 = CreateRequest("localhost6", 80, MEDIUM, ADDRESS_FAMILY_IPV4);
  EXPECT_THAT(req1->Resolve(), IsOk());
  EXPECT_EQ(0u, req1->NumberOfAddresses());

  Request* req2 = CreateRequest("localhost6", 80, MEDIUM, ADDRESS_FAMILY_IPV6);
  EXPECT_THAT(req2->Resolve(), IsOk());
  EXPECT_TRUE(req2->HasOneAddress("::1", 80));

  Request* req3 =
      CreateRequest("localhost6", 80, MEDIUM, ADDRESS_FAMILY_UNSPECIFIED);
  EXPECT_THAT(req3->Resolve(), IsOk());
  EXPECT_TRUE(req3->HasOneAddress("::1", 80));

  Request* req4 = CreateRequest("localhost", 80, MEDIUM, ADDRESS_FAMILY_IPV4);
  EXPECT_THAT(req4->Resolve(), IsOk());
  EXPECT_TRUE(req4->HasOneAddress("127.0.0.1", 80));

  Request* req5 = CreateRequest("localhost", 80, MEDIUM, ADDRESS_FAMILY_IPV6);
  EXPECT_THAT(req5->Resolve(), IsOk());
  EXPECT_TRUE(req5->HasOneAddress("::1", 80));
}

TEST_F(HostResolverImplTest, ResolveIPLiteralWithHostResolverSystemOnly) {
  const char kIpLiteral[] = "178.78.32.1";
  // Add a mapping to tell if the resolver proc was called (if it was called,
  // then the result will be the remapped value. Otherwise it will be the IP
  // literal).
  proc_->AddRuleForAllFamilies(kIpLiteral, "183.45.32.1");

  HostResolver::RequestInfo info_bypass(HostPortPair(kIpLiteral, 80));
  info_bypass.set_host_resolver_flags(HOST_RESOLVER_SYSTEM_ONLY);

  Request* req = CreateRequest(info_bypass, MEDIUM);
  EXPECT_THAT(req->Resolve(), IsOk());

  EXPECT_TRUE(req->HasAddress(kIpLiteral, 80));
}

TEST_F(HostResolverImplTest, EmptyListMeansNameNotResolved) {
  proc_->AddRuleForAllFamilies("just.testing", "");
  proc_->SignalMultiple(1u);

  Request* req = CreateRequest("just.testing", 80);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(req->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));
  EXPECT_EQ(0u, req->NumberOfAddresses());
  EXPECT_EQ("just.testing", proc_->GetCaptureList()[0].hostname);
}

TEST_F(HostResolverImplTest, FailedAsynchronousLookup) {
  proc_->AddRuleForAllFamilies(std::string(),
                               "0.0.0.0");  // Default to failures.
  proc_->SignalMultiple(1u);

  Request* req = CreateRequest("just.testing", 80);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(req->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));

  EXPECT_EQ("just.testing", proc_->GetCaptureList()[0].hostname);

  // Also test that the error is not cached.
  EXPECT_THAT(req->ResolveFromCache(), IsError(ERR_DNS_CACHE_MISS));
}

TEST_F(HostResolverImplTest, AbortedAsynchronousLookup) {
  Request* req0 = CreateRequest("just.testing", 80);
  EXPECT_THAT(req0->Resolve(), IsError(ERR_IO_PENDING));

  EXPECT_TRUE(proc_->WaitFor(1u));

  // Resolver is destroyed while job is running on WorkerPool.
  resolver_.reset();

  proc_->SignalAll();

  // To ensure there was no spurious callback, complete with a new resolver.
  CreateResolver();
  Request* req1 = CreateRequest("just.testing", 80);
  EXPECT_THAT(req1->Resolve(), IsError(ERR_IO_PENDING));

  proc_->SignalMultiple(2u);

  EXPECT_THAT(req1->WaitForResult(), IsOk());

  // This request was canceled.
  EXPECT_FALSE(req0->completed());
}

#if defined(THREAD_SANITIZER)
// Use of WorkerPool in HostResolverImpl causes a data race. crbug.com/334140
#define MAYBE_NumericIPv4Address DISABLED_NumericIPv4Address
#else
#define MAYBE_NumericIPv4Address NumericIPv4Address
#endif
TEST_F(HostResolverImplTest, MAYBE_NumericIPv4Address) {
  // Stevens says dotted quads with AI_UNSPEC resolve to a single sockaddr_in.
  Request* req = CreateRequest("127.1.2.3", 5555);
  EXPECT_THAT(req->Resolve(), IsOk());

  EXPECT_TRUE(req->HasOneAddress("127.1.2.3", 5555));
}

#if defined(THREAD_SANITIZER)
// Use of WorkerPool in HostResolverImpl causes a data race. crbug.com/334140
#define MAYBE_NumericIPv6Address DISABLED_NumericIPv6Address
#else
#define MAYBE_NumericIPv6Address NumericIPv6Address
#endif
TEST_F(HostResolverImplTest, MAYBE_NumericIPv6Address) {
  // Resolve a plain IPv6 address.  Don't worry about [brackets], because
  // the caller should have removed them.
  Request* req = CreateRequest("2001:db8::1", 5555);
  EXPECT_THAT(req->Resolve(), IsOk());

  EXPECT_TRUE(req->HasOneAddress("2001:db8::1", 5555));
}

#if defined(THREAD_SANITIZER)
// Use of WorkerPool in HostResolverImpl causes a data race. crbug.com/334140
#define MAYBE_EmptyHost DISABLED_EmptyHost
#else
#define MAYBE_EmptyHost EmptyHost
#endif
TEST_F(HostResolverImplTest, MAYBE_EmptyHost) {
  Request* req = CreateRequest(std::string(), 5555);
  EXPECT_THAT(req->Resolve(), IsError(ERR_NAME_NOT_RESOLVED));
}

#if defined(THREAD_SANITIZER)
// There's a data race in this test that may lead to use-after-free.
// If the test starts to crash without ThreadSanitizer it needs to be disabled
// globally. See http://crbug.com/268946 (stacks for this test in
// crbug.com/333567).
#define MAYBE_EmptyDotsHost DISABLED_EmptyDotsHost
#else
#define MAYBE_EmptyDotsHost EmptyDotsHost
#endif
TEST_F(HostResolverImplTest, MAYBE_EmptyDotsHost) {
  for (int i = 0; i < 16; ++i) {
    Request* req = CreateRequest(std::string(i, '.'), 5555);
    EXPECT_THAT(req->Resolve(), IsError(ERR_NAME_NOT_RESOLVED));
  }
}

#if defined(THREAD_SANITIZER)
// There's a data race in this test that may lead to use-after-free.
// If the test starts to crash without ThreadSanitizer it needs to be disabled
// globally. See http://crbug.com/268946.
#define MAYBE_LongHost DISABLED_LongHost
#else
#define MAYBE_LongHost LongHost
#endif
TEST_F(HostResolverImplTest, MAYBE_LongHost) {
  Request* req = CreateRequest(std::string(4097, 'a'), 5555);
  EXPECT_THAT(req->Resolve(), IsError(ERR_NAME_NOT_RESOLVED));
}

TEST_F(HostResolverImplTest, DeDupeRequests) {
  // Start 5 requests, duplicating hosts "a" and "b". Since the resolver_proc is
  // blocked, these should all pile up until we signal it.
  EXPECT_THAT(CreateRequest("a", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("b", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("b", 81)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("a", 82)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("b", 83)->Resolve(), IsError(ERR_IO_PENDING));

  proc_->SignalMultiple(2u);  // One for "a", one for "b".

  for (size_t i = 0; i < requests_.size(); ++i) {
    EXPECT_EQ(OK, requests_[i]->WaitForResult()) << i;
  }
}

TEST_F(HostResolverImplTest, CancelMultipleRequests) {
  EXPECT_THAT(CreateRequest("a", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("b", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("b", 81)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("a", 82)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("b", 83)->Resolve(), IsError(ERR_IO_PENDING));

  // Cancel everything except request for ("a", 82).
  requests_[0]->Cancel();
  requests_[1]->Cancel();
  requests_[2]->Cancel();
  requests_[4]->Cancel();

  proc_->SignalMultiple(2u);  // One for "a", one for "b".

  EXPECT_THAT(requests_[3]->WaitForResult(), IsOk());
}

TEST_F(HostResolverImplTest, CanceledRequestsReleaseJobSlots) {
  // Fill up the dispatcher and queue.
  for (unsigned i = 0; i < kMaxJobs + 1; ++i) {
    std::string hostname = "a_";
    hostname[1] = 'a' + i;
    EXPECT_THAT(CreateRequest(hostname, 80)->Resolve(),
                IsError(ERR_IO_PENDING));
    EXPECT_THAT(CreateRequest(hostname, 81)->Resolve(),
                IsError(ERR_IO_PENDING));
  }

  EXPECT_TRUE(proc_->WaitFor(kMaxJobs));

  // Cancel all but last two.
  for (unsigned i = 0; i < requests_.size() - 2; ++i) {
    requests_[i]->Cancel();
  }

  EXPECT_TRUE(proc_->WaitFor(kMaxJobs + 1));

  proc_->SignalAll();

  size_t num_requests = requests_.size();
  EXPECT_THAT(requests_[num_requests - 1]->WaitForResult(), IsOk());
  EXPECT_THAT(requests_[num_requests - 2]->result(), IsOk());
}

TEST_F(HostResolverImplTest, CancelWithinCallback) {
  struct MyHandler : public Handler {
    void Handle(Request* req) override {
      // Port 80 is the first request that the callback will be invoked for.
      // While we are executing within that callback, cancel the other requests
      // in the job and start another request.
      if (req->index() == 0) {
        // Once "a:80" completes, it will cancel "a:81" and "a:82".
        requests()[1]->Cancel();
        requests()[2]->Cancel();
      }
    }
  };
  set_handler(new MyHandler());

  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(ERR_IO_PENDING, CreateRequest("a", 80 + i)->Resolve()) << i;
  }

  proc_->SignalMultiple(2u);  // One for "a". One for "finalrequest".

  EXPECT_THAT(requests_[0]->WaitForResult(), IsOk());

  Request* final_request = CreateRequest("finalrequest", 70);
  EXPECT_THAT(final_request->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(final_request->WaitForResult(), IsOk());
  EXPECT_TRUE(requests_[3]->completed());
}

TEST_F(HostResolverImplTest, DeleteWithinCallback) {
  struct MyHandler : public Handler {
    void Handle(Request* req) override {
      EXPECT_EQ("a", req->info().hostname());
      EXPECT_EQ(80, req->info().port());

      DeleteResolver();

      // Quit after returning from OnCompleted (to give it a chance at
      // incorrectly running the cancelled tasks).
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
    }
  };
  set_handler(new MyHandler());

  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(ERR_IO_PENDING, CreateRequest("a", 80 + i)->Resolve()) << i;
  }

  proc_->SignalMultiple(1u);  // One for "a".

  // |MyHandler| will send quit message once all the requests have finished.
  base::RunLoop().Run();
}

TEST_F(HostResolverImplTest, DeleteWithinAbortedCallback) {
  struct MyHandler : public Handler {
    void Handle(Request* req) override {
      EXPECT_EQ("a", req->info().hostname());
      EXPECT_EQ(80, req->info().port());

      DeleteResolver();

      // Quit after returning from OnCompleted (to give it a chance at
      // incorrectly running the cancelled tasks).
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
    }
  };
  set_handler(new MyHandler());

  // This test assumes that the Jobs will be Aborted in order ["a", "b"]
  EXPECT_THAT(CreateRequest("a", 80)->Resolve(), IsError(ERR_IO_PENDING));
  // HostResolverImpl will be deleted before later Requests can complete.
  EXPECT_THAT(CreateRequest("a", 81)->Resolve(), IsError(ERR_IO_PENDING));
  // Job for 'b' will be aborted before it can complete.
  EXPECT_THAT(CreateRequest("b", 82)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("b", 83)->Resolve(), IsError(ERR_IO_PENDING));

  EXPECT_TRUE(proc_->WaitFor(1u));

  // Triggering an IP address change.
  NetworkChangeNotifier::NotifyObserversOfIPAddressChangeForTests();

  // |MyHandler| will send quit message once all the requests have finished.
  base::RunLoop().Run();

  EXPECT_THAT(requests_[0]->result(), IsError(ERR_NETWORK_CHANGED));
  EXPECT_THAT(requests_[1]->result(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[2]->result(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[3]->result(), IsError(ERR_IO_PENDING));
  // Clean up.
  proc_->SignalMultiple(requests_.size());
}

TEST_F(HostResolverImplTest, StartWithinCallback) {
  struct MyHandler : public Handler {
    void Handle(Request* req) override {
      if (req->index() == 0) {
        // On completing the first request, start another request for "a".
        // Since caching is disabled, this will result in another async request.
        EXPECT_THAT(CreateRequest("a", 70)->Resolve(), IsError(ERR_IO_PENDING));
      }
    }
  };
  set_handler(new MyHandler());

  // Turn off caching for this host resolver.
  HostResolver::Options options = DefaultOptions();
  options.enable_caching = false;
  resolver_.reset(new TestHostResolverImpl(options, NULL));
  resolver_->set_proc_params_for_test(DefaultParams(proc_.get()));

  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(ERR_IO_PENDING, CreateRequest("a", 80 + i)->Resolve()) << i;
  }

  proc_->SignalMultiple(2u);  // One for "a". One for the second "a".

  EXPECT_THAT(requests_[0]->WaitForResult(), IsOk());
  ASSERT_EQ(5u, requests_.size());
  EXPECT_THAT(requests_.back()->WaitForResult(), IsOk());

  EXPECT_EQ(2u, proc_->GetCaptureList().size());
}

TEST_F(HostResolverImplTest, BypassCache) {
  struct MyHandler : public Handler {
    void Handle(Request* req) override {
      if (req->index() == 0) {
        // On completing the first request, start another request for "a".
        // Since caching is enabled, this should complete synchronously.
        std::string hostname = req->info().hostname();
        EXPECT_THAT(CreateRequest(hostname, 70)->Resolve(), IsOk());
        EXPECT_THAT(CreateRequest(hostname, 75)->ResolveFromCache(), IsOk());

        // Ok good. Now make sure that if we ask to bypass the cache, it can no
        // longer service the request synchronously.
        HostResolver::RequestInfo info(HostPortPair(hostname, 71));
        info.set_allow_cached_response(false);
        EXPECT_EQ(ERR_IO_PENDING,
                  CreateRequest(info, DEFAULT_PRIORITY)->Resolve());
      } else if (71 == req->info().port()) {
        // Test is done.
        base::RunLoop::QuitCurrentWhenIdleDeprecated();
      } else {
        FAIL() << "Unexpected request";
      }
    }
  };
  set_handler(new MyHandler());

  EXPECT_THAT(CreateRequest("a", 80)->Resolve(), IsError(ERR_IO_PENDING));
  proc_->SignalMultiple(3u);  // Only need two, but be generous.

  // |verifier| will send quit message once all the requests have finished.
  base::RunLoop().Run();
  EXPECT_EQ(2u, proc_->GetCaptureList().size());
}

// Test that IP address changes flush the cache but initial DNS config reads do
// not.
TEST_F(HostResolverImplTest, FlushCacheOnIPAddressChange) {
  proc_->SignalMultiple(2u);  // One before the flush, one after.

  Request* req = CreateRequest("host1", 70);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(req->WaitForResult(), IsOk());

  req = CreateRequest("host1", 75);
  EXPECT_THAT(req->Resolve(), IsOk());  // Should complete synchronously.

  // Verify initial DNS config read does not flush cache.
  NetworkChangeNotifier::NotifyObserversOfInitialDNSConfigReadForTests();
  req = CreateRequest("host1", 75);
  EXPECT_THAT(req->Resolve(), IsOk());  // Should complete synchronously.

  // Flush cache by triggering an IP address change.
  NetworkChangeNotifier::NotifyObserversOfIPAddressChangeForTests();
  base::RunLoop().RunUntilIdle();  // Notification happens async.

  // Resolve "host1" again -- this time it won't be served from cache, so it
  // will complete asynchronously.
  req = CreateRequest("host1", 80);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(req->WaitForResult(), IsOk());
}

// Test that IP address changes send ERR_NETWORK_CHANGED to pending requests.
TEST_F(HostResolverImplTest, AbortOnIPAddressChanged) {
  Request* req = CreateRequest("host1", 70);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));

  EXPECT_TRUE(proc_->WaitFor(1u));
  // Triggering an IP address change.
  NetworkChangeNotifier::NotifyObserversOfIPAddressChangeForTests();
  base::RunLoop().RunUntilIdle();  // Notification happens async.
  proc_->SignalAll();

  EXPECT_THAT(req->WaitForResult(), IsError(ERR_NETWORK_CHANGED));
  EXPECT_EQ(0u, resolver_->GetHostCache()->size());
}

// Test that initial DNS config read signals do not abort pending requests.
TEST_F(HostResolverImplTest, DontAbortOnInitialDNSConfigRead) {
  Request* req = CreateRequest("host1", 70);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));

  EXPECT_TRUE(proc_->WaitFor(1u));
  // Triggering initial DNS config read signal.
  NetworkChangeNotifier::NotifyObserversOfInitialDNSConfigReadForTests();
  base::RunLoop().RunUntilIdle();  // Notification happens async.
  proc_->SignalAll();

  EXPECT_THAT(req->WaitForResult(), IsOk());
}

// Obey pool constraints after IP address has changed.
TEST_F(HostResolverImplTest, ObeyPoolConstraintsAfterIPAddressChange) {
  // Runs at most one job at a time.
  CreateSerialResolver();
  EXPECT_THAT(CreateRequest("a")->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("b")->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("c")->Resolve(), IsError(ERR_IO_PENDING));

  EXPECT_TRUE(proc_->WaitFor(1u));
  // Triggering an IP address change.
  NetworkChangeNotifier::NotifyObserversOfIPAddressChangeForTests();
  base::RunLoop().RunUntilIdle();  // Notification happens async.
  proc_->SignalMultiple(3u);  // Let the false-start go so that we can catch it.

  EXPECT_THAT(requests_[0]->WaitForResult(), IsError(ERR_NETWORK_CHANGED));

  EXPECT_EQ(1u, num_running_dispatcher_jobs());

  EXPECT_FALSE(requests_[1]->completed());
  EXPECT_FALSE(requests_[2]->completed());

  EXPECT_THAT(requests_[2]->WaitForResult(), IsOk());
  EXPECT_THAT(requests_[1]->result(), IsOk());
}

// Tests that a new Request made from the callback of a previously aborted one
// will not be aborted.
TEST_F(HostResolverImplTest, AbortOnlyExistingRequestsOnIPAddressChange) {
  struct MyHandler : public Handler {
    void Handle(Request* req) override {
      // Start new request for a different hostname to ensure that the order
      // of jobs in HostResolverImpl is not stable.
      std::string hostname;
      if (req->index() == 0)
        hostname = "zzz";
      else if (req->index() == 1)
        hostname = "aaa";
      else if (req->index() == 2)
        hostname = "eee";
      else
        return;  // A request started from within MyHandler.
      EXPECT_EQ(ERR_IO_PENDING, CreateRequest(hostname)->Resolve()) << hostname;
    }
  };
  set_handler(new MyHandler());

  EXPECT_THAT(CreateRequest("bbb")->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("eee")->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("ccc")->Resolve(), IsError(ERR_IO_PENDING));

  // Wait until all are blocked;
  EXPECT_TRUE(proc_->WaitFor(3u));
  // Trigger an IP address change.
  NetworkChangeNotifier::NotifyObserversOfIPAddressChangeForTests();
  // This should abort all running jobs.
  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(requests_[0]->result(), IsError(ERR_NETWORK_CHANGED));
  EXPECT_THAT(requests_[1]->result(), IsError(ERR_NETWORK_CHANGED));
  EXPECT_THAT(requests_[2]->result(), IsError(ERR_NETWORK_CHANGED));
  ASSERT_EQ(6u, requests_.size());
  // Unblock all calls to proc.
  proc_->SignalMultiple(requests_.size());
  // Run until the re-started requests finish.
  EXPECT_THAT(requests_[3]->WaitForResult(), IsOk());
  EXPECT_THAT(requests_[4]->WaitForResult(), IsOk());
  EXPECT_THAT(requests_[5]->WaitForResult(), IsOk());
  // Verify that results of aborted Jobs were not cached.
  EXPECT_EQ(6u, proc_->GetCaptureList().size());
  EXPECT_EQ(3u, resolver_->GetHostCache()->size());
}

// Tests that when the maximum threads is set to 1, requests are dequeued
// in order of priority.
TEST_F(HostResolverImplTest, HigherPriorityRequestsStartedFirst) {
  CreateSerialResolver();

  // Note that at this point the MockHostResolverProc is blocked, so any
  // requests we make will not complete.
  CreateRequest("req0", 80, LOW);
  CreateRequest("req1", 80, MEDIUM);
  CreateRequest("req2", 80, MEDIUM);
  CreateRequest("req3", 80, LOW);
  CreateRequest("req4", 80, HIGHEST);
  CreateRequest("req5", 80, LOW);
  CreateRequest("req6", 80, LOW);
  CreateRequest("req5", 80, HIGHEST);

  for (size_t i = 0; i < requests_.size(); ++i) {
    EXPECT_EQ(ERR_IO_PENDING, requests_[i]->Resolve()) << i;
  }

  // Unblock the resolver thread so the requests can run.
  proc_->SignalMultiple(requests_.size());  // More than needed.

  // Wait for all the requests to complete succesfully.
  for (size_t i = 0; i < requests_.size(); ++i) {
    EXPECT_EQ(OK, requests_[i]->WaitForResult()) << i;
  }

  // Since we have restricted to a single concurrent thread in the jobpool,
  // the requests should complete in order of priority (with the exception
  // of the first request, which gets started right away, since there is
  // nothing outstanding).
  MockHostResolverProc::CaptureList capture_list = proc_->GetCaptureList();
  ASSERT_EQ(7u, capture_list.size());

  EXPECT_EQ("req0", capture_list[0].hostname);
  EXPECT_EQ("req4", capture_list[1].hostname);
  EXPECT_EQ("req5", capture_list[2].hostname);
  EXPECT_EQ("req1", capture_list[3].hostname);
  EXPECT_EQ("req2", capture_list[4].hostname);
  EXPECT_EQ("req3", capture_list[5].hostname);
  EXPECT_EQ("req6", capture_list[6].hostname);
}

// Test that changing a job's priority affects the dequeueing order.
TEST_F(HostResolverImplTest, ChangePriority) {
  CreateSerialResolver();

  CreateRequest("req0", 80, MEDIUM);
  CreateRequest("req1", 80, LOW);
  CreateRequest("req2", 80, LOWEST);

  ASSERT_EQ(3u, requests_.size());

  // req0 starts immediately; without ChangePriority, req1 and then req2 should
  // run.
  EXPECT_THAT(requests_[0]->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[1]->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[2]->Resolve(), IsError(ERR_IO_PENDING));

  // Changing req2 to HIGH should make it run before req1.
  // (It can't run before req0, since req0 started immediately.)
  requests_[2]->ChangePriority(HIGHEST);

  // Let all 3 requests finish.
  proc_->SignalMultiple(3u);

  EXPECT_THAT(requests_[0]->WaitForResult(), IsOk());
  EXPECT_THAT(requests_[1]->WaitForResult(), IsOk());
  EXPECT_THAT(requests_[2]->WaitForResult(), IsOk());

  MockHostResolverProc::CaptureList capture_list = proc_->GetCaptureList();
  ASSERT_EQ(3u, capture_list.size());

  EXPECT_EQ("req0", capture_list[0].hostname);
  EXPECT_EQ("req2", capture_list[1].hostname);
  EXPECT_EQ("req1", capture_list[2].hostname);
}

// Try cancelling a job which has not started yet.
TEST_F(HostResolverImplTest, CancelPendingRequest) {
  CreateSerialResolver();

  CreateRequest("req0", 80, LOWEST);
  CreateRequest("req1", 80, HIGHEST);  // Will cancel.
  CreateRequest("req2", 80, MEDIUM);
  CreateRequest("req3", 80, LOW);
  CreateRequest("req4", 80, HIGHEST);  // Will cancel.
  CreateRequest("req5", 80, LOWEST);   // Will cancel.
  CreateRequest("req6", 80, MEDIUM);

  // Start all of the requests.
  for (size_t i = 0; i < requests_.size(); ++i) {
    EXPECT_EQ(ERR_IO_PENDING, requests_[i]->Resolve()) << i;
  }

  // Cancel some requests
  requests_[1]->Cancel();
  requests_[4]->Cancel();
  requests_[5]->Cancel();

  // Unblock the resolver thread so the requests can run.
  proc_->SignalMultiple(requests_.size());  // More than needed.

  // Wait for all the requests to complete succesfully.
  for (size_t i = 0; i < requests_.size(); ++i) {
    if (!requests_[i]->pending())
      continue;  // Don't wait for the requests we cancelled.
    EXPECT_EQ(OK, requests_[i]->WaitForResult()) << i;
  }

  // Verify that they called out the the resolver proc (which runs on the
  // resolver thread) in the expected order.
  MockHostResolverProc::CaptureList capture_list = proc_->GetCaptureList();
  ASSERT_EQ(4u, capture_list.size());

  EXPECT_EQ("req0", capture_list[0].hostname);
  EXPECT_EQ("req2", capture_list[1].hostname);
  EXPECT_EQ("req6", capture_list[2].hostname);
  EXPECT_EQ("req3", capture_list[3].hostname);
}

// Test that when too many requests are enqueued, old ones start to be aborted.
TEST_F(HostResolverImplTest, QueueOverflow) {
  CreateSerialResolver();

  // Allow only 3 queued jobs.
  const size_t kMaxPendingJobs = 3u;
  resolver_->SetMaxQueuedJobs(kMaxPendingJobs);

  // Note that at this point the MockHostResolverProc is blocked, so any
  // requests we make will not complete.

  EXPECT_THAT(CreateRequest("req0", 80, LOWEST)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("req1", 80, HIGHEST)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("req2", 80, MEDIUM)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("req3", 80, MEDIUM)->Resolve(),
              IsError(ERR_IO_PENDING));

  // At this point, there are 3 enqueued jobs.
  // Insertion of subsequent requests will cause evictions
  // based on priority.

  EXPECT_EQ(ERR_HOST_RESOLVER_QUEUE_TOO_LARGE,
            CreateRequest("req4", 80, LOW)->Resolve());  // Evicts itself!

  EXPECT_THAT(CreateRequest("req5", 80, MEDIUM)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[2]->result(),
              IsError(ERR_HOST_RESOLVER_QUEUE_TOO_LARGE));
  EXPECT_THAT(CreateRequest("req6", 80, HIGHEST)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[3]->result(),
              IsError(ERR_HOST_RESOLVER_QUEUE_TOO_LARGE));
  EXPECT_THAT(CreateRequest("req7", 80, MEDIUM)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[5]->result(),
              IsError(ERR_HOST_RESOLVER_QUEUE_TOO_LARGE));

  // Unblock the resolver thread so the requests can run.
  proc_->SignalMultiple(4u);

  // The rest should succeed.
  EXPECT_THAT(requests_[7]->WaitForResult(), IsOk());
  EXPECT_THAT(requests_[0]->result(), IsOk());
  EXPECT_THAT(requests_[1]->result(), IsOk());
  EXPECT_THAT(requests_[6]->result(), IsOk());

  // Verify that they called out the the resolver proc (which runs on the
  // resolver thread) in the expected order.
  MockHostResolverProc::CaptureList capture_list = proc_->GetCaptureList();
  ASSERT_EQ(4u, capture_list.size());

  EXPECT_EQ("req0", capture_list[0].hostname);
  EXPECT_EQ("req1", capture_list[1].hostname);
  EXPECT_EQ("req6", capture_list[2].hostname);
  EXPECT_EQ("req7", capture_list[3].hostname);

  // Verify that the evicted (incomplete) requests were not cached.
  EXPECT_EQ(4u, resolver_->GetHostCache()->size());

  for (size_t i = 0; i < requests_.size(); ++i) {
    EXPECT_TRUE(requests_[i]->completed()) << i;
  }
}

// Make sure that the address family parameter is respected when raw IPs are
// passed in.
TEST_F(HostResolverImplTest, AddressFamilyWithRawIPs) {
  Request* request =
      CreateRequest("127.0.0.1", 80,  MEDIUM, ADDRESS_FAMILY_IPV4);
  EXPECT_THAT(request->Resolve(), IsOk());
  EXPECT_TRUE(request->HasOneAddress("127.0.0.1", 80));

  request = CreateRequest("127.0.0.1", 80,  MEDIUM, ADDRESS_FAMILY_IPV6);
  EXPECT_THAT(request->Resolve(), IsError(ERR_NAME_NOT_RESOLVED));

  request = CreateRequest("127.0.0.1", 80,  MEDIUM, ADDRESS_FAMILY_UNSPECIFIED);
  EXPECT_THAT(request->Resolve(), IsOk());
  EXPECT_TRUE(request->HasOneAddress("127.0.0.1", 80));

  request = CreateRequest("::1", 80,  MEDIUM, ADDRESS_FAMILY_IPV4);
  EXPECT_THAT(request->Resolve(), IsError(ERR_NAME_NOT_RESOLVED));

  request = CreateRequest("::1", 80,  MEDIUM, ADDRESS_FAMILY_IPV6);
  EXPECT_THAT(request->Resolve(), IsOk());
  EXPECT_TRUE(request->HasOneAddress("::1", 80));

  request = CreateRequest("::1", 80,  MEDIUM, ADDRESS_FAMILY_UNSPECIFIED);
  EXPECT_THAT(request->Resolve(), IsOk());
  EXPECT_TRUE(request->HasOneAddress("::1", 80));
}

TEST_F(HostResolverImplTest, ResolveFromCache) {
  proc_->AddRuleForAllFamilies("just.testing", "192.168.1.42");
  proc_->SignalMultiple(1u);  // Need only one.

  HostResolver::RequestInfo info(HostPortPair("just.testing", 80));

  // First query will miss the cache.
  EXPECT_EQ(ERR_DNS_CACHE_MISS,
            CreateRequest(info, DEFAULT_PRIORITY)->ResolveFromCache());

  // This time, we fetch normally.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[1]->WaitForResult(), IsOk());

  // Now we should be able to fetch from the cache.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->ResolveFromCache(),
              IsOk());
  EXPECT_TRUE(requests_[2]->HasOneAddress("192.168.1.42", 80));
}

TEST_F(HostResolverImplTest, ResolveFromCacheInvalidName) {
  proc_->AddRuleForAllFamilies("foo,bar.com", "192.168.1.42");

  HostResolver::RequestInfo info(HostPortPair("foo,bar.com", 80));

  // Query should be rejected before it makes it to the cache.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->ResolveFromCache(),
              IsError(ERR_NAME_NOT_RESOLVED));

  // Query should be rejected without attempting to resolve it.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->Resolve(),
              IsError(ERR_NAME_NOT_RESOLVED));
  EXPECT_THAT(requests_[1]->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));
}

TEST_F(HostResolverImplTest, ResolveFromCacheInvalidNameLocalhost) {
  HostResolver::RequestInfo info(HostPortPair("foo,bar.localhost", 80));

  // Query should be rejected before it makes it to the localhost check.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->ResolveFromCache(),
              IsError(ERR_NAME_NOT_RESOLVED));

  // Query should be rejected without attempting to resolve it.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->Resolve(),
              IsError(ERR_NAME_NOT_RESOLVED));
  EXPECT_THAT(requests_[1]->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));
}

TEST_F(HostResolverImplTest, ResolveStaleFromCache) {
  proc_->AddRuleForAllFamilies("just.testing", "192.168.1.42");
  proc_->SignalMultiple(1u);  // Need only one.

  HostResolver::RequestInfo info(HostPortPair("just.testing", 80));

  // First query will miss the cache.
  EXPECT_EQ(ERR_DNS_CACHE_MISS,
            CreateRequest(info, DEFAULT_PRIORITY)->ResolveFromCache());

  // This time, we fetch normally.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[1]->WaitForResult(), IsOk());

  // Now we should be able to fetch from the cache.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->ResolveFromCache(),
              IsOk());
  EXPECT_TRUE(requests_[2]->HasOneAddress("192.168.1.42", 80));
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->ResolveStaleFromCache(),
              IsOk());
  EXPECT_TRUE(requests_[3]->HasOneAddress("192.168.1.42", 80));
  EXPECT_FALSE(requests_[3]->staleness().is_stale());

  MakeCacheStale();

  // Now we should be able to fetch from the cache only if we use
  // ResolveStaleFromCache.
  EXPECT_EQ(ERR_DNS_CACHE_MISS,
            CreateRequest(info, DEFAULT_PRIORITY)->ResolveFromCache());
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->ResolveStaleFromCache(),
              IsOk());
  EXPECT_TRUE(requests_[5]->HasOneAddress("192.168.1.42", 80));
  EXPECT_TRUE(requests_[5]->staleness().is_stale());
}

TEST_F(HostResolverImplTest, ResolveStaleFromCacheError) {
  proc_->AddRuleForAllFamilies("just.testing", "192.168.1.42");
  proc_->SignalMultiple(1u);  // Need only one.

  HostResolver::RequestInfo info(HostPortPair("just.testing", 80));

  // First query will miss the cache.
  EXPECT_EQ(ERR_DNS_CACHE_MISS,
            CreateRequest(info, DEFAULT_PRIORITY)->ResolveFromCache());

  // This time, we fetch normally.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[1]->WaitForResult(), IsOk());

  // Now we should be able to fetch from the cache.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->ResolveFromCache(),
              IsOk());
  EXPECT_TRUE(requests_[2]->HasOneAddress("192.168.1.42", 80));
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->ResolveStaleFromCache(),
              IsOk());
  EXPECT_TRUE(requests_[3]->HasOneAddress("192.168.1.42", 80));
  EXPECT_FALSE(requests_[3]->staleness().is_stale());

  MakeCacheStale();

  proc_->AddRuleForAllFamilies("just.testing", "");
  proc_->SignalMultiple(1u);

  // Now make another query, and return an error this time.
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[4]->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));

  // Now we should be able to fetch from the cache only if we use
  // ResolveStaleFromCache, and the result should be the older good result, not
  // the error.
  EXPECT_EQ(ERR_DNS_CACHE_MISS,
            CreateRequest(info, DEFAULT_PRIORITY)->ResolveFromCache());
  EXPECT_THAT(CreateRequest(info, DEFAULT_PRIORITY)->ResolveStaleFromCache(),
              IsOk());
  EXPECT_TRUE(requests_[6]->HasOneAddress("192.168.1.42", 80));
  EXPECT_TRUE(requests_[6]->staleness().is_stale());
}

// TODO(mgersh): add a test case for errors with positive TTL after
// https://crbug.com/115051 is fixed.

// Test the retry attempts simulating host resolver proc that takes too long.
TEST_F(HostResolverImplTest, MultipleAttempts) {
  // Total number of attempts would be 3 and we want the 3rd attempt to resolve
  // the host. First and second attempt will be forced to sleep until they get
  // word that a resolution has completed. The 3rd resolution attempt will try
  // to get done ASAP, and won't sleep..
  int kAttemptNumberToResolve = 3;
  int kTotalAttempts = 3;

  scoped_refptr<LookupAttemptHostResolverProc> resolver_proc(
      new LookupAttemptHostResolverProc(
          NULL, kAttemptNumberToResolve, kTotalAttempts));

  HostResolverImpl::ProcTaskParams params = DefaultParams(resolver_proc.get());

  // Specify smaller interval for unresponsive_delay_ for HostResolverImpl so
  // that unit test runs faster. For example, this test finishes in 1.5 secs
  // (500ms * 3).
  params.unresponsive_delay = base::TimeDelta::FromMilliseconds(500);

  resolver_.reset(new TestHostResolverImpl(DefaultOptions(), NULL));
  resolver_->set_proc_params_for_test(params);

  // Resolve "host1".
  HostResolver::RequestInfo info(HostPortPair("host1", 70));
  Request* req = CreateRequest(info, DEFAULT_PRIORITY);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));

  // Resolve returns -4 to indicate that 3rd attempt has resolved the host.
  EXPECT_EQ(-4, req->WaitForResult());

  resolver_proc->WaitForAllAttemptsToFinish(
      base::TimeDelta::FromMilliseconds(60000));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(resolver_proc->total_attempts_resolved(), kTotalAttempts);
  EXPECT_EQ(resolver_proc->resolved_attempt_number(), kAttemptNumberToResolve);
}

// If a host resolves to a list that includes 127.0.53.53, this is treated as
// an error. 127.0.53.53 is a localhost address, however it has been given a
// special significance by ICANN to help surface name collision resulting from
// the new gTLDs.
TEST_F(HostResolverImplTest, NameCollisionIcann) {
  proc_->AddRuleForAllFamilies("single", "127.0.53.53");
  proc_->AddRuleForAllFamilies("multiple", "127.0.0.1,127.0.53.53");
  proc_->AddRuleForAllFamilies("ipv6", "::127.0.53.53");
  proc_->AddRuleForAllFamilies("not_reserved1", "53.53.0.127");
  proc_->AddRuleForAllFamilies("not_reserved2", "127.0.53.54");
  proc_->AddRuleForAllFamilies("not_reserved3", "10.0.53.53");
  proc_->SignalMultiple(6u);

  Request* request;

  request = CreateRequest("single");
  EXPECT_THAT(request->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(request->WaitForResult(), IsError(ERR_ICANN_NAME_COLLISION));

  // ERR_ICANN_NAME_COLLISION is cached like any other error, using a
  // fixed TTL for failed entries from proc-based resolver. That said, the
  // fixed TTL is 0, so it will never be cached.
  request = CreateRequest("single");
  EXPECT_THAT(request->ResolveFromCache(), IsError(ERR_DNS_CACHE_MISS));

  request = CreateRequest("multiple");
  EXPECT_THAT(request->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(request->WaitForResult(), IsError(ERR_ICANN_NAME_COLLISION));

  // Resolving an IP literal of 127.0.53.53 however is allowed.
  EXPECT_THAT(CreateRequest("127.0.53.53")->Resolve(), IsOk());

  // Moreover the address should not be recognized when embedded in an IPv6
  // address.
  request = CreateRequest("ipv6");
  EXPECT_THAT(request->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(request->WaitForResult(), IsOk());

  // Try some other IPs which are similar, but NOT an exact match on
  // 127.0.53.53.
  request = CreateRequest("not_reserved1");
  EXPECT_THAT(request->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(request->WaitForResult(), IsOk());

  request = CreateRequest("not_reserved2");
  EXPECT_THAT(request->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(request->WaitForResult(), IsOk());

  request = CreateRequest("not_reserved3");
  EXPECT_THAT(request->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(request->WaitForResult(), IsOk());
}

TEST_F(HostResolverImplTest, IsIPv6Reachable) {
  // The real HostResolverImpl is needed since TestHostResolverImpl will
  // bypass the IPv6 reachability tests.
  resolver_.reset(new HostResolverImpl(DefaultOptions(), nullptr));

  // Verify that two consecutive calls return the same value.
  TestNetLog test_net_log;
  NetLogWithSource net_log =
      NetLogWithSource::Make(&test_net_log, NetLogSourceType::NONE);
  bool result1 = IsIPv6Reachable(net_log);
  bool result2 = IsIPv6Reachable(net_log);
  EXPECT_EQ(result1, result2);

  // Filter reachability check events and verify that there are two of them.
  TestNetLogEntry::List event_list;
  test_net_log.GetEntries(&event_list);
  TestNetLogEntry::List probe_event_list;
  for (const auto& event : event_list) {
    if (event.type ==
        NetLogEventType::HOST_RESOLVER_IMPL_IPV6_REACHABILITY_CHECK) {
      probe_event_list.push_back(event);
    }
  }
  ASSERT_EQ(2U, probe_event_list.size());

  // Verify that the first request was not cached and the second one was.
  bool cached;
  EXPECT_TRUE(probe_event_list[0].GetBooleanValue("cached", &cached));
  EXPECT_FALSE(cached);
  EXPECT_TRUE(probe_event_list[1].GetBooleanValue("cached", &cached));
  EXPECT_TRUE(cached);
}

DnsConfig CreateValidDnsConfig() {
  IPAddress dns_ip(192, 168, 1, 0);
  DnsConfig config;
  config.nameservers.push_back(IPEndPoint(dns_ip, dns_protocol::kDefaultPort));
  EXPECT_TRUE(config.IsValid());
  return config;
}

// Specialized fixture for tests of DnsTask.
class HostResolverImplDnsTest : public HostResolverImplTest {
 public:
  HostResolverImplDnsTest() : dns_client_(NULL) {}

 protected:
  // testing::Test implementation:
  void SetUp() override {
    AddDnsRule("nodomain", dns_protocol::kTypeA, MockDnsClientRule::NODOMAIN,
               false);
    AddDnsRule("nodomain", dns_protocol::kTypeAAAA, MockDnsClientRule::NODOMAIN,
               false);
    AddDnsRule("nx", dns_protocol::kTypeA, MockDnsClientRule::FAIL, false);
    AddDnsRule("nx", dns_protocol::kTypeAAAA, MockDnsClientRule::FAIL, false);
    AddDnsRule("ok", dns_protocol::kTypeA, MockDnsClientRule::OK, false);
    AddDnsRule("ok", dns_protocol::kTypeAAAA, MockDnsClientRule::OK, false);
    AddDnsRule("4ok", dns_protocol::kTypeA, MockDnsClientRule::OK, false);
    AddDnsRule("4ok", dns_protocol::kTypeAAAA, MockDnsClientRule::EMPTY, false);
    AddDnsRule("6ok", dns_protocol::kTypeA, MockDnsClientRule::EMPTY, false);
    AddDnsRule("6ok", dns_protocol::kTypeAAAA, MockDnsClientRule::OK, false);
    AddDnsRule("4nx", dns_protocol::kTypeA, MockDnsClientRule::OK, false);
    AddDnsRule("4nx", dns_protocol::kTypeAAAA, MockDnsClientRule::FAIL, false);
    AddDnsRule("empty", dns_protocol::kTypeA, MockDnsClientRule::EMPTY, false);
    AddDnsRule("empty", dns_protocol::kTypeAAAA, MockDnsClientRule::EMPTY,
               false);

    AddDnsRule("slow_nx", dns_protocol::kTypeA, MockDnsClientRule::FAIL, true);
    AddDnsRule("slow_nx", dns_protocol::kTypeAAAA, MockDnsClientRule::FAIL,
               true);

    AddDnsRule("4slow_ok", dns_protocol::kTypeA, MockDnsClientRule::OK, true);
    AddDnsRule("4slow_ok", dns_protocol::kTypeAAAA, MockDnsClientRule::OK,
               false);
    AddDnsRule("6slow_ok", dns_protocol::kTypeA, MockDnsClientRule::OK, false);
    AddDnsRule("6slow_ok", dns_protocol::kTypeAAAA, MockDnsClientRule::OK,
               true);
    AddDnsRule("4slow_4ok", dns_protocol::kTypeA, MockDnsClientRule::OK, true);
    AddDnsRule("4slow_4ok", dns_protocol::kTypeAAAA, MockDnsClientRule::EMPTY,
               false);
    AddDnsRule("4slow_4timeout", dns_protocol::kTypeA,
               MockDnsClientRule::TIMEOUT, true);
    AddDnsRule("4slow_4timeout", dns_protocol::kTypeAAAA, MockDnsClientRule::OK,
               false);
    AddDnsRule("4slow_6timeout", dns_protocol::kTypeA,
               MockDnsClientRule::OK, true);
    AddDnsRule("4slow_6timeout", dns_protocol::kTypeAAAA,
               MockDnsClientRule::TIMEOUT, false);
    AddDnsRule("4collision", dns_protocol::kTypeA, IPAddress(127, 0, 53, 53),
               false);
    AddDnsRule("4collision", dns_protocol::kTypeAAAA, MockDnsClientRule::EMPTY,
               false);
    AddDnsRule("6collision", dns_protocol::kTypeA, MockDnsClientRule::EMPTY,
               false);
    // This isn't the expected IP for collisions (but looks close to it).
    AddDnsRule("6collision", dns_protocol::kTypeAAAA,
               IPAddress(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 0, 53, 53),
               false);
    CreateResolver();
  }

  void TearDown() override {
    HostResolverImplTest::TearDown();
    ChangeDnsConfig(DnsConfig());
  }

  // HostResolverImplTest implementation:
  void CreateResolverWithLimitsAndParams(
      size_t max_concurrent_resolves,
      const HostResolverImpl::ProcTaskParams& params) override {
    HostResolverImpl::Options options = DefaultOptions();
    options.max_concurrent_resolves = max_concurrent_resolves;
    resolver_.reset(new TestHostResolverImpl(options, NULL));
    resolver_->set_proc_params_for_test(params);
    dns_client_ = new MockDnsClient(DnsConfig(), dns_rules_);
    resolver_->SetDnsClient(std::unique_ptr<DnsClient>(dns_client_));
  }

  // Adds a rule to |dns_rules_|. Must be followed by |CreateResolver| to apply.
  void AddDnsRule(const std::string& prefix,
                  uint16_t qtype,
                  MockDnsClientRule::ResultType result_type,
                  bool delay) {
    return AddDnsRule(prefix, qtype, MockDnsClientRule::Result(result_type),
                      delay);
  }

  void AddDnsRule(const std::string& prefix,
                  uint16_t qtype,
                  const IPAddress& result_ip,
                  bool delay) {
    return AddDnsRule(prefix, qtype, MockDnsClientRule::Result(result_ip),
                      delay);
  }

  void AddDnsRule(const std::string& prefix,
                  uint16_t qtype,
                  MockDnsClientRule::Result result,
                  bool delay) {
    dns_rules_.push_back(MockDnsClientRule(prefix, qtype, result, delay));
  }

  void ChangeDnsConfig(const DnsConfig& config) {
    NetworkChangeNotifier::SetDnsConfig(config);
    // Notification is delivered asynchronously.
    base::RunLoop().RunUntilIdle();
  }

  void SetInitialDnsConfig(const DnsConfig& config) {
    NetworkChangeNotifier::ClearDnsConfigForTesting();
    NetworkChangeNotifier::SetDnsConfig(config);
    // Notification is delivered asynchronously.
    base::RunLoop().RunUntilIdle();
  }

  MockDnsClientRuleList dns_rules_;
  // Owned by |resolver_|.
  MockDnsClient* dns_client_;
};

// TODO(szym): Test AbortAllInProgressJobs due to DnsConfig change.

// TODO(cbentzel): Test a mix of requests with different HostResolverFlags.

// RFC 6761 localhost names should always resolve to loopback.
TEST_F(HostResolverImplDnsTest, LocalhostLookup) {
  // Add a rule resolving localhost names to a non-loopback IP and test
  // that they still resolves to loopback.
  proc_->AddRuleForAllFamilies("foo.localhost", "192.168.1.42");
  proc_->AddRuleForAllFamilies("localhost", "192.168.1.42");
  proc_->AddRuleForAllFamilies("localhost.", "192.168.1.42");

  Request* req0 = CreateRequest("foo.localhost", 80);
  EXPECT_THAT(req0->Resolve(), IsOk());
  EXPECT_TRUE(req0->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(req0->HasAddress("::1", 80));

  Request* req1 = CreateRequest("localhost", 80);
  EXPECT_THAT(req1->Resolve(), IsOk());
  EXPECT_TRUE(req1->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(req1->HasAddress("::1", 80));

  Request* req2 = CreateRequest("localhost.", 80);
  EXPECT_THAT(req2->Resolve(), IsOk());
  EXPECT_TRUE(req2->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(req2->HasAddress("::1", 80));
}

// RFC 6761 localhost names should always resolve to loopback, even if a HOSTS
// file is active.
TEST_F(HostResolverImplDnsTest, LocalhostLookupWithHosts) {
  DnsHosts hosts;
  hosts[DnsHostsKey("localhost", ADDRESS_FAMILY_IPV4)] =
      IPAddress({192, 168, 1, 1});
  hosts[DnsHostsKey("foo.localhost", ADDRESS_FAMILY_IPV4)] =
      IPAddress({192, 168, 1, 2});

  DnsConfig config = CreateValidDnsConfig();
  config.hosts = hosts;
  ChangeDnsConfig(config);

  Request* req1 = CreateRequest("localhost", 80);
  EXPECT_THAT(req1->Resolve(), IsOk());
  EXPECT_TRUE(req1->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(req1->HasAddress("::1", 80));
  EXPECT_FALSE(req1->HasAddress("192.168.1.1", 80));

  Request* req2 = CreateRequest("foo.localhost", 80);
  EXPECT_THAT(req2->Resolve(), IsOk());
  EXPECT_TRUE(req2->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(req2->HasAddress("::1", 80));
  EXPECT_FALSE(req2->HasAddress("192.168.1.2", 80));
}

// Test successful and fallback resolutions in HostResolverImpl::DnsTask.
TEST_F(HostResolverImplDnsTest, DnsTask) {
  proc_->AddRuleForAllFamilies("nx_succeed", "192.168.1.102");
  // All other hostnames will fail in proc_.

  // Initially there is no config, so client should not be invoked.
  EXPECT_THAT(CreateRequest("ok_fail", 80)->Resolve(), IsError(ERR_IO_PENDING));
  proc_->SignalMultiple(requests_.size());

  EXPECT_THAT(requests_[0]->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));

  ChangeDnsConfig(CreateValidDnsConfig());

  EXPECT_EQ(ERR_IO_PENDING, CreateRequest("ok_fail", 80, MEDIUM,
                                          ADDRESS_FAMILY_IPV4)->Resolve());
  EXPECT_EQ(ERR_IO_PENDING, CreateRequest("nx_fail", 80, MEDIUM,
                                          ADDRESS_FAMILY_IPV4)->Resolve());
  EXPECT_EQ(ERR_IO_PENDING, CreateRequest("nx_succeed", 80, MEDIUM,
                                          ADDRESS_FAMILY_IPV4)->Resolve());

  proc_->SignalMultiple(requests_.size());

  for (size_t i = 1; i < requests_.size(); ++i)
    EXPECT_NE(ERR_UNEXPECTED, requests_[i]->WaitForResult()) << i;

  EXPECT_THAT(requests_[1]->result(), IsOk());
  // Resolved by MockDnsClient.
  EXPECT_TRUE(requests_[1]->HasOneAddress("127.0.0.1", 80));

  // Resolutions done by DnsClient are known to have performed a DNS lookup,
  // so they should result in a cache entry with SOURCE_DNS.
  const HostCache::Entry* cache_entry = GetCacheEntry(*requests_[1]);
  ASSERT_NE(nullptr, cache_entry);
  EXPECT_EQ(HostCache::Entry::SOURCE_DNS, cache_entry->source());

  // Fallback to ProcTask.
  EXPECT_THAT(requests_[2]->result(), IsError(ERR_NAME_NOT_RESOLVED));
  EXPECT_THAT(requests_[3]->result(), IsOk());
  EXPECT_TRUE(requests_[3]->HasOneAddress("192.168.1.102", 80));

  // Resolutions done by ProcTask could have performed a DNS lookup, or
  // consulted a HOSTS file, or anything else, so they should result in a cache
  // entry with SOURCE_UNKNOWN.
  cache_entry = GetCacheEntry(*requests_[3]);
  ASSERT_NE(nullptr, cache_entry);
  EXPECT_EQ(HostCache::Entry::SOURCE_UNKNOWN, cache_entry->source());
}

// Test successful and failing resolutions in HostResolverImpl::DnsTask when
// fallback to ProcTask is disabled.
TEST_F(HostResolverImplDnsTest, NoFallbackToProcTask) {
  set_fallback_to_proctask(false);

  proc_->AddRuleForAllFamilies("nx_succeed", "192.168.1.102");
  // All other hostnames will fail in proc_.

  // Set empty DnsConfig.
  ChangeDnsConfig(DnsConfig());
  // Initially there is no config, so client should not be invoked.
  EXPECT_THAT(CreateRequest("ok_fail", 80)->Resolve(), IsError(ERR_IO_PENDING));
  // There is no config, so fallback to ProcTask must work.
  EXPECT_THAT(CreateRequest("nx_succeed", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  proc_->SignalMultiple(requests_.size());

  EXPECT_THAT(requests_[0]->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));
  EXPECT_THAT(requests_[1]->WaitForResult(), IsOk());
  EXPECT_TRUE(requests_[1]->HasOneAddress("192.168.1.102", 80));

  ChangeDnsConfig(CreateValidDnsConfig());

  EXPECT_EQ(ERR_IO_PENDING, CreateRequest("ok_abort", 80, MEDIUM,
                                          ADDRESS_FAMILY_IPV4)->Resolve());
  EXPECT_EQ(ERR_IO_PENDING, CreateRequest("nx_abort", 80, MEDIUM,
                                          ADDRESS_FAMILY_IPV4)->Resolve());

  // Simulate the case when the preference or policy has disabled the DNS client
  // causing AbortDnsTasks.
  resolver_->SetDnsClient(
      std::unique_ptr<DnsClient>(new MockDnsClient(DnsConfig(), dns_rules_)));
  ChangeDnsConfig(CreateValidDnsConfig());

  // First request is resolved by MockDnsClient, others should fail due to
  // disabled fallback to ProcTask.
  EXPECT_EQ(ERR_IO_PENDING, CreateRequest("ok_fail", 80, MEDIUM,
                                          ADDRESS_FAMILY_IPV4)->Resolve());
  EXPECT_EQ(ERR_IO_PENDING, CreateRequest("nx_fail", 80, MEDIUM,
                                          ADDRESS_FAMILY_IPV4)->Resolve());
  proc_->SignalMultiple(requests_.size());

  // Aborted due to Network Change.
  EXPECT_THAT(requests_[2]->WaitForResult(), IsError(ERR_NETWORK_CHANGED));
  EXPECT_THAT(requests_[3]->WaitForResult(), IsError(ERR_NETWORK_CHANGED));
  // Resolved by MockDnsClient.
  EXPECT_THAT(requests_[4]->WaitForResult(), IsOk());
  EXPECT_TRUE(requests_[4]->HasOneAddress("127.0.0.1", 80));
  // Fallback to ProcTask is disabled.
  EXPECT_THAT(requests_[5]->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));
}

// Test behavior of OnDnsTaskFailure when Job is aborted.
TEST_F(HostResolverImplDnsTest, OnDnsTaskFailureAbortedJob) {
  ChangeDnsConfig(CreateValidDnsConfig());
  EXPECT_THAT(CreateRequest("nx_abort", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  // Abort all jobs here.
  CreateResolver();
  proc_->SignalMultiple(requests_.size());
  // Run to completion.
  base::RunLoop().RunUntilIdle();  // Notification happens async.
  // It shouldn't crash during OnDnsTaskFailure callbacks.
  EXPECT_THAT(requests_[0]->result(), IsError(ERR_IO_PENDING));

  // Repeat test with Fallback to ProcTask disabled
  set_fallback_to_proctask(false);
  ChangeDnsConfig(CreateValidDnsConfig());
  EXPECT_THAT(CreateRequest("nx_abort", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  // Abort all jobs here.
  CreateResolver();
  // Run to completion.
  base::RunLoop().RunUntilIdle();  // Notification happens async.
  // It shouldn't crash during OnDnsTaskFailure callbacks.
  EXPECT_THAT(requests_[1]->result(), IsError(ERR_IO_PENDING));
}

TEST_F(HostResolverImplDnsTest, DnsTaskUnspec) {
  ChangeDnsConfig(CreateValidDnsConfig());

  proc_->AddRuleForAllFamilies("4nx", "192.168.1.101");
  // All other hostnames will fail in proc_.

  EXPECT_THAT(CreateRequest("ok", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("4ok", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("6ok", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("4nx", 80)->Resolve(), IsError(ERR_IO_PENDING));

  proc_->SignalMultiple(requests_.size());

  for (size_t i = 0; i < requests_.size(); ++i)
    EXPECT_EQ(OK, requests_[i]->WaitForResult()) << i;

  EXPECT_EQ(2u, requests_[0]->NumberOfAddresses());
  EXPECT_TRUE(requests_[0]->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(requests_[0]->HasAddress("::1", 80));
  EXPECT_EQ(1u, requests_[1]->NumberOfAddresses());
  EXPECT_TRUE(requests_[1]->HasAddress("127.0.0.1", 80));
  EXPECT_EQ(1u, requests_[2]->NumberOfAddresses());
  EXPECT_TRUE(requests_[2]->HasAddress("::1", 80));
  EXPECT_EQ(1u, requests_[3]->NumberOfAddresses());
  EXPECT_TRUE(requests_[3]->HasAddress("192.168.1.101", 80));
}

TEST_F(HostResolverImplDnsTest, NameCollisionIcann) {
  ChangeDnsConfig(CreateValidDnsConfig());

  // When the resolver returns an A record with 127.0.53.53 it should be mapped
  // to a special error.
  EXPECT_THAT(CreateRequest("4collision", 80)->Resolve(),
              IsError(ERR_IO_PENDING));

  EXPECT_THAT(requests_[0]->WaitForResult(), IsError(ERR_ICANN_NAME_COLLISION));

  // When the resolver returns an AAAA record with ::127.0.53.53 it should
  // work just like any other IP. (Despite having the same suffix, it is not
  // considered special)
  EXPECT_THAT(CreateRequest("6collision", 80)->Resolve(),
              IsError(ERR_IO_PENDING));

  EXPECT_THAT(requests_[1]->WaitForResult(), IsError(OK));
  EXPECT_TRUE(requests_[1]->HasAddress("::127.0.53.53", 80));

  // The mock responses for 4collision (and 6collision) have a TTL of 1 day.
  // Test whether the ERR_ICANN_NAME_COLLISION failure was cached.
  // On the one hand caching the failure makes sense, as the error is derived
  // from the IP in the response. However for consistency with the the proc-
  // based implementation the TTL is unused.
  EXPECT_THAT(CreateRequest("4collision", 80)->ResolveFromCache(),
              IsError(ERR_DNS_CACHE_MISS));
}

TEST_F(HostResolverImplDnsTest, ServeFromHosts) {
  // Initially, use empty HOSTS file.
  DnsConfig config = CreateValidDnsConfig();
  ChangeDnsConfig(config);

  proc_->AddRuleForAllFamilies(std::string(),
                               std::string());  // Default to failures.
  proc_->SignalMultiple(1u);  // For the first request which misses.

  Request* req0 = CreateRequest("nx_ipv4", 80);
  EXPECT_THAT(req0->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(req0->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));

  IPAddress local_ipv4 = IPAddress::IPv4Localhost();
  IPAddress local_ipv6 = IPAddress::IPv6Localhost();

  DnsHosts hosts;
  hosts[DnsHostsKey("nx_ipv4", ADDRESS_FAMILY_IPV4)] = local_ipv4;
  hosts[DnsHostsKey("nx_ipv6", ADDRESS_FAMILY_IPV6)] = local_ipv6;
  hosts[DnsHostsKey("nx_both", ADDRESS_FAMILY_IPV4)] = local_ipv4;
  hosts[DnsHostsKey("nx_both", ADDRESS_FAMILY_IPV6)] = local_ipv6;

  // Update HOSTS file.
  config.hosts = hosts;
  ChangeDnsConfig(config);

  Request* req1 = CreateRequest("nx_ipv4", 80);
  EXPECT_THAT(req1->Resolve(), IsOk());
  EXPECT_TRUE(req1->HasOneAddress("127.0.0.1", 80));

  Request* req2 = CreateRequest("nx_ipv6", 80);
  EXPECT_THAT(req2->Resolve(), IsOk());
  EXPECT_TRUE(req2->HasOneAddress("::1", 80));

  Request* req3 = CreateRequest("nx_both", 80);
  EXPECT_THAT(req3->Resolve(), IsOk());
  EXPECT_TRUE(req3->HasAddress("127.0.0.1", 80) &&
              req3->HasAddress("::1", 80));

  // Requests with specified AddressFamily.
  Request* req4 = CreateRequest("nx_ipv4", 80, MEDIUM, ADDRESS_FAMILY_IPV4);
  EXPECT_THAT(req4->Resolve(), IsOk());
  EXPECT_TRUE(req4->HasOneAddress("127.0.0.1", 80));

  Request* req5 = CreateRequest("nx_ipv6", 80, MEDIUM, ADDRESS_FAMILY_IPV6);
  EXPECT_THAT(req5->Resolve(), IsOk());
  EXPECT_TRUE(req5->HasOneAddress("::1", 80));

  // Request with upper case.
  Request* req6 = CreateRequest("nx_IPV4", 80);
  EXPECT_THAT(req6->Resolve(), IsOk());
  EXPECT_TRUE(req6->HasOneAddress("127.0.0.1", 80));
}

TEST_F(HostResolverImplDnsTest, CacheHostsLookupOnConfigChange) {
  // Only allow 1 resolution at a time, so that the second lookup is queued and
  // occurs when the DNS config changes.
  CreateResolverWithLimitsAndParams(1u, DefaultParams(proc_.get()));
  DnsConfig config = CreateValidDnsConfig();
  ChangeDnsConfig(config);

  proc_->AddRuleForAllFamilies(std::string(),
                               std::string());  // Default to failures.
  proc_->SignalMultiple(1u);  // For the first request which fails.

  Request* req1 = CreateRequest("nx_ipv4", 80);
  EXPECT_THAT(req1->Resolve(), IsError(ERR_IO_PENDING));
  Request* req2 = CreateRequest("nx_ipv6", 80);
  EXPECT_THAT(req2->Resolve(), IsError(ERR_IO_PENDING));

  DnsHosts hosts;
  hosts[DnsHostsKey("nx_ipv4", ADDRESS_FAMILY_IPV4)] =
      IPAddress::IPv4Localhost();
  hosts[DnsHostsKey("nx_ipv6", ADDRESS_FAMILY_IPV6)] =
      IPAddress::IPv6Localhost();

  config.hosts = hosts;
  ChangeDnsConfig(config);

  EXPECT_THAT(req1->WaitForResult(), IsError(ERR_NETWORK_CHANGED));
  EXPECT_THAT(req2->WaitForResult(), IsOk());
  EXPECT_TRUE(req2->HasOneAddress("::1", 80));

  // Resolutions done by consulting the HOSTS file when the DNS config changes
  // should result in a cache entry with SOURCE_HOSTS.
  const HostCache::Entry* cache_entry = GetCacheEntry(*req2);
  ASSERT_THAT(cache_entry, NotNull());
  EXPECT_EQ(HostCache::Entry::SOURCE_HOSTS, cache_entry->source());
}

TEST_F(HostResolverImplDnsTest, BypassDnsTask) {
  ChangeDnsConfig(CreateValidDnsConfig());

  proc_->AddRuleForAllFamilies(std::string(),
                               std::string());  // Default to failures.

  EXPECT_THAT(CreateRequest("ok.local", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("ok.local.", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("oklocal", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("oklocal.", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("ok", 80)->Resolve(), IsError(ERR_IO_PENDING));

  proc_->SignalMultiple(requests_.size());

  for (size_t i = 0; i < 2; ++i)
    EXPECT_EQ(ERR_NAME_NOT_RESOLVED, requests_[i]->WaitForResult()) << i;

  for (size_t i = 2; i < requests_.size(); ++i)
    EXPECT_EQ(OK, requests_[i]->WaitForResult()) << i;
}

TEST_F(HostResolverImplDnsTest, SystemOnlyBypassesDnsTask) {
  ChangeDnsConfig(CreateValidDnsConfig());

  proc_->AddRuleForAllFamilies(std::string(), std::string());

  HostResolver::RequestInfo info_bypass(HostPortPair("ok", 80));
  info_bypass.set_host_resolver_flags(HOST_RESOLVER_SYSTEM_ONLY);
  EXPECT_THAT(CreateRequest(info_bypass, MEDIUM)->Resolve(),
              IsError(ERR_IO_PENDING));

  HostResolver::RequestInfo info(HostPortPair("ok", 80));
  EXPECT_THAT(CreateRequest(info, MEDIUM)->Resolve(), IsError(ERR_IO_PENDING));

  proc_->SignalMultiple(requests_.size());

  EXPECT_THAT(requests_[0]->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));
  EXPECT_THAT(requests_[1]->WaitForResult(), IsOk());
}

TEST_F(HostResolverImplDnsTest, DisableDnsClientOnPersistentFailure) {
  ChangeDnsConfig(CreateValidDnsConfig());

  proc_->AddRuleForAllFamilies(std::string(),
                               std::string());  // Default to failures.

  // Check that DnsTask works.
  Request* req = CreateRequest("ok_1", 80);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(req->WaitForResult(), IsOk());

  for (unsigned i = 0; i < maximum_dns_failures(); ++i) {
    // Use custom names to require separate Jobs.
    std::string hostname = base::StringPrintf("nx_%u", i);
    // Ensure fallback to ProcTask succeeds.
    proc_->AddRuleForAllFamilies(hostname, "192.168.1.101");
    EXPECT_EQ(ERR_IO_PENDING, CreateRequest(hostname, 80)->Resolve()) << i;
  }

  proc_->SignalMultiple(requests_.size());

  for (size_t i = 0; i < requests_.size(); ++i)
    EXPECT_EQ(OK, requests_[i]->WaitForResult()) << i;

  ASSERT_FALSE(proc_->HasBlockedRequests());

  // DnsTask should be disabled by now.
  req = CreateRequest("ok_2", 80);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));
  proc_->SignalMultiple(1u);
  EXPECT_THAT(req->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));

  // Check that it is re-enabled after DNS change.
  ChangeDnsConfig(CreateValidDnsConfig());
  req = CreateRequest("ok_3", 80);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(req->WaitForResult(), IsOk());
}

TEST_F(HostResolverImplDnsTest, DontDisableDnsClientOnSporadicFailure) {
  ChangeDnsConfig(CreateValidDnsConfig());

  // |proc_| defaults to successes.

  // 20 failures interleaved with 20 successes.
  for (unsigned i = 0; i < 40; ++i) {
    // Use custom names to require separate Jobs.
    std::string hostname = (i % 2) == 0 ? base::StringPrintf("nx_%u", i)
                                        : base::StringPrintf("ok_%u", i);
    EXPECT_EQ(ERR_IO_PENDING, CreateRequest(hostname, 80)->Resolve()) << i;
  }

  proc_->SignalMultiple(requests_.size());

  for (size_t i = 0; i < requests_.size(); ++i)
    EXPECT_EQ(OK, requests_[i]->WaitForResult()) << i;

  // Make |proc_| default to failures.
  proc_->AddRuleForAllFamilies(std::string(), std::string());

  // DnsTask should still be enabled.
  Request* req = CreateRequest("ok_last", 80);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(req->WaitForResult(), IsOk());
}

// Confirm that resolving "localhost" is unrestricted even if there are no
// global IPv6 address. See SystemHostResolverCall for rationale.
// Test both the DnsClient and system host resolver paths.
TEST_F(HostResolverImplDnsTest, DualFamilyLocalhost) {
  // Use regular SystemHostResolverCall!
  scoped_refptr<HostResolverProc> proc(new SystemHostResolverProc());
  resolver_.reset(new TestHostResolverImpl(DefaultOptions(), NULL, false));
  resolver_->set_proc_params_for_test(DefaultParams(proc.get()));

  resolver_->SetDnsClient(
      std::unique_ptr<DnsClient>(new MockDnsClient(DnsConfig(), dns_rules_)));

  // Get the expected output.
  AddressList addrlist;
  int rv = proc->Resolve("localhost", ADDRESS_FAMILY_UNSPECIFIED, 0, &addrlist,
                         NULL);
  if (rv != OK)
    return;

  for (unsigned i = 0; i < addrlist.size(); ++i)
    LOG(WARNING) << addrlist[i].ToString();

  bool saw_ipv4 = AddressListContains(addrlist, "127.0.0.1", 0);
  bool saw_ipv6 = AddressListContains(addrlist, "::1", 0);
  if (!saw_ipv4 && !saw_ipv6)
    return;

  // Try without DnsClient.
  DnsConfig config = CreateValidDnsConfig();
  config.use_local_ipv6 = false;
  ChangeDnsConfig(config);
  HostResolver::RequestInfo info_proc(HostPortPair("localhost", 80));
  info_proc.set_address_family(ADDRESS_FAMILY_UNSPECIFIED);
  info_proc.set_host_resolver_flags(HOST_RESOLVER_SYSTEM_ONLY);
  Request* req = CreateRequest(info_proc, DEFAULT_PRIORITY);

  EXPECT_THAT(req->Resolve(), IsOk());

  EXPECT_TRUE(req->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(req->HasAddress("::1", 80));

  // Configure DnsClient with dual-host HOSTS file.
  DnsConfig config_hosts = CreateValidDnsConfig();
  DnsHosts hosts;
  IPAddress local_ipv4 = IPAddress::IPv4Localhost();
  IPAddress local_ipv6 = IPAddress::IPv6Localhost();
  if (saw_ipv4)
    hosts[DnsHostsKey("localhost", ADDRESS_FAMILY_IPV4)] = local_ipv4;
  if (saw_ipv6)
    hosts[DnsHostsKey("localhost", ADDRESS_FAMILY_IPV6)] = local_ipv6;
  config_hosts.hosts = hosts;

  ChangeDnsConfig(config_hosts);
  HostResolver::RequestInfo info_hosts(HostPortPair("localhost", 80));
  info_hosts.set_address_family(ADDRESS_FAMILY_UNSPECIFIED);
  req = CreateRequest(info_hosts, DEFAULT_PRIORITY);
  // Expect synchronous resolution from DnsHosts.
  EXPECT_THAT(req->Resolve(), IsOk());

  // Localhost names always resolve to IPv4 and IPv6, regardless of the content
  // written into the HOSTS file above based on the results of the
  // SystemHostResolverCall at the top of this test.
  EXPECT_TRUE(req->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(req->HasAddress("::1", 80));
}

// Cancel a request with a single DNS transaction active.
TEST_F(HostResolverImplDnsTest, CancelWithOneTransactionActive) {
  ChangeDnsConfig(CreateValidDnsConfig());

  EXPECT_EQ(ERR_IO_PENDING,
            CreateRequest("ok", 80, MEDIUM, ADDRESS_FAMILY_IPV4)->Resolve());
  EXPECT_EQ(1u, num_running_dispatcher_jobs());
  requests_[0]->Cancel();

  // Dispatcher state checked in TearDown.
}

// Cancel a request with a single DNS transaction active and another pending.
TEST_F(HostResolverImplDnsTest, CancelWithOneTransactionActiveOnePending) {
  CreateSerialResolver();
  ChangeDnsConfig(CreateValidDnsConfig());

  EXPECT_THAT(CreateRequest("ok", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_EQ(1u, num_running_dispatcher_jobs());
  requests_[0]->Cancel();

  // Dispatcher state checked in TearDown.
}

// Cancel a request with two DNS transactions active.
TEST_F(HostResolverImplDnsTest, CancelWithTwoTransactionsActive) {
  ChangeDnsConfig(CreateValidDnsConfig());

  EXPECT_THAT(CreateRequest("ok", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_EQ(2u, num_running_dispatcher_jobs());
  requests_[0]->Cancel();

  // Dispatcher state checked in TearDown.
}

// Delete a resolver with some active requests and some queued requests.
TEST_F(HostResolverImplDnsTest, DeleteWithActiveTransactions) {
  // At most 10 Jobs active at once.
  CreateResolverWithLimitsAndParams(10u, DefaultParams(proc_.get()));

  ChangeDnsConfig(CreateValidDnsConfig());

  // First active job is an IPv4 request.
  EXPECT_EQ(ERR_IO_PENDING, CreateRequest("ok", 80, MEDIUM,
                                          ADDRESS_FAMILY_IPV4)->Resolve());

  // Add 10 more DNS lookups for different hostnames.  First 4 should have two
  // active jobs, next one has a single active job, and one pending.  Others
  // should all be queued.
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(ERR_IO_PENDING, CreateRequest(
        base::StringPrintf("ok%i", i))->Resolve());
  }
  EXPECT_EQ(10u, num_running_dispatcher_jobs());

  resolver_.reset();
}

// Cancel a request with only the IPv6 transaction active.
TEST_F(HostResolverImplDnsTest, CancelWithIPv6TransactionActive) {
  ChangeDnsConfig(CreateValidDnsConfig());

  EXPECT_THAT(CreateRequest("6slow_ok", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_EQ(2u, num_running_dispatcher_jobs());

  // The IPv4 request should complete, the IPv6 request is still pending.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1u, num_running_dispatcher_jobs());
  requests_[0]->Cancel();

  // Dispatcher state checked in TearDown.
}

// Cancel a request with only the IPv4 transaction pending.
TEST_F(HostResolverImplDnsTest, CancelWithIPv4TransactionPending) {
  set_fallback_to_proctask(false);
  ChangeDnsConfig(CreateValidDnsConfig());

  EXPECT_THAT(CreateRequest("4slow_ok", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_EQ(2u, num_running_dispatcher_jobs());

  // The IPv6 request should complete, the IPv4 request is still pending.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1u, num_running_dispatcher_jobs());

  requests_[0]->Cancel();
}

// Test cases where AAAA completes first.
TEST_F(HostResolverImplDnsTest, AAAACompletesFirst) {
  set_fallback_to_proctask(false);
  ChangeDnsConfig(CreateValidDnsConfig());

  EXPECT_THAT(CreateRequest("4slow_ok", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("4slow_4ok", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("4slow_4timeout", 80)->Resolve(),
              IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("4slow_6timeout", 80)->Resolve(),
              IsError(ERR_IO_PENDING));

  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(requests_[0]->completed());
  EXPECT_FALSE(requests_[1]->completed());
  EXPECT_FALSE(requests_[2]->completed());
  // The IPv6 of the third request should have failed and resulted in cancelling
  // the IPv4 request.
  EXPECT_TRUE(requests_[3]->completed());
  EXPECT_THAT(requests_[3]->result(), IsError(ERR_DNS_TIMED_OUT));
  EXPECT_EQ(3u, num_running_dispatcher_jobs());

  dns_client_->CompleteDelayedTransactions();
  EXPECT_TRUE(requests_[0]->completed());
  EXPECT_THAT(requests_[0]->result(), IsOk());
  EXPECT_EQ(2u, requests_[0]->NumberOfAddresses());
  EXPECT_TRUE(requests_[0]->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(requests_[0]->HasAddress("::1", 80));

  EXPECT_TRUE(requests_[1]->completed());
  EXPECT_THAT(requests_[1]->result(), IsOk());
  EXPECT_EQ(1u, requests_[1]->NumberOfAddresses());
  EXPECT_TRUE(requests_[1]->HasAddress("127.0.0.1", 80));

  EXPECT_TRUE(requests_[2]->completed());
  EXPECT_THAT(requests_[2]->result(), IsError(ERR_DNS_TIMED_OUT));
}

// Test the case where only a single transaction slot is available.
TEST_F(HostResolverImplDnsTest, SerialResolver) {
  CreateSerialResolver();
  set_fallback_to_proctask(false);
  ChangeDnsConfig(CreateValidDnsConfig());

  EXPECT_THAT(CreateRequest("ok", 80)->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_EQ(1u, num_running_dispatcher_jobs());

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(requests_[0]->completed());
  EXPECT_THAT(requests_[0]->result(), IsOk());
  EXPECT_EQ(2u, requests_[0]->NumberOfAddresses());
  EXPECT_TRUE(requests_[0]->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(requests_[0]->HasAddress("::1", 80));
}

// Test the case where the AAAA query is started when another transaction
// completes.
TEST_F(HostResolverImplDnsTest, AAAAStartsAfterOtherJobFinishes) {
  CreateResolverWithLimitsAndParams(2u, DefaultParams(proc_.get()));
  set_fallback_to_proctask(false);
  ChangeDnsConfig(CreateValidDnsConfig());

  EXPECT_EQ(ERR_IO_PENDING, CreateRequest("ok", 80, MEDIUM,
                                          ADDRESS_FAMILY_IPV4)->Resolve());
  EXPECT_EQ(ERR_IO_PENDING,
            CreateRequest("4slow_ok", 80, MEDIUM)->Resolve());
  // An IPv4 request should have been started pending for each job.
  EXPECT_EQ(2u, num_running_dispatcher_jobs());

  // Request 0's IPv4 request should complete, starting Request 1's IPv6
  // request, which should also complete.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1u, num_running_dispatcher_jobs());
  EXPECT_TRUE(requests_[0]->completed());
  EXPECT_FALSE(requests_[1]->completed());

  dns_client_->CompleteDelayedTransactions();
  EXPECT_TRUE(requests_[1]->completed());
  EXPECT_THAT(requests_[1]->result(), IsOk());
  EXPECT_EQ(2u, requests_[1]->NumberOfAddresses());
  EXPECT_TRUE(requests_[1]->HasAddress("127.0.0.1", 80));
  EXPECT_TRUE(requests_[1]->HasAddress("::1", 80));
}

// Tests the case that a Job with a single transaction receives an empty address
// list, triggering fallback to ProcTask.
TEST_F(HostResolverImplDnsTest, IPv4EmptyFallback) {
  ChangeDnsConfig(CreateValidDnsConfig());
  proc_->AddRuleForAllFamilies("empty_fallback", "192.168.0.1");
  proc_->SignalMultiple(1u);
  EXPECT_EQ(ERR_IO_PENDING,
            CreateRequest("empty_fallback", 80, MEDIUM,
                          ADDRESS_FAMILY_IPV4)->Resolve());
  EXPECT_THAT(requests_[0]->WaitForResult(), IsOk());
  EXPECT_TRUE(requests_[0]->HasOneAddress("192.168.0.1", 80));
}

// Tests the case that a Job with two transactions receives two empty address
// lists, triggering fallback to ProcTask.
TEST_F(HostResolverImplDnsTest, UnspecEmptyFallback) {
  ChangeDnsConfig(CreateValidDnsConfig());
  proc_->AddRuleForAllFamilies("empty_fallback", "192.168.0.1");
  proc_->SignalMultiple(1u);
  EXPECT_EQ(ERR_IO_PENDING,
            CreateRequest("empty_fallback", 80, MEDIUM,
                          ADDRESS_FAMILY_UNSPECIFIED)->Resolve());
  EXPECT_THAT(requests_[0]->WaitForResult(), IsOk());
  EXPECT_TRUE(requests_[0]->HasOneAddress("192.168.0.1", 80));
}

// Tests getting a new invalid DnsConfig while there are active DnsTasks.
TEST_F(HostResolverImplDnsTest, InvalidDnsConfigWithPendingRequests) {
  // At most 3 jobs active at once.  This number is important, since we want to
  // make sure that aborting the first HostResolverImpl::Job does not trigger
  // another DnsTransaction on the second Job when it releases its second
  // prioritized dispatcher slot.
  CreateResolverWithLimitsAndParams(3u, DefaultParams(proc_.get()));

  ChangeDnsConfig(CreateValidDnsConfig());

  proc_->AddRuleForAllFamilies("slow_nx1", "192.168.0.1");
  proc_->AddRuleForAllFamilies("slow_nx2", "192.168.0.2");
  proc_->AddRuleForAllFamilies("ok", "192.168.0.3");

  // First active job gets two slots.
  EXPECT_THAT(CreateRequest("slow_nx1")->Resolve(), IsError(ERR_IO_PENDING));
  // Next job gets one slot, and waits on another.
  EXPECT_THAT(CreateRequest("slow_nx2")->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(CreateRequest("ok")->Resolve(), IsError(ERR_IO_PENDING));

  EXPECT_EQ(3u, num_running_dispatcher_jobs());

  // Clear DNS config.  Two in-progress jobs should be aborted, and the next one
  // should use a ProcTask.
  ChangeDnsConfig(DnsConfig());
  EXPECT_THAT(requests_[0]->WaitForResult(), IsError(ERR_NETWORK_CHANGED));
  EXPECT_THAT(requests_[1]->WaitForResult(), IsError(ERR_NETWORK_CHANGED));

  // Finish up the third job.  Should bypass the DnsClient, and get its results
  // from MockHostResolverProc.
  EXPECT_FALSE(requests_[2]->completed());
  proc_->SignalMultiple(1u);
  EXPECT_THAT(requests_[2]->WaitForResult(), IsOk());
  EXPECT_TRUE(requests_[2]->HasOneAddress("192.168.0.3", 80));
}

// Test that initial DNS config read signals do not abort pending requests when
// using DnsClient.
TEST_F(HostResolverImplDnsTest, DontAbortOnInitialDNSConfigRead) {
  // DnsClient is enabled, but there's no DnsConfig, so the request should start
  // using ProcTask.
  Request* req = CreateRequest("host1", 70);
  EXPECT_THAT(req->Resolve(), IsError(ERR_IO_PENDING));

  EXPECT_TRUE(proc_->WaitFor(1u));
  // Send the initial config read signal, with a valid config.
  SetInitialDnsConfig(CreateValidDnsConfig());
  proc_->SignalAll();

  EXPECT_THAT(req->WaitForResult(), IsOk());
}

// Tests the case that DnsClient is automatically disabled due to failures
// while there are active DnsTasks.
TEST_F(HostResolverImplDnsTest,
       AutomaticallyDisableDnsClientWithPendingRequests) {
  // Trying different limits is important for this test:  Different limits
  // result in different behavior when aborting in-progress DnsTasks.  Having
  // a DnsTask that has one job active and one in the queue when another job
  // occupying two slots has its DnsTask aborted is the case most likely to run
  // into problems.
  for (size_t limit = 1u; limit < 6u; ++limit) {
    CreateResolverWithLimitsAndParams(limit, DefaultParams(proc_.get()));

    ChangeDnsConfig(CreateValidDnsConfig());

    // Queue up enough failures to disable DnsTasks.  These will all fall back
    // to ProcTasks, and succeed there.
    for (unsigned i = 0u; i < maximum_dns_failures(); ++i) {
      std::string host = base::StringPrintf("nx%u", i);
      proc_->AddRuleForAllFamilies(host, "192.168.0.1");
      EXPECT_THAT(CreateRequest(host)->Resolve(), IsError(ERR_IO_PENDING));
    }

    // These requests should all bypass DnsTasks, due to the above failures,
    // so should end up using ProcTasks.
    proc_->AddRuleForAllFamilies("slow_ok1", "192.168.0.2");
    EXPECT_THAT(CreateRequest("slow_ok1")->Resolve(), IsError(ERR_IO_PENDING));
    proc_->AddRuleForAllFamilies("slow_ok2", "192.168.0.3");
    EXPECT_THAT(CreateRequest("slow_ok2")->Resolve(), IsError(ERR_IO_PENDING));
    proc_->AddRuleForAllFamilies("slow_ok3", "192.168.0.4");
    EXPECT_THAT(CreateRequest("slow_ok3")->Resolve(), IsError(ERR_IO_PENDING));
    proc_->SignalMultiple(maximum_dns_failures() + 3);

    for (size_t i = 0u; i < maximum_dns_failures(); ++i) {
      EXPECT_THAT(requests_[i]->WaitForResult(), IsOk());
      EXPECT_TRUE(requests_[i]->HasOneAddress("192.168.0.1", 80));
    }

    EXPECT_THAT(requests_[maximum_dns_failures()]->WaitForResult(), IsOk());
    EXPECT_TRUE(requests_[maximum_dns_failures()]->HasOneAddress(
                    "192.168.0.2", 80));
    EXPECT_THAT(requests_[maximum_dns_failures() + 1]->WaitForResult(), IsOk());
    EXPECT_TRUE(requests_[maximum_dns_failures() + 1]->HasOneAddress(
                    "192.168.0.3", 80));
    EXPECT_THAT(requests_[maximum_dns_failures() + 2]->WaitForResult(), IsOk());
    EXPECT_TRUE(requests_[maximum_dns_failures() + 2]->HasOneAddress(
                    "192.168.0.4", 80));
    requests_.clear();
  }
}

// Tests a call to SetDnsClient while there are active DnsTasks.
TEST_F(HostResolverImplDnsTest, ManuallyDisableDnsClientWithPendingRequests) {
  // At most 3 jobs active at once.  This number is important, since we want to
  // make sure that aborting the first HostResolverImpl::Job does not trigger
  // another DnsTransaction on the second Job when it releases its second
  // prioritized dispatcher slot.
  CreateResolverWithLimitsAndParams(3u, DefaultParams(proc_.get()));

  ChangeDnsConfig(CreateValidDnsConfig());

  proc_->AddRuleForAllFamilies("slow_ok1", "192.168.0.1");
  proc_->AddRuleForAllFamilies("slow_ok2", "192.168.0.2");
  proc_->AddRuleForAllFamilies("ok", "192.168.0.3");

  // First active job gets two slots.
  EXPECT_THAT(CreateRequest("slow_ok1")->Resolve(), IsError(ERR_IO_PENDING));
  // Next job gets one slot, and waits on another.
  EXPECT_THAT(CreateRequest("slow_ok2")->Resolve(), IsError(ERR_IO_PENDING));
  // Next one is queued.
  EXPECT_THAT(CreateRequest("ok")->Resolve(), IsError(ERR_IO_PENDING));

  EXPECT_EQ(3u, num_running_dispatcher_jobs());

  // Clear DnsClient.  The two in-progress jobs should fall back to a ProcTask,
  // and the next one should be started with a ProcTask.
  resolver_->SetDnsClient(std::unique_ptr<DnsClient>());

  // All three in-progress requests should now be running a ProcTask.
  EXPECT_EQ(3u, num_running_dispatcher_jobs());
  proc_->SignalMultiple(3u);

  EXPECT_THAT(requests_[0]->WaitForResult(), IsOk());
  EXPECT_TRUE(requests_[0]->HasOneAddress("192.168.0.1", 80));
  EXPECT_THAT(requests_[1]->WaitForResult(), IsOk());
  EXPECT_TRUE(requests_[1]->HasOneAddress("192.168.0.2", 80));
  EXPECT_THAT(requests_[2]->WaitForResult(), IsOk());
  EXPECT_TRUE(requests_[2]->HasOneAddress("192.168.0.3", 80));
}

TEST_F(HostResolverImplDnsTest, NoIPv6OnWifi) {
  // CreateSerialResolver will destroy the current resolver_ which will attempt
  // to remove itself from the NetworkChangeNotifier. If this happens after a
  // new NetworkChangeNotifier is active, then it will not remove itself from
  // the old NetworkChangeNotifier which is a potential use-after-free.
  resolver_ = nullptr;
  test::ScopedMockNetworkChangeNotifier notifier;
  CreateSerialResolver();  // To guarantee order of resolutions.
  resolver_->SetNoIPv6OnWifi(true);

  notifier.mock_network_change_notifier()->SetConnectionType(
      NetworkChangeNotifier::CONNECTION_WIFI);
  // Needed so IPv6 availability check isn't skipped.
  ChangeDnsConfig(CreateValidDnsConfig());

  proc_->AddRule("h1", ADDRESS_FAMILY_UNSPECIFIED, "::3");
  proc_->AddRule("h1", ADDRESS_FAMILY_IPV4, "1.0.0.1");
  proc_->AddRule("h1", ADDRESS_FAMILY_IPV6, "::2");

  CreateRequest("h1", 80, MEDIUM, ADDRESS_FAMILY_UNSPECIFIED);
  CreateRequest("h1", 80, MEDIUM, ADDRESS_FAMILY_IPV4);
  CreateRequest("h1", 80, MEDIUM, ADDRESS_FAMILY_IPV6);

  // Start all of the requests.
  for (size_t i = 0u; i < requests_.size(); ++i) {
    EXPECT_THAT(requests_[i]->Resolve(), IsError(ERR_IO_PENDING)) << i;
  }

  proc_->SignalMultiple(requests_.size());

  // Wait for all the requests to complete.
  for (size_t i = 0u; i < requests_.size(); ++i) {
    EXPECT_THAT(requests_[i]->WaitForResult(), IsOk()) << i;
  }

  // Since the requests all had the same priority and we limited the thread
  // count to 1, they should have completed in the same order as they were
  // requested.
  MockHostResolverProc::CaptureList capture_list = proc_->GetCaptureList();
  ASSERT_EQ(3u, capture_list.size());

  EXPECT_EQ("h1", capture_list[0].hostname);
  EXPECT_EQ(ADDRESS_FAMILY_IPV4, capture_list[0].address_family);

  EXPECT_EQ("h1", capture_list[1].hostname);
  EXPECT_EQ(ADDRESS_FAMILY_IPV4, capture_list[1].address_family);

  EXPECT_EQ("h1", capture_list[2].hostname);
  EXPECT_EQ(ADDRESS_FAMILY_IPV6, capture_list[2].address_family);

  // Now check that the correct resolved IP addresses were returned.
  EXPECT_TRUE(requests_[0]->HasOneAddress("1.0.0.1", 80));
  EXPECT_TRUE(requests_[1]->HasOneAddress("1.0.0.1", 80));
  EXPECT_TRUE(requests_[2]->HasOneAddress("::2", 80));

  // Now repeat the test on non-wifi to check that IPv6 is used as normal
  // after the network changes.
  notifier.mock_network_change_notifier()->SetConnectionType(
      NetworkChangeNotifier::CONNECTION_4G);
  base::RunLoop().RunUntilIdle();  // Wait for NetworkChangeNotifier.

  CreateRequest("h1", 80, MEDIUM, ADDRESS_FAMILY_UNSPECIFIED);
  CreateRequest("h1", 80, MEDIUM, ADDRESS_FAMILY_IPV4);
  CreateRequest("h1", 80, MEDIUM, ADDRESS_FAMILY_IPV6);

  // The IPv4 and IPv6 requests are in cache, but the UNSPECIFIED one isn't.
  EXPECT_THAT(requests_[3]->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(requests_[4]->Resolve(), IsOk());
  EXPECT_THAT(requests_[5]->Resolve(), IsOk());

  proc_->SignalMultiple(1);

  EXPECT_THAT(requests_[3]->WaitForResult(), IsOk());

  // The MockHostResolverProc has only seen one new request.
  capture_list = proc_->GetCaptureList();
  ASSERT_EQ(4u, capture_list.size());

  EXPECT_EQ("h1", capture_list[3].hostname);
  EXPECT_EQ(ADDRESS_FAMILY_UNSPECIFIED, capture_list[3].address_family);

  // Now check that the correct resolved IP addresses were returned.
  EXPECT_TRUE(requests_[3]->HasOneAddress("::3", 80));
  EXPECT_TRUE(requests_[4]->HasOneAddress("1.0.0.1", 80));
  EXPECT_TRUE(requests_[5]->HasOneAddress("::2", 80));
}

TEST_F(HostResolverImplDnsTest, NotFoundTTL) {
  CreateResolver();
  set_fallback_to_proctask(false);
  ChangeDnsConfig(CreateValidDnsConfig());
  // NODATA
  Request* request = CreateRequest("empty");
  EXPECT_THAT(request->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(request->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));
  EXPECT_THAT(request->NumberOfAddresses(), 0);
  HostCache::Key key(request->info().hostname(), ADDRESS_FAMILY_UNSPECIFIED, 0);
  HostCache::EntryStaleness staleness;
  const HostCache::Entry* cache_entry =
      resolver_->GetHostCache()->Lookup(key, base::TimeTicks::Now());
  EXPECT_TRUE(!!cache_entry);
  EXPECT_TRUE(cache_entry->has_ttl());
  EXPECT_THAT(cache_entry->ttl(), base::TimeDelta::FromSeconds(86400));

  // NXDOMAIN
  request = CreateRequest("nodomain");
  EXPECT_THAT(request->Resolve(), IsError(ERR_IO_PENDING));
  EXPECT_THAT(request->WaitForResult(), IsError(ERR_NAME_NOT_RESOLVED));
  EXPECT_THAT(request->NumberOfAddresses(), 0);
  HostCache::Key nxkey(request->info().hostname(), ADDRESS_FAMILY_UNSPECIFIED,
                       0);
  cache_entry =
      resolver_->GetHostCache()->Lookup(nxkey, base::TimeTicks::Now());
  EXPECT_TRUE(!!cache_entry);
  EXPECT_TRUE(cache_entry->has_ttl());
  EXPECT_THAT(cache_entry->ttl(), base::TimeDelta::FromSeconds(86400));
}

TEST_F(HostResolverImplTest, ResolveLocalHostname) {
  AddressList addresses;

  TestBothLoopbackIPs("localhost");
  TestBothLoopbackIPs("localhoST");
  TestBothLoopbackIPs("localhost.");
  TestBothLoopbackIPs("localhoST.");
  TestBothLoopbackIPs("localhost.localdomain");
  TestBothLoopbackIPs("localhost.localdomAIn");
  TestBothLoopbackIPs("localhost.localdomain.");
  TestBothLoopbackIPs("localhost.localdomAIn.");
  TestBothLoopbackIPs("foo.localhost");
  TestBothLoopbackIPs("foo.localhOSt");
  TestBothLoopbackIPs("foo.localhost.");
  TestBothLoopbackIPs("foo.localhOSt.");

  TestIPv6LoopbackOnly("localhost6");
  TestIPv6LoopbackOnly("localhoST6");
  TestIPv6LoopbackOnly("localhost6.");
  TestIPv6LoopbackOnly("localhost6.localdomain6");
  TestIPv6LoopbackOnly("localhost6.localdomain6.");

  EXPECT_FALSE(
      ResolveLocalHostname("127.0.0.1", kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(ResolveLocalHostname("::1", kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(ResolveLocalHostname("0:0:0:0:0:0:0:1", kLocalhostLookupPort,
                                    &addresses));
  EXPECT_FALSE(
      ResolveLocalHostname("localhostx", kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(
      ResolveLocalHostname("localhost.x", kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(ResolveLocalHostname("foo.localdomain", kLocalhostLookupPort,
                                    &addresses));
  EXPECT_FALSE(ResolveLocalHostname("foo.localdomain.x", kLocalhostLookupPort,
                                    &addresses));
  EXPECT_FALSE(
      ResolveLocalHostname("localhost6x", kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(ResolveLocalHostname("localhost.localdomain6",
                                    kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(ResolveLocalHostname("localhost6.localdomain",
                                    kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(
      ResolveLocalHostname("127.0.0.1.1", kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(
      ResolveLocalHostname(".127.0.0.255", kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(ResolveLocalHostname("::2", kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(ResolveLocalHostname("::1:1", kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(ResolveLocalHostname("0:0:0:0:1:0:0:1", kLocalhostLookupPort,
                                    &addresses));
  EXPECT_FALSE(ResolveLocalHostname("::1:1", kLocalhostLookupPort, &addresses));
  EXPECT_FALSE(ResolveLocalHostname("0:0:0:0:0:0:0:0:1", kLocalhostLookupPort,
                                    &addresses));
  EXPECT_FALSE(ResolveLocalHostname("foo.localhost.com", kLocalhostLookupPort,
                                    &addresses));
  EXPECT_FALSE(
      ResolveLocalHostname("foo.localhoste", kLocalhostLookupPort, &addresses));
}

TEST_F(HostResolverImplDnsTest, AddDnsOverHttpsServerAfterConfig) {
  resolver_ = nullptr;
  test::ScopedMockNetworkChangeNotifier notifier;
  CreateSerialResolver();  // To guarantee order of resolutions.
  notifier.mock_network_change_notifier()->SetConnectionType(
      NetworkChangeNotifier::CONNECTION_WIFI);
  ChangeDnsConfig(CreateValidDnsConfig());

  resolver_->SetDnsClientEnabled(true);
  std::string spec("https://dns.example.com/");
  resolver_->AddDnsOverHttpsServer(spec, true);
  base::DictionaryValue* config;

  auto value = resolver_->GetDnsConfigAsValue();
  EXPECT_TRUE(value);
  if (!value)
    return;
  value->GetAsDictionary(&config);
  base::ListValue* doh_servers;
  config->GetListWithoutPathExpansion("doh_servers", &doh_servers);
  EXPECT_TRUE(doh_servers);
  if (!doh_servers)
    return;
  EXPECT_EQ(doh_servers->GetSize(), 1u);
  base::DictionaryValue* server_method;
  EXPECT_TRUE(doh_servers->GetDictionary(0, &server_method));
  bool use_post;
  EXPECT_TRUE(server_method->GetBoolean("use_post", &use_post));
  EXPECT_TRUE(use_post);
  std::string server_spec;
  EXPECT_TRUE(server_method->GetString("server", &server_spec));
  EXPECT_EQ(server_spec, spec);
}

TEST_F(HostResolverImplDnsTest, AddDnsOverHttpsServerBeforeConfig) {
  resolver_ = nullptr;
  test::ScopedMockNetworkChangeNotifier notifier;
  CreateSerialResolver();  // To guarantee order of resolutions.
  resolver_->SetDnsClientEnabled(true);
  std::string spec("https://dns.example.com/");
  resolver_->AddDnsOverHttpsServer(spec, true);

  notifier.mock_network_change_notifier()->SetConnectionType(
      NetworkChangeNotifier::CONNECTION_WIFI);
  ChangeDnsConfig(CreateValidDnsConfig());

  base::DictionaryValue* config;
  auto value = resolver_->GetDnsConfigAsValue();
  EXPECT_TRUE(value);
  if (!value)
    return;
  value->GetAsDictionary(&config);
  base::ListValue* doh_servers;
  config->GetListWithoutPathExpansion("doh_servers", &doh_servers);
  EXPECT_TRUE(doh_servers);
  if (!doh_servers)
    return;
  EXPECT_EQ(doh_servers->GetSize(), 1u);
  base::DictionaryValue* server_method;
  EXPECT_TRUE(doh_servers->GetDictionary(0, &server_method));
  bool use_post;
  EXPECT_TRUE(server_method->GetBoolean("use_post", &use_post));
  EXPECT_TRUE(use_post);
  std::string server_spec;
  EXPECT_TRUE(server_method->GetString("server", &server_spec));
  EXPECT_EQ(server_spec, spec);
}

TEST_F(HostResolverImplDnsTest, AddDnsOverHttpsServerBeforeClient) {
  resolver_ = nullptr;
  test::ScopedMockNetworkChangeNotifier notifier;
  CreateSerialResolver();  // To guarantee order of resolutions.
  std::string spec("https://dns.example.com/");
  resolver_->AddDnsOverHttpsServer(spec, true);

  notifier.mock_network_change_notifier()->SetConnectionType(
      NetworkChangeNotifier::CONNECTION_WIFI);
  ChangeDnsConfig(CreateValidDnsConfig());

  resolver_->SetDnsClientEnabled(true);

  base::DictionaryValue* config;
  auto value = resolver_->GetDnsConfigAsValue();
  EXPECT_TRUE(value);
  if (!value)
    return;
  value->GetAsDictionary(&config);
  base::ListValue* doh_servers;
  config->GetListWithoutPathExpansion("doh_servers", &doh_servers);
  EXPECT_TRUE(doh_servers);
  if (!doh_servers)
    return;
  EXPECT_EQ(doh_servers->GetSize(), 1u);
  base::DictionaryValue* server_method;
  EXPECT_TRUE(doh_servers->GetDictionary(0, &server_method));
  bool use_post;
  EXPECT_TRUE(server_method->GetBoolean("use_post", &use_post));
  EXPECT_TRUE(use_post);
  std::string server_spec;
  EXPECT_TRUE(server_method->GetString("server", &server_spec));
  EXPECT_EQ(server_spec, spec);
}

TEST_F(HostResolverImplDnsTest, AddDnsOverHttpsServerAndThenRemove) {
  resolver_ = nullptr;
  test::ScopedMockNetworkChangeNotifier notifier;
  CreateSerialResolver();  // To guarantee order of resolutions.
  std::string spec("https://dns.example.com/");
  resolver_->AddDnsOverHttpsServer(spec, true);

  notifier.mock_network_change_notifier()->SetConnectionType(
      NetworkChangeNotifier::CONNECTION_WIFI);
  ChangeDnsConfig(CreateValidDnsConfig());

  resolver_->SetDnsClientEnabled(true);

  base::DictionaryValue* config;
  auto value = resolver_->GetDnsConfigAsValue();
  EXPECT_TRUE(value);
  if (!value)
    return;
  value->GetAsDictionary(&config);
  base::ListValue* doh_servers;
  config->GetListWithoutPathExpansion("doh_servers", &doh_servers);
  EXPECT_TRUE(doh_servers);
  if (!doh_servers)
    return;
  EXPECT_EQ(doh_servers->GetSize(), 1u);
  base::DictionaryValue* server_method;
  EXPECT_TRUE(doh_servers->GetDictionary(0, &server_method));
  bool use_post;
  EXPECT_TRUE(server_method->GetBoolean("use_post", &use_post));
  EXPECT_TRUE(use_post);
  std::string server_spec;
  EXPECT_TRUE(server_method->GetString("server", &server_spec));
  EXPECT_EQ(server_spec, spec);

  resolver_->ClearDnsOverHttpsServers();
  value = resolver_->GetDnsConfigAsValue();
  EXPECT_TRUE(value);
  if (!value)
    return;
  value->GetAsDictionary(&config);
  config->GetListWithoutPathExpansion("doh_servers", &doh_servers);
  EXPECT_TRUE(doh_servers);
  if (!doh_servers)
    return;
  EXPECT_EQ(doh_servers->GetSize(), 0u);
}

}  // namespace net
