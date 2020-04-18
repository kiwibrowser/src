// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/auth/auth_prewarmer.h"

#include <stddef.h>

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/resource_hints.h"
#include "google_apis/gaia/gaia_urls.h"
#include "url/gurl.h"

namespace chromeos {

AuthPrewarmer::AuthPrewarmer() : doing_prewarm_(false) {}

AuthPrewarmer::~AuthPrewarmer() {
  if (registrar_.IsRegistered(
          this,
          chrome::NOTIFICATION_PROFILE_URL_REQUEST_CONTEXT_GETTER_INITIALIZED,
          content::Source<Profile>(ProfileHelper::GetSigninProfile()))) {
    registrar_.Remove(
        this,
        chrome::NOTIFICATION_PROFILE_URL_REQUEST_CONTEXT_GETTER_INITIALIZED,
        content::Source<Profile>(ProfileHelper::GetSigninProfile()));
  }
  NetworkHandler::Get()->network_state_handler()->RemoveObserver(this,
                                                                 FROM_HERE);
}

void AuthPrewarmer::PrewarmAuthentication(
    base::OnceClosure completion_callback) {
  if (doing_prewarm_) {
    LOG(ERROR) << "PrewarmAuthentication called twice.";
    return;
  }
  doing_prewarm_ = true;
  completion_callback_ = std::move(completion_callback);
  if (GetRequestContext() && IsNetworkConnected()) {
    DoPrewarm();
    return;
  }
  if (!IsNetworkConnected()) {
    // DefaultNetworkChanged will get called when a network becomes connected.
    NetworkHandler::Get()->network_state_handler()->AddObserver(this,
                                                                FROM_HERE);
  }
  if (!GetRequestContext()) {
    registrar_.Add(
        this,
        chrome::NOTIFICATION_PROFILE_URL_REQUEST_CONTEXT_GETTER_INITIALIZED,
        content::Source<Profile>(ProfileHelper::GetSigninProfile()));
  }
}

void AuthPrewarmer::DefaultNetworkChanged(const NetworkState* network) {
  if (!network)
    return;  // Still no default (connected) network.

  NetworkHandler::Get()->network_state_handler()->RemoveObserver(this,
                                                                 FROM_HERE);
  if (GetRequestContext())
    DoPrewarm();
}

void AuthPrewarmer::Observe(int type,
                            const content::NotificationSource& source,
                            const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_PROFILE_URL_REQUEST_CONTEXT_GETTER_INITIALIZED,
            type);
  registrar_.Remove(
      this, chrome::NOTIFICATION_PROFILE_URL_REQUEST_CONTEXT_GETTER_INITIALIZED,
      content::Source<Profile>(ProfileHelper::GetSigninProfile()));
  if (IsNetworkConnected())
    DoPrewarm();
}

void AuthPrewarmer::DoPrewarm() {
  const int kConnectionsNeeded = 1;
  const GURL& url = GaiaUrls::GetInstance()->service_login_url();
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&content::PreconnectUrl,
                     base::RetainedRef(GetRequestContext()), url, url,
                     kConnectionsNeeded, true));
  if (!completion_callback_.is_null()) {
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                     std::move(completion_callback_));
  }
}

bool AuthPrewarmer::IsNetworkConnected() const {
  NetworkStateHandler* nsh = NetworkHandler::Get()->network_state_handler();
  return (nsh->ConnectedNetworkByType(NetworkTypePattern::Default()) != NULL);
}

net::URLRequestContextGetter* AuthPrewarmer::GetRequestContext() const {
  return login::GetSigninContext();
}

}  // namespace chromeos
