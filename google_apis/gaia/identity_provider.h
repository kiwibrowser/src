// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_IDENTITY_PROVIDER_H_
#define GOOGLE_APIS_GAIA_IDENTITY_PROVIDER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "google_apis/gaia/oauth2_token_service.h"

// Helper class that provides access to information about logged-in GAIA
// accounts. Each instance of this class references an entity who may be logged
// in to zero, one or multiple GAIA accounts. The class provides access to the
// OAuth tokens for all logged-in accounts and indicates which of these is
// currently active.
// The main purpose of this abstraction layer is to isolate consumers of GAIA
// information from the different sources and various token service
// implementations. Whenever possible, consumers of GAIA information should be
// provided with an instance of this class instead of accessing other GAIA APIs
// directly.
class IdentityProvider : public OAuth2TokenService::Observer {
 public:
  class Observer {
   public:
    // Called when a GAIA account logs in and becomes the active account. All
    // account information is available when this method is called and all
    // |IdentityProvider| methods will return valid data.
    virtual void OnActiveAccountLogin() {}

    // Called when the active GAIA account logs out. The account information may
    // have been cleared already when this method is called. The
    // |IdentityProvider| methods may return inconsistent or outdated
    // information if called from within OnLogout().
    virtual void OnActiveAccountLogout() {}

   protected:
    virtual ~Observer();
  };

  ~IdentityProvider() override;

  // Adds and removes observers that will be notified of changes to the refresh
  // token availability for the active account.
  void AddActiveAccountRefreshTokenObserver(
      OAuth2TokenService::Observer* observer);
  void RemoveActiveAccountRefreshTokenObserver(
      OAuth2TokenService::Observer* observer);

  // Gets the active account's user name.
  virtual std::string GetActiveUsername() = 0;

  // Gets the active account's account ID.
  virtual std::string GetActiveAccountId() = 0;

  // Gets the token service vending OAuth tokens for all logged-in accounts.
  virtual OAuth2TokenService* GetTokenService() = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // OAuth2TokenService::Observer:
  void OnRefreshTokenAvailable(const std::string& account_id) override;
  void OnRefreshTokenRevoked(const std::string& account_id) override;
  void OnRefreshTokensLoaded() override;

 protected:
  IdentityProvider();

  // Fires an OnActiveAccountLogin notification.
  void FireOnActiveAccountLogin();

  // Fires an OnActiveAccountLogout notification.
  void FireOnActiveAccountLogout();

 private:
  base::ObserverList<Observer, true> observers_;
  base::ObserverList<OAuth2TokenService::Observer, true>
      token_service_observers_;
  int token_service_observer_count_;

  DISALLOW_COPY_AND_ASSIGN(IdentityProvider);
};

#endif  // GOOGLE_APIS_GAIA_IDENTITY_PROVIDER_H_
