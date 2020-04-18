// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_cookie_notifier.h"

#include "base/bind.h"
#include "base/location.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/net/chrome_cookie_notification_details.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/storage_partition.h"
#include "net/cookies/canonical_cookie.h"

ExtensionCookieNotifier::ExtensionCookieNotifier(Profile* profile)
    : profile_(profile), binding_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(profile);

  network::mojom::CookieManagerPtr manager_ptr;

  content::BrowserContext::GetDefaultStoragePartition(profile)
      ->GetNetworkContext()
      ->GetCookieManager(mojo::MakeRequest(&manager_ptr));

  network::mojom::CookieChangeListenerPtr listener_ptr;
  binding_.Bind(mojo::MakeRequest(&listener_ptr));
  manager_ptr->AddGlobalChangeListener(std::move(listener_ptr));
}

void ExtensionCookieNotifier::OnCookieChange(
    const net::CanonicalCookie& cookie,
    network::mojom::CookieChangeCause cause) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Confirm the profile hasn't gone away since this object was created.
  if (!g_browser_process->profile_manager()->IsValidProfile(profile_))
    return;

  ChromeCookieDetails cookie_details(
      &cookie, cause != network::mojom::CookieChangeCause::INSERTED, cause);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_COOKIE_CHANGED_FOR_EXTENSIONS,
      content::Source<Profile>(profile_),
      content::Details<ChromeCookieDetails>(&cookie_details));
}

ExtensionCookieNotifier::~ExtensionCookieNotifier() {
}
