// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_FORM_FETCHER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_FORM_FETCHER_H_

#include <memory>
#include <vector>

#include "base/macros.h"

namespace autofill {
struct PasswordForm;
}

namespace password_manager {

struct InteractionsStats;

// This is an API for providing stored credentials to PasswordFormManager (PFM),
// so that PFM instances do not have to talk to PasswordStore directly. This
// indirection allows caching of identical requests from PFM on the same origin,
// as well as easier testing (no need to mock the whole PasswordStore when
// testing a PFM).
// TODO(crbug.com/621355): Actually modify the API to support fetching in the
// FormFetcher instance.
class FormFetcher {
 public:
  // State of waiting for a response from a PasswordStore. There might be
  // multiple transitions between these states.
  enum class State { WAITING, NOT_WAITING };

  // API to be implemented by classes which want the results from FormFetcher.
  class Consumer {
   public:
    virtual ~Consumer() = default;

    // FormFetcher calls this method every time the state changes from WAITING
    // to UP_TO_DATE. It fills |non_federated| with pointers to non-federated
    // matches (pointees stay owned by FormFetcher). To access the federated
    // matches, the consumer can simply call GetFederatedMatches().
    // |filtered_count| is the number of non-federated forms which were
    // filtered out by CredentialsFilter and not included in |non_federated|.
    virtual void ProcessMatches(
        const std::vector<const autofill::PasswordForm*>& non_federated,
        size_t filtered_count) = 0;
  };

  FormFetcher() = default;

  virtual ~FormFetcher() = default;

  // Adds |consumer|, which must not be null. If the current state is
  // UP_TO_DATE, calls ProcessMatches on the consumer immediately. Assumes that
  // |consumer| outlives |this|.
  virtual void AddConsumer(Consumer* consumer) = 0;

  // Call this to stop |consumer| from receiving updates from |this|.
  virtual void RemoveConsumer(Consumer* consumer) = 0;

  // Returns the current state of the FormFetcher
  virtual State GetState() const = 0;

  // Statistics for recent password bubble usage.
  virtual const std::vector<InteractionsStats>& GetInteractionsStats()
      const = 0;

  // Federated matches obtained from the backend. Valid only if GetState()
  // returns NOT_WAITING.
  virtual const std::vector<const autofill::PasswordForm*>&
  GetFederatedMatches() const = 0;

  // The following accessors return various kinds of `suppressed` credentials.
  // These are stored credentials that are not (auto-)filled, because they are
  // for an origin that is similar to, but not exactly matching the origin that
  // this FormFetcher was created for. They are used for recording metrics on
  // how often such -- potentially, but not necessarily related -- credentials
  // are not offered to the user, unduly increasing log-in friction.
  //
  // There are currently three kinds of suppressed credentials:
  //  1.) HTTPS credentials not filled on the HTTP version of the origin.
  //  2.) PSL-matches that are not auto-filled (but filled on account select).
  //  3.) Same-organization name credentials, not filled.
  //
  // Results below are queried on a best-effort basis, might be somewhat stale,
  // and are available shortly after the Consumer::ProcessMatches callback.

  // When this instance fetches forms for an HTTP origin: Returns saved
  // credentials, if any, found for the HTTPS version of that origin. Empty
  // otherwise.
  virtual const std::vector<const autofill::PasswordForm*>&
  GetSuppressedHTTPSForms() const = 0;

  // Returns saved credentials, if any, for PSL-matching origins. Autofilling
  // these is suppressed, however, they *can be* filled on account select.
  virtual const std::vector<const autofill::PasswordForm*>&
  GetSuppressedPSLMatchingForms() const = 0;

  // Returns saved credentials, if any, found for HTTP/HTTPS origins with the
  // same organization name as the origin this FormFetcher was created for.
  virtual const std::vector<const autofill::PasswordForm*>&
  GetSuppressedSameOrganizationNameForms() const = 0;

  // Whether querying suppressed forms (of all flavors) was attempted and did
  // complete at least once during the lifetime of this instance, regardless of
  // whether there have been any results.
  virtual bool DidCompleteQueryingSuppressedForms() const = 0;

  // Fetches stored matching logins. In addition the statistics is fetched on
  // platforms with the password bubble. This is called automatically during
  // construction and can be called manually later as well to cause an update
  // of the cached credentials.
  virtual void Fetch() = 0;

  // Creates a copy of |*this| with contains the same credentials without the
  // need for calling Fetch().
  virtual std::unique_ptr<FormFetcher> Clone() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(FormFetcher);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_FORM_FETCHER_H_
