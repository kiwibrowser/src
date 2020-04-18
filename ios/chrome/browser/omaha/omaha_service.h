// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_OMAHA_OMAHA_SERVICE_H_
#define IOS_CHROME_BROWSER_OMAHA_OMAHA_SERVICE_H_

#include <memory>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/scoped_observer.h"
#include "base/timer/timer.h"
#include "base/version.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace base {
class DictionaryValue;
}

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

struct UpgradeRecommendedDetails;

// This service handles the communication with the Omaha server. It also
// handles all the scheduling necessary to contact the server regularly.
// All methods, but the constructor, |GetInstance| and |Start| methods, must be
// called from the IO thread.
class OmahaService : public net::URLFetcherDelegate {
 public:
  // Called when an upgrade is recommended.
  using UpgradeRecommendedCallback =
      base::Callback<void(const UpgradeRecommendedDetails&)>;

  // Starts the service. Also set the |URLRequestContextGetter| necessary to
  // access the Omaha server. This method should only be called once.
  static void Start(net::URLRequestContextGetter* request_context_getter,
                    const UpgradeRecommendedCallback& callback);

  // Returns debug information about the omaha service.
  static void GetDebugInformation(
      const base::Callback<void(base::DictionaryValue*)> callback);

 private:
  // For tests:
  friend class OmahaServiceTest;
  friend class OmahaServiceInternalTest;
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, PingMessageTest);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest,
                           PingMessageTestWithUnknownInstallDate);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, InstallEventMessageTest);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, SendPingFailure);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, SendPingSuccess);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, SendInstallEventSuccess);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, SendPingReceiveUpdate);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, PersistStatesTest);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, BackoffTest);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, NonSpammingTest);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, ActivePingAfterInstallEventTest);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceTest, InstallRetryTest);
  FRIEND_TEST_ALL_PREFIXES(OmahaServiceInternalTest,
                           PingMessageTestWithProfileData);
  // For the singleton:
  friend struct base::DefaultSingletonTraits<OmahaService>;
  friend class base::Singleton<OmahaService>;

  // Enum for the |GetPingContent| and |GetNextPingRequestId| method.
  enum PingContent {
    INSTALL_EVENT,
    USAGE_PING,
  };

  // Initialize the timer. Used on startup.
  void Initialize();

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* fetcher) override;

  // Raw GetInstance method. Necessary for using singletons.
  static OmahaService* GetInstance();

  // Private constructor, only used by the singleton.
  OmahaService();
  // Private constructor, only used for tests.
  explicit OmahaService(bool schedule);
  ~OmahaService() override;

  // Returns the time to wait before next attempt.
  static base::TimeDelta GetBackOff(uint8_t number_of_tries);

  void set_upgrade_recommended_callback(
      const UpgradeRecommendedCallback& callback) {
    upgrade_recommended_callback_ = callback;
  }

  // Sends a ping to the Omaha server.
  void SendPing();

  // Method that will either start sending a ping to the server, or schedule
  // itself to be called again when the next ping must be send.
  void SendOrScheduleNextPing();

  // Persists the state of the service.
  void PersistStates();

  // Returns the XML representation of the ping message to send to the Omaha
  // server. If |sendInstallEvent| is true, the message will contain an
  // installation complete event.
  std::string GetPingContent(const std::string& requestId,
                             const std::string& sessionId,
                             const std::string& versionName,
                             const std::string& channelName,
                             const base::Time& installationTime,
                             PingContent pingContent);

  // Returns the xml representation of the ping message to send to the Omaha
  // server. Use the current state of the service to compute the right message.
  std::string GetCurrentPingContent();

  // Computes debugging information and fill |result|.
  void GetDebugInformationOnIOThread(
      const base::Callback<void(base::DictionaryValue*)> callback);

  // Returns whether the next ping to send must a an install/update ping. If
  // |true|, the next ping must use |GetInstallRetryRequestId| as identifier
  // for the request and must include a X-RequestAge header.
  bool IsNextPingInstallRetry();

  // Returns the request identifier to use for the next ping. If it is an
  // install/update retry, it will return the identifier used on the initial
  // request. If this is not the case, returns a random id.
  // |send_install_event| must be true if the next ping is a install/update
  // event, in that case, the identifier will be stored so that it can be
  // reused until the ping is successful.
  std::string GetNextPingRequestId(PingContent ping_content);

  // Stores the given request id to be reused on install/update retry.
  void SetInstallRetryRequestId(const std::string& request_id);

  // Clears the stored request id for a installation/update ping retry. Must be
  // called after a successful installation/update ping.
  void ClearInstallRetryRequestId();

  // Clears the all persistent state. Should only be used for testing.
  static void ClearPersistentStateForTests();

  // To communicate with the Omaha server.
  std::unique_ptr<net::URLFetcher> fetcher_;
  net::URLRequestContextGetter* request_context_getter_;

  // The timer that call this object back when needed.
  base::OneShotTimer timer_;

  // Whether to schedule pings. This is only false for tests.
  const bool schedule_;

  // The install date of the application.  This is fetched in |Initialize| on
  // the main thread and cached for use on the IO thread.
  int64_t application_install_date_;

  // The time at which the last ping was sent.
  base::Time last_sent_time_;

  // The time at which to send the next ping.
  base::Time next_tries_time_;

  // The timestamp of the ping to send.
  base::Time current_ping_time_;

  // Last version for which an installation ping has been sent.
  base::Version last_sent_version_;

  // The language in use at start up.
  std::string locale_lang_;

  // Number of tries of the last ping.
  uint8_t number_of_tries_;

  // Whether the ping currently being sent is an install (new or update) ping.
  bool sending_install_event_;

  // Called to notify that upgrade is recommended.
  UpgradeRecommendedCallback upgrade_recommended_callback_;

  DISALLOW_COPY_AND_ASSIGN(OmahaService);
};

#endif  // IOS_CHROME_BROWSER_OMAHA_OMAHA_SERVICE_H_
