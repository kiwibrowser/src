// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_request_options.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/rand_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/safe_sprintf.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_features.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_switches.h"
#include "components/variations/variations_associated_data.h"
#include "crypto/random.h"
#include "net/base/host_port_pair.h"
#include "net/base/load_flags.h"
#include "net/base/proxy_server.h"
#include "net/url_request/url_request.h"

#if defined(USE_GOOGLE_API_KEYS_FOR_AUTH_KEY)
#include "google_apis/google_api_keys.h"
#endif

namespace data_reduction_proxy {
namespace {

std::string FormatOption(const std::string& name, const std::string& value) {
  return name + "=" + value;
}

}  // namespace

const char kSessionHeaderOption[] = "ps";
const char kCredentialsHeaderOption[] = "sid";
const char kSecureSessionHeaderOption[] = "s";
const char kBuildNumberHeaderOption[] = "b";
const char kPatchNumberHeaderOption[] = "p";
const char kClientHeaderOption[] = "c";
const char kExperimentsOption[] = "exp";
const char kPageIdOption[] = "pid";

// The empty version for the authentication protocol. Currently used by
// Android webview.
#if defined(OS_ANDROID)
const char kAndroidWebViewProtocolVersion[] = "";
#endif

// static
bool DataReductionProxyRequestOptions::IsKeySetOnCommandLine() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  return command_line.HasSwitch(
      data_reduction_proxy::switches::kDataReductionProxyKey);
}

DataReductionProxyRequestOptions::DataReductionProxyRequestOptions(
    Client client,
    DataReductionProxyConfig* config)
    : DataReductionProxyRequestOptions(client,
                                       util::ChromiumVersion(),
                                       config) {}

DataReductionProxyRequestOptions::DataReductionProxyRequestOptions(
    Client client,
    const std::string& version,
    DataReductionProxyConfig* config)
    : client_(util::GetStringForClient(client)),
      use_assigned_credentials_(false),
      data_reduction_proxy_config_(config),
      current_page_id_(base::RandUint64()) {
  DCHECK(data_reduction_proxy_config_);
  util::GetChromiumBuildAndPatch(version, &build_, &patch_);
}

DataReductionProxyRequestOptions::~DataReductionProxyRequestOptions() {
}

void DataReductionProxyRequestOptions::Init() {
  DCHECK(thread_checker_.CalledOnValidThread());
  key_ = GetDefaultKey(),
  UpdateCredentials();
  UpdateExperiments();
  // Called on the UI thread, but should be checked on the IO thread.
  thread_checker_.DetachFromThread();
}

std::string DataReductionProxyRequestOptions::GetHeaderValueForTesting() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return header_value_;
}

void DataReductionProxyRequestOptions::UpdateExperiments() {
  experiments_.clear();
  std::string experiments =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          data_reduction_proxy::switches::kDataReductionProxyExperiment);

  // The command line override takes precedence over field trial "exp"
  // directives.
  if (!experiments.empty()) {
    base::StringTokenizer experiment_tokenizer(experiments, ", ");
    experiment_tokenizer.set_quote_chars("\"");
    while (experiment_tokenizer.GetNext()) {
      if (!experiment_tokenizer.token().empty())
        experiments_.push_back(experiment_tokenizer.token());
    }
  } else {
    // If no other "exp" directive is forced by flags, add the field trial
    // value.
    AddServerExperimentFromFieldTrial();
  }

  RegenerateRequestHeaderValue();
}

void DataReductionProxyRequestOptions::AddServerExperimentFromFieldTrial() {
  if (!params::IsIncludedInServerExperimentsFieldTrial())
    return;
  const std::string server_experiment = variations::GetVariationParamValue(
      params::GetServerExperimentsFieldTrialName(), kExperimentsOption);
  if (!server_experiment.empty())
    experiments_.push_back(server_experiment);
}

// static
base::string16 DataReductionProxyRequestOptions::AuthHashForSalt(
    int64_t salt,
    const std::string& key) {
  std::string salted_key =
      base::StringPrintf("%lld%s%lld",
                         static_cast<long long>(salt),
                         key.c_str(),
                         static_cast<long long>(salt));
  return base::UTF8ToUTF16(base::MD5String(salted_key));
}

base::Time DataReductionProxyRequestOptions::Now() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return base::Time::Now();
}

void DataReductionProxyRequestOptions::RandBytes(void* output,
                                                 size_t length) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  crypto::RandBytes(output, length);
}

void DataReductionProxyRequestOptions::AddRequestHeader(
    net::HttpRequestHeaders* request_headers,
    base::Optional<uint64_t> page_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!page_id || page_id.value() > 0u);
  base::Time now = Now();
  // Authorization credentials must be regenerated if they are expired.
  if (!use_assigned_credentials_ && (now > credentials_expiration_time_))
    UpdateCredentials();
  const char kChromeProxyHeader[] = "Chrome-Proxy";
  std::string header_value;
  if (request_headers->HasHeader(kChromeProxyHeader)) {
    request_headers->GetHeader(kChromeProxyHeader, &header_value);
    request_headers->RemoveHeader(kChromeProxyHeader);
    header_value += ", ";
  }
  header_value += header_value_;

  if (page_id) {
    // 64 bit uint fits in 16 characters when represented in hexadecimal, but
    // there needs to be a trailing null termianted character in the buffer.
    char page_id_buffer[17];
    if (base::strings::SafeSPrintf(page_id_buffer, "%x", page_id.value()) > 0) {
      header_value += ", " + FormatOption(kPageIdOption, page_id_buffer);
    }
    uint64_t page_id_tested;
    DCHECK(base::HexStringToUInt64(page_id_buffer, &page_id_tested) &&
           page_id_tested == page_id.value());
    ALLOW_UNUSED_LOCAL(page_id_tested);
  }
  request_headers->SetHeader(kChromeProxyHeader, header_value);
}

void DataReductionProxyRequestOptions::ComputeCredentials(
    const base::Time& now,
    std::string* session,
    std::string* credentials) const {
  DCHECK(session);
  DCHECK(credentials);
  int64_t timestamp = (now - base::Time::UnixEpoch()).InMilliseconds() / 1000;

  int32_t rand[3];
  RandBytes(rand, 3 * sizeof(rand[0]));
  *session = base::StringPrintf("%lld-%u-%u-%u",
                                static_cast<long long>(timestamp),
                                rand[0],
                                rand[1],
                                rand[2]);
  *credentials = base::UTF16ToUTF8(AuthHashForSalt(timestamp, key_));

  DVLOG(1) << "session: [" << *session << "] "
           << "password: [" << *credentials  << "]";
}

void DataReductionProxyRequestOptions::UpdateCredentials() {
  base::Time now = Now();
  ComputeCredentials(now, &session_, &credentials_);
  credentials_expiration_time_ = now + base::TimeDelta::FromHours(24);
  RegenerateRequestHeaderValue();
}

void DataReductionProxyRequestOptions::SetKeyOnIO(const std::string& key) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if(!key.empty()) {
    key_ = key;
    UpdateCredentials();
  }
}

void DataReductionProxyRequestOptions::SetSecureSession(
    const std::string& secure_session) {
  DCHECK(thread_checker_.CalledOnValidThread());
  session_.clear();
  credentials_.clear();
  secure_session_ = secure_session;
  // Reset Page ID, so users can't be tracked across sessions.
  ResetPageId();
  // Force skipping of credential regeneration. It should be handled by the
  // caller.
  use_assigned_credentials_ = true;
  RegenerateRequestHeaderValue();
}

void DataReductionProxyRequestOptions::Invalidate() {
  DCHECK(thread_checker_.CalledOnValidThread());
  SetSecureSession(std::string());
}

std::string DataReductionProxyRequestOptions::GetDefaultKey() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  std::string key =
    command_line.GetSwitchValueASCII(switches::kDataReductionProxyKey);
// Chrome on iOS gets the default key from a preprocessor constant. Chrome on
// Android and Chrome on desktop get the key from google_apis. Cronet and
// Webview have no default key.
#if defined(OS_IOS)
#if defined(SPDY_PROXY_AUTH_VALUE)
  if (key.empty())
    key = SPDY_PROXY_AUTH_VALUE;
#endif
#elif USE_GOOGLE_API_KEYS_FOR_AUTH_KEY
  if (key.empty()) {
    key = google_apis::GetSpdyProxyAuthValue();
  }
#endif  // defined(OS_IOS)
  return key;
}

const std::string& DataReductionProxyRequestOptions::GetSecureSession() const {
  return secure_session_;
}

void DataReductionProxyRequestOptions::RegenerateRequestHeaderValue() {
  std::vector<std::string> headers;
  if (!session_.empty())
    headers.push_back(FormatOption(kSessionHeaderOption, session_));
  if (!credentials_.empty())
    headers.push_back(FormatOption(kCredentialsHeaderOption, credentials_));
  if (!secure_session_.empty()) {
    headers.push_back(
        FormatOption(kSecureSessionHeaderOption, secure_session_));
  }
  if (!client_.empty())
    headers.push_back(FormatOption(kClientHeaderOption, client_));

  DCHECK(!build_.empty());
  headers.push_back(FormatOption(kBuildNumberHeaderOption, build_));

  DCHECK(!patch_.empty());
  headers.push_back(FormatOption(kPatchNumberHeaderOption, patch_));

  for (const auto& experiment : experiments_)
    headers.push_back(FormatOption(kExperimentsOption, experiment));

  header_value_ = base::JoinString(headers, ", ");
}

std::string DataReductionProxyRequestOptions::GetSessionKeyFromRequestHeaders(
    const net::HttpRequestHeaders& request_headers) const {
  std::string chrome_proxy_header_value;
  base::StringPairs kv_pairs;
  // Return if the request does not have request headers or if they can't be
  // parsed into key-value pairs.
  if (!request_headers.GetHeader(chrome_proxy_header(),
                                 &chrome_proxy_header_value) ||
      !base::SplitStringIntoKeyValuePairs(chrome_proxy_header_value,
                                          '=',  // Key-value delimiter
                                          ',',  // Key-value pair delimiter
                                          &kv_pairs)) {
    return "";
  }

  for (const auto& kv_pair : kv_pairs) {
    // Delete leading and trailing white space characters from the key before
    // comparing.
    if (base::TrimWhitespaceASCII(kv_pair.first, base::TRIM_ALL) ==
        kSecureSessionHeaderOption) {
      return base::TrimWhitespaceASCII(kv_pair.second, base::TRIM_ALL)
          .as_string();
    }
  }
  return "";
}

uint64_t DataReductionProxyRequestOptions::GeneratePageId() {
  // Caller should not depend on order.
  return ++current_page_id_;
}

void DataReductionProxyRequestOptions::ResetPageId() {
  current_page_id_ = base::RandUint64();
}

}  // namespace data_reduction_proxy
