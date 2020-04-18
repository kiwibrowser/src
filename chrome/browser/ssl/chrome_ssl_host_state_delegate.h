// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_CHROME_SSL_HOST_STATE_DELEGATE_H_
#define CHROME_BROWSER_SSL_CHROME_SSL_HOST_STATE_DELEGATE_H_

#include <memory>
#include <set>

#include "base/feature_list.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "content/public/browser/ssl_host_state_delegate.h"

class Profile;

namespace base {
class Clock;
class DictionaryValue;
}  //  namespace base

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

// The Finch feature that controls whether a message is shown when users
// encounter the same error multiiple times.
extern const base::Feature kRecurrentInterstitialFeature;

// Tracks state related to certificate and SSL errors. This state includes:
// - certificate error exceptions (which are remembered for a particular length
//   of time depending on experimental groups)
// - mixed content exceptions
// - when errors have recurred multiple times
class ChromeSSLHostStateDelegate : public content::SSLHostStateDelegate {
 public:
  explicit ChromeSSLHostStateDelegate(Profile* profile);
  ~ChromeSSLHostStateDelegate() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // SSLHostStateDelegate:
  void AllowCert(const std::string& host,
                 const net::X509Certificate& cert,
                 net::CertStatus error) override;
  void Clear(
      const base::Callback<bool(const std::string&)>& host_filter) override;
  CertJudgment QueryPolicy(const std::string& host,
                           const net::X509Certificate& cert,
                           net::CertStatus error,
                           bool* expired_previous_decision) override;
  void HostRanInsecureContent(const std::string& host,
                              int child_id,
                              InsecureContentType content_type) override;
  bool DidHostRunInsecureContent(
      const std::string& host,
      int child_id,
      InsecureContentType content_type) const override;

  // Revokes all SSL certificate error allow exceptions made by the user for
  // |host| in the given Profile.
  void RevokeUserAllowExceptions(const std::string& host) override;

  // RevokeUserAllowExceptionsHard is the same as RevokeUserAllowExceptions but
  // additionally may close idle connections in the process. This should be used
  // *only* for rare events, such as a user controlled button, as it may be very
  // disruptive to the networking stack.
  virtual void RevokeUserAllowExceptionsHard(const std::string& host);

  // Returns whether the user has allowed a certificate error exception for
  // |host|. This does not mean that *all* certificate errors are allowed, just
  // that there exists an exception. To see if a particular certificate and
  // error combination exception is allowed, use QueryPolicy().
  bool HasAllowException(const std::string& host) const override;

  // Called when an error page is displayed for a given error code |error|.
  // Tracks whether an error of interest has recurred over a threshold number of
  // times.
  void DidDisplayErrorPage(int error);

  // Returns true if DidDisplayErrorPage() has been called over a threshold
  // number of times for a particular error in a particular time period. Always
  // returns false if |kRecurrentInterstitialFeature| is not enabled. The number
  // of times and time period are controlled by the feature parameters. Only
  // certain error codes of interest are tracked, so this may return false for
  // an error code that has recurred.
  bool HasSeenRecurrentErrors(int error) const;

  void ResetRecurrentErrorCountForTesting();

  // SetClockForTesting takes ownership of the passed in clock.
  void SetClockForTesting(std::unique_ptr<base::Clock> clock);

 private:
  // Used to specify whether new content setting entries should be created if
  // they don't already exist when querying the user's settings.
  enum CreateDictionaryEntriesDisposition {
    CREATE_DICTIONARY_ENTRIES,
    DO_NOT_CREATE_DICTIONARY_ENTRIES
  };

  // Specifies whether user SSL error decisions should be forgetten at the end
  // of this current session (the old style of remembering decisions), or
  // whether they should be remembered across session restarts for a specified
  // length of time, deteremined by
  // |default_ssl_cert_decision_expiration_delta_|.
  enum RememberSSLExceptionDecisionsDisposition {
    FORGET_SSL_EXCEPTION_DECISIONS_AT_SESSION_END,
    REMEMBER_SSL_EXCEPTION_DECISIONS_FOR_DELTA
  };

  // Returns a dictionary of certificate fingerprints and errors that have been
  // allowed as exceptions by the user.
  //
  // |dict| specifies the user's full exceptions dictionary for a specific site
  // in their content settings. Must be retrieved directly from a website
  // setting in the the profile's HostContentSettingsMap.
  //
  // If |create_entries| specifies CreateDictionaryEntries, then
  // GetValidCertDecisionsDict will create a new set of entries within the
  // dictionary if they do not already exist. Otherwise will fail and return if
  // NULL if they do not exist.
  //
  // |expired_previous_decision| is set to true if there had been a previous
  // decision made by the user but it has expired. Otherwise it is set to false.
  base::DictionaryValue* GetValidCertDecisionsDict(
      base::DictionaryValue* dict,
      CreateDictionaryEntriesDisposition create_entries,
      bool* expired_previous_decision);

  std::unique_ptr<base::Clock> clock_;
  RememberSSLExceptionDecisionsDisposition should_remember_ssl_decisions_;
  Profile* profile_;

  // A BrokenHostEntry is a pair of (host, child_id) that indicates the host
  // contains insecure content in that renderer process.
  typedef std::pair<std::string, int> BrokenHostEntry;

  // Hosts which have been contaminated with insecure mixed content in the
  // specified process.  Note that insecure content can travel between
  // same-origin frames in one processs but cannot jump between processes.
  std::set<BrokenHostEntry> ran_mixed_content_hosts_;

  // Hosts which have been contaminated with content with certificate errors in
  // the specific process.
  std::set<BrokenHostEntry> ran_content_with_cert_errors_hosts_;

  // This is a GUID to mark this unique session. Whenever a certificate decision
  // expiration is set, the GUID is saved as well so Chrome can tell if it was
  // last set during the current session. This is used by the
  // FORGET_SSL_EXCEPTION_DECISIONS_AT_SESSION_END experimental group to
  // determine if the expired_previous_decision bit should be set on queries.
  //
  // Why not just iterate over the set of current extensions and mark them all
  // as expired when the session starts, rather than storing a GUID for the
  // current session? Glad you asked! Unfortunately, content settings does not
  // currently support iterating over all current *compound* content setting
  // values (iteration only works for simple content settings). While this could
  // be added, it would be a fair amount of work for what amounts to a temporary
  // measurement problem, so it's not worth the complexity.
  //
  // TODO(jww): This is only used by the default and disable groups of the
  // certificate memory decisions experiment to tell if a decision has expired
  // since the last session. Since this is only used for UMA purposes, this
  // should be removed after the experiment has finished, and a call to Clear()
  // should be added to the constructor and destructor for members of the
  // FORGET_SSL_EXCEPTION_DECISIONS_AT_SESSION_END groups. See
  // https://crbug.com/418631 for more details.
  const std::string current_expiration_guid_;

  // Tracks how many times an error page has been shown for a given error, up
  // to a certain threshold value.
  std::map<int /* error code */, int /* count */> recurrent_errors_;

  DISALLOW_COPY_AND_ASSIGN(ChromeSSLHostStateDelegate);
};

#endif  // CHROME_BROWSER_SSL_CHROME_SSL_HOST_STATE_DELEGATE_H_
