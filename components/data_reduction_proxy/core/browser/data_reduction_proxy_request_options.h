// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_AUTH_REQUEST_HANDLER_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_AUTH_REQUEST_HANDLER_H_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_util.h"

namespace net {
class HttpRequestHeaders;
}

namespace data_reduction_proxy {

extern const char kSessionHeaderOption[];
extern const char kCredentialsHeaderOption[];
extern const char kSecureSessionHeaderOption[];
extern const char kBuildNumberHeaderOption[];
extern const char kPatchNumberHeaderOption[];
extern const char kClientHeaderOption[];
extern const char kExperimentsOption[];

#if defined(OS_ANDROID)
extern const char kAndroidWebViewProtocolVersion[];
#endif

class DataReductionProxyConfig;

class DataReductionProxyRequestOptions {
 public:
  static bool IsKeySetOnCommandLine();

  // Constructs a DataReductionProxyRequestOptions object with the given
  // client type, and config.
  DataReductionProxyRequestOptions(Client client,
                                   DataReductionProxyConfig* config);

  virtual ~DataReductionProxyRequestOptions();

  // Sets |key_| to the default key and initializes the credentials, version,
  // client, and lo-fi header values. Generates the |header_value_| string,
  // which is concatenated to the Chrome-proxy header. Called on the UI thread.
  void Init();

  // Adds a 'Chrome-Proxy' header to |request_headers| with the data reduction
  // proxy authentication credentials. |page_id| should only be non-empty for
  // main frame requests.
  void AddRequestHeader(net::HttpRequestHeaders* request_headers,
                        base::Optional<uint64_t> page_id);

  // Stores the supplied key and sets up credentials suitable for authenticating
  // with the data reduction proxy.
  // This can be called more than once. For example on a platform that does not
  // have a default key defined, this function will be called some time after
  // this class has been constructed. Android WebView is a platform that does
  // this. The caller needs to make sure |this| pointer is valid when
  // SetKeyOnIO is called.
  void SetKeyOnIO(const std::string& key);

  // Sets the credentials for sending to the Data Reduction Proxy.
  void SetSecureSession(const std::string& secure_session);

  // Retrieves the credentials for sending to the Data Reduction Proxy.
  const std::string& GetSecureSession() const;

  // Invalidates the secure session credentials.
  void Invalidate();

  // Parses |request_headers| and returns the value of the session key.
  std::string GetSessionKeyFromRequestHeaders(
      const net::HttpRequestHeaders& request_headers) const;

  // Creates and returns a new unique page ID (unique per session).
  uint64_t GeneratePageId();

 protected:
  // Returns a UTF16 string that's the hash of the configured authentication
  // |key| and |salt|. Returns an empty UTF16 string if no key is configured or
  // the data reduction proxy feature isn't available.
  static base::string16 AuthHashForSalt(int64_t salt, const std::string& key);
  // Visible for testing.
  virtual base::Time Now() const;
  virtual void RandBytes(void* output, size_t length) const;

  // Visible for testing.
  virtual std::string GetDefaultKey() const;

  // Visible for testing.
  DataReductionProxyRequestOptions(Client client,
                                   const std::string& version,
                                   DataReductionProxyConfig* config);

  // Returns the chrome proxy header. Protected so that it is available for
  // testing.
  std::string GetHeaderValueForTesting() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(DataReductionProxyRequestOptionsTest,
                           AuthHashForSalt);

  // Resets the page ID for a new session.
  // TODO(ryansturm): Create a session object to store this and other data saver
  // session info. crbug.com/709624
  void ResetPageId();

  // Updates the value of the experiments to be run and regenerate the header if
  // necessary.
  void UpdateExperiments();

  // Adds the server-side experiment from the field trial.
  void AddServerExperimentFromFieldTrial();

  // Generates a session ID and credentials suitable for authenticating with
  // the data reduction proxy.
  void ComputeCredentials(const base::Time& now,
                          std::string* session,
                          std::string* credentials) const;

  // Generates and updates the session ID and credentials.
  void UpdateCredentials();

  // Regenerates the |header_value_| string which is concatenated to the
  // Chrome-proxy header.
  void RegenerateRequestHeaderValue();

  // The Chrome-Proxy header value.
  std::string header_value_;

  // Authentication state.
  std::string key_;

  // Name of the client and version of the data reduction proxy protocol to use.
  std::string client_;
  std::string session_;
  std::string credentials_;
  std::string secure_session_;
  std::string build_;
  std::string patch_;
  std::vector<std::string> experiments_;

  // The time at which the session expires. Used to ensure that a session is
  // never used for more than twenty-four hours.
  base::Time credentials_expiration_time_;

  // Whether the authentication headers are sourced by |this| or injected via
  // |SetCredentials|.
  bool use_assigned_credentials_;

  // Must outlive |this|.
  DataReductionProxyConfig* data_reduction_proxy_config_;

  // The page identifier that was last generated for data saver proxy server.
  uint64_t current_page_id_;

  // Enforce usage on the IO thread.
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyRequestOptions);
};

}  // namespace data_reduction_proxy
#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_AUTH_REQUEST_HANDLER_H_
