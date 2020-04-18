// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Library functions related to the Financial Server ping.

#include "rlz/lib/financial_ping.h"

#include <stdint.h>

#include <memory>

#include "base/atomicops.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "rlz/lib/assert.h"
#include "rlz/lib/lib_values.h"
#include "rlz/lib/machine_id.h"
#include "rlz/lib/rlz_lib.h"
#include "rlz/lib/rlz_value_store.h"
#include "rlz/lib/string_utils.h"

#if !defined(OS_WIN)
#include "base/time/time.h"
#endif

#if defined(RLZ_NETWORK_IMPLEMENTATION_WIN_INET)

#include <windows.h>
#include <wininet.h>

namespace {

class InternetHandle {
 public:
  InternetHandle(HINTERNET handle) { handle_ = handle; }
  ~InternetHandle() { if (handle_) InternetCloseHandle(handle_); }
  operator HINTERNET() const { return handle_; }
  bool operator!() const { return (handle_ == NULL); }

 private:
  HINTERNET handle_;
};

}  // namespace

#else

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

#endif

namespace rlz_lib {

using base::subtle::AtomicWord;

bool FinancialPing::FormRequest(Product product,
    const AccessPoint* access_points, const char* product_signature,
    const char* product_brand, const char* product_id,
    const char* product_lang, bool exclude_machine_id,
    std::string* request) {
  if (!request) {
    ASSERT_STRING("FinancialPing::FormRequest: request is NULL");
    return false;
  }

  request->clear();

  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kReadAccess))
    return false;

  if (!access_points) {
    ASSERT_STRING("FinancialPing::FormRequest: access_points is NULL");
    return false;
  }

  if (!product_signature) {
    ASSERT_STRING("FinancialPing::FormRequest: product_signature is NULL");
    return false;
  }

  if (!SupplementaryBranding::GetBrand().empty()) {
    if (SupplementaryBranding::GetBrand() != product_brand) {
      ASSERT_STRING("FinancialPing::FormRequest: supplementary branding bad");
      return false;
    }
  }

  base::StringAppendF(request, "%s?", kFinancialPingPath);

  // Add the signature, brand, product id and language.
  base::StringAppendF(request, "%s=%s", kProductSignatureCgiVariable,
                      product_signature);
  if (product_brand)
    base::StringAppendF(request, "&%s=%s", kProductBrandCgiVariable,
                        product_brand);

  if (product_id)
    base::StringAppendF(request, "&%s=%s", kProductIdCgiVariable, product_id);

  if (product_lang)
    base::StringAppendF(request, "&%s=%s", kProductLanguageCgiVariable,
                        product_lang);

  // Add the product events.
  char cgi[kMaxCgiLength + 1];
  cgi[0] = 0;
  bool has_events = GetProductEventsAsCgi(product, cgi, arraysize(cgi));
  if (has_events)
    base::StringAppendF(request, "&%s", cgi);

  // If we don't have any events, we should ping all the AP's on the system
  // that we know about and have a current RLZ value, even if they are not
  // used by this product.
  AccessPoint all_points[LAST_ACCESS_POINT];
  if (!has_events) {
    char rlz[kMaxRlzLength + 1];
    int idx = 0;
    for (int ap = NO_ACCESS_POINT + 1; ap < LAST_ACCESS_POINT; ap++) {
      rlz[0] = 0;
      AccessPoint point = static_cast<AccessPoint>(ap);
      if (GetAccessPointRlz(point, rlz, arraysize(rlz)) &&
          rlz[0] != '\0')
        all_points[idx++] = point;
    }
    all_points[idx] = NO_ACCESS_POINT;
  }

  // Add the RLZ's and the DCC if needed. This is the same as get PingParams.
  // This will also include the RLZ Exchange Protocol CGI Argument.
  cgi[0] = 0;
  if (GetPingParams(product, has_events ? access_points : all_points,
                    cgi, arraysize(cgi)))
    base::StringAppendF(request, "&%s", cgi);

  if (has_events && !exclude_machine_id) {
    std::string machine_id;
    if (GetMachineId(&machine_id)) {
      base::StringAppendF(request, "&%s=%s", kMachineIdCgiVariable,
                          machine_id.c_str());
    }
  }

  return true;
}

#if defined(RLZ_NETWORK_IMPLEMENTATION_CHROME_NET)
// The pointer to URLRequestContextGetter used by FinancialPing::PingServer().
// It is atomic pointer because it can be accessed and modified by multiple
// threads.
AtomicWord g_context;

bool FinancialPing::SetURLRequestContext(
    net::URLRequestContextGetter* context) {
  base::subtle::Release_Store(
      &g_context, reinterpret_cast<AtomicWord>(context));
  return true;
}

// Signal to stop the ShutdownCheck() task.
AtomicWord g_cancelShutdownCheck;

namespace {

// A waitable event used to detect when either:
//
//   1/ the RLZ ping request completes
//   2/ the RLZ ping request times out
//   3/ browser shutdown begins
class RefCountedWaitableEvent
    : public base::RefCountedThreadSafe<RefCountedWaitableEvent> {
 public:
  RefCountedWaitableEvent()
      : event_(base::WaitableEvent::ResetPolicy::MANUAL,
               base::WaitableEvent::InitialState::NOT_SIGNALED) {}

  void SignalShutdown() { event_.Signal(); }

  void SignalFetchComplete(int response_code, std::string response) {
    base::AutoLock autolock(lock_);
    response_code_ = response_code;
    response_ = std::move(response);
    event_.Signal();
  }

  bool TimedWait(base::TimeDelta timeout) { return event_.TimedWait(timeout); }

  int GetResponseCode() {
    base::AutoLock autolock(lock_);
    return response_code_;
  }

  std::string TakeResponse() {
    base::AutoLock autolock(lock_);
    std::string temp = std::move(response_);
    response_.clear();
    return temp;
  }

 private:
  ~RefCountedWaitableEvent() = default;
  friend class base::RefCountedThreadSafe<RefCountedWaitableEvent>;

  base::WaitableEvent event_;
  base::Lock lock_;
  std::string response_;
  int response_code_ = net::URLFetcher::RESPONSE_CODE_INVALID;
};

// A fetcher delegate that signals an instance of RefCountedWaitableEvent when
// the fetch completes.
class FinancialPingUrlFetcherDelegate : public net::URLFetcherDelegate {
 public:
  FinancialPingUrlFetcherDelegate(scoped_refptr<RefCountedWaitableEvent> event)
      : event_(std::move(event)) {}

  void SetFetcher(std::unique_ptr<net::URLFetcher> fetcher) {
    fetcher_ = std::move(fetcher);
  }

 private:
  void OnURLFetchComplete(const net::URLFetcher* source) override {
    std::string response;
    source->GetResponseAsString(&response);
    event_->SignalFetchComplete(source->GetResponseCode(), std::move(response));
    base::SequencedTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
  }

  scoped_refptr<RefCountedWaitableEvent> event_;
  std::unique_ptr<net::URLFetcher> fetcher_;
};

bool send_financial_ping_interrupted_for_test = false;

}  // namespace

#if defined(RLZ_NETWORK_IMPLEMENTATION_CHROME_NET)
void ShutdownCheck(scoped_refptr<RefCountedWaitableEvent> event) {
  if (base::subtle::Acquire_Load(&g_cancelShutdownCheck))
    return;

  if (!base::subtle::Acquire_Load(&g_context)) {
    send_financial_ping_interrupted_for_test = true;
    event->SignalShutdown();
    return;
  }
  // How frequently the financial ping thread should check
  // the shutdown condition?
  const base::TimeDelta kInterval = base::TimeDelta::FromMilliseconds(500);
  base::PostDelayedTaskWithTraits(FROM_HERE, {base::TaskPriority::BACKGROUND},
                                  base::BindOnce(&ShutdownCheck, event),
                                  kInterval);
}
#endif

void PingRlzServer(std::string url,
                   scoped_refptr<RefCountedWaitableEvent> event) {
  // Copy the pointer to stack because g_context may be set to NULL
  // in different thread. The instance is guaranteed to exist while
  // the method is running.
  net::URLRequestContextGetter* context =
      reinterpret_cast<net::URLRequestContextGetter*>(
          base::subtle::Acquire_Load(&g_context));

  // Browser shutdown will cause the context to be reset to NULL.
  // ShutdownCheck will catch this.
  if (!context)
    return;

  // Delegate will delete itself when the fetch completes.
  FinancialPingUrlFetcherDelegate* delegate =
      new FinancialPingUrlFetcherDelegate(event);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("rlz_ping", R"(
        semantics {
          sender: "RLZ Ping"
          description:
            "Used for measuring the effectiveness of a promotion. See the "
            "Chrome Privacy Whitepaper for complete details."
          trigger:
            "1- At Chromium first run.\n"
            "2- When Chromium is re-activated by a new promotion.\n"
            "3- Once a week thereafter as long as Chromium is used.\n"
          data:
            "1- Non-unique cohort tag of when Chromium was installed.\n"
            "2- Unique machine id on desktop platforms.\n"
            "3- Whether Google is the default omnibox search.\n"
            "4- Whether google.com is the default home page."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled in settings."
          policy_exception_justification: "Not implemented."
        })");
  std::unique_ptr<net::URLFetcher> fetcher = net::URLFetcher::Create(
      GURL(url), net::URLFetcher::GET, delegate, traffic_annotation);

  fetcher->SetLoadFlags(
      net::LOAD_DISABLE_CACHE | net::LOAD_DO_NOT_SEND_AUTH_DATA |
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES);

  // Ensure rlz_lib::SetURLRequestContext() has been called before sending
  // pings.
  fetcher->SetRequestContext(context);
  fetcher->Start();

  // Pass ownership of the fetcher to the delegate.  Otherwise the fetch will
  // be canceled when the URLFetcher object is destroyed.
  delegate->SetFetcher(std::move(fetcher));
}
#endif

FinancialPing::PingResponse FinancialPing::PingServer(const char* request,
                                                      std::string* response) {
  if (!response)
    return PING_FAILURE;

  response->clear();

#if defined(RLZ_NETWORK_IMPLEMENTATION_WIN_INET)
  // Initialize WinInet.
  InternetHandle inet_handle = InternetOpenA(kFinancialPingUserAgent,
                                             INTERNET_OPEN_TYPE_PRECONFIG,
                                             NULL, NULL, 0);
  if (!inet_handle)
    return PING_FAILURE;

  // Open network connection.
  InternetHandle connection_handle = InternetConnectA(inet_handle,
      kFinancialServer, kFinancialPort, "", "", INTERNET_SERVICE_HTTP,
      INTERNET_FLAG_NO_CACHE_WRITE, 0);
  if (!connection_handle)
    return PING_FAILURE;

  // Prepare the HTTP request.
  const DWORD kFlags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_COOKIES |
                       INTERNET_FLAG_SECURE;
  InternetHandle http_handle =
      HttpOpenRequestA(connection_handle, "GET", request, NULL, NULL,
                       kFinancialPingResponseObjects, kFlags, NULL);
  if (!http_handle)
    return PING_FAILURE;

  // Timeouts are probably:
  // INTERNET_OPTION_SEND_TIMEOUT, INTERNET_OPTION_RECEIVE_TIMEOUT

  // Send the HTTP request. Note: Fails if user is working in off-line mode.
  if (!HttpSendRequest(http_handle, NULL, 0, NULL, 0))
    return PING_FAILURE;

  // Check the response status.
  DWORD status;
  DWORD status_size = sizeof(status);
  if (!HttpQueryInfo(http_handle, HTTP_QUERY_STATUS_CODE |
                     HTTP_QUERY_FLAG_NUMBER, &status, &status_size, NULL) ||
      200 != status)
    return PING_FAILURE;

  // Get the response text.
  std::unique_ptr<char[]> buffer(new char[kMaxPingResponseLength]);
  if (buffer.get() == NULL)
    return PING_FAILURE;

  DWORD bytes_read = 0;
  while (InternetReadFile(http_handle, buffer.get(), kMaxPingResponseLength,
                          &bytes_read) && bytes_read > 0) {
    response->append(buffer.get(), bytes_read);
    bytes_read = 0;
  };

  return PING_SUCCESSFUL;
#else
  std::string url =
      base::StringPrintf("https://%s%s", kFinancialServer, request);

  // Use a waitable event to cause this function to block, to match the
  // wininet implementation.
  auto event = base::MakeRefCounted<RefCountedWaitableEvent>();

  base::subtle::Release_Store(&g_cancelShutdownCheck, 0);

  base::PostTaskWithTraits(FROM_HERE, {base::TaskPriority::BACKGROUND},
                           base::BindOnce(&ShutdownCheck, event));

  // PingRlzServer must be run in a separate sequence so that the TimedWait()
  // call below does not block the URL fetch response from being handled by
  // the URL delegate.
  scoped_refptr<base::SequencedTaskRunner> background_runner(
      base::CreateSequencedTaskRunnerWithTraits(
          {base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
           base::TaskPriority::BACKGROUND}));
  background_runner->PostTask(FROM_HERE,
                              base::BindOnce(&PingRlzServer, url, event));

  bool is_signaled;
  {
    base::ScopedAllowBaseSyncPrimitives allow_base_sync_primitives;
    is_signaled = event->TimedWait(base::TimeDelta::FromMinutes(5));
  }

  base::subtle::Release_Store(&g_cancelShutdownCheck, 1);

  if (!is_signaled)
    return PING_FAILURE;

  if (event->GetResponseCode() == net::URLFetcher::RESPONSE_CODE_INVALID) {
    return PING_SHUTDOWN;
  } else if (event->GetResponseCode() != 200) {
    return PING_FAILURE;
  }

  *response = event->TakeResponse();
  return PING_SUCCESSFUL;
#endif
}

bool FinancialPing::IsPingTime(Product product, bool no_delay) {
  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kReadAccess))
    return false;

  int64_t last_ping = 0;
  if (!store->ReadPingTime(product, &last_ping))
    return true;

  uint64_t now = GetSystemTimeAsInt64();
  int64_t interval = now - last_ping;

  // If interval is negative, clock was probably reset. So ping.
  if (interval < 0)
    return true;

  // Check if this product has any unreported events.
  char cgi[kMaxCgiLength + 1];
  cgi[0] = 0;
  bool has_events = GetProductEventsAsCgi(product, cgi, arraysize(cgi));
  if (no_delay && has_events)
    return true;

  return interval >= (has_events ? kEventsPingInterval : kNoEventsPingInterval);
}


bool FinancialPing::UpdateLastPingTime(Product product) {
  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kWriteAccess))
    return false;

  uint64_t now = GetSystemTimeAsInt64();
  return store->WritePingTime(product, now);
}


bool FinancialPing::ClearLastPingTime(Product product) {
  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kWriteAccess))
    return false;
  return store->ClearPingTime(product);
}

int64_t FinancialPing::GetSystemTimeAsInt64() {
#if defined(OS_WIN)
  FILETIME now_as_file_time;
  // Relative to Jan 1, 1601 (UTC).
  GetSystemTimeAsFileTime(&now_as_file_time);

  LARGE_INTEGER integer;
  integer.HighPart = now_as_file_time.dwHighDateTime;
  integer.LowPart = now_as_file_time.dwLowDateTime;
  return integer.QuadPart;
#else
  // Seconds since epoch (Jan 1, 1970).
  double now_seconds = base::Time::Now().ToDoubleT();
  return static_cast<int64_t>(now_seconds * 1000 * 1000 * 10);
#endif
}

#if defined(RLZ_NETWORK_IMPLEMENTATION_CHROME_NET)
namespace test {

void ResetSendFinancialPingInterrupted() {
  send_financial_ping_interrupted_for_test = false;
}

bool WasSendFinancialPingInterrupted() {
  return send_financial_ping_interrupted_for_test;
}

}  // namespace test
#endif

}  // namespace rlz_lib
