// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/signin_client.h"

#include "base/guid.h"
#include "base/logging.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_pref_names.h"

namespace {
const char kEphemeralUserDeviceIDPrefix[] = "t_";
}

// static
std::string SigninClient::GenerateSigninScopedDeviceID(bool for_ephemeral) {
  std::string guid = base::GenerateGUID();
  return for_ephemeral ? kEphemeralUserDeviceIDPrefix + guid : guid;
}

std::string SigninClient::GetOrCreateScopedDeviceIdPref(PrefService* prefs) {
  std::string signin_scoped_device_id =
      prefs->GetString(prefs::kGoogleServicesSigninScopedDeviceId);
  if (signin_scoped_device_id.empty()) {
    // If device_id doesn't exist then generate new and save in prefs.
    signin_scoped_device_id = GenerateSigninScopedDeviceID(false);
    DCHECK(!signin_scoped_device_id.empty());
    prefs->SetString(prefs::kGoogleServicesSigninScopedDeviceId,
                     signin_scoped_device_id);
  }
  return signin_scoped_device_id;
}

void SigninClient::PreSignOut(
    const base::Callback<void()>& sign_out,
    signin_metrics::ProfileSignout signout_source_metric) {
  sign_out.Run();
}

void SigninClient::PreGaiaLogout(base::OnceClosure callback) {
  std::move(callback).Run();
}

void SigninClient::SignOut() {
  GetPrefs()->ClearPref(prefs::kGoogleServicesSigninScopedDeviceId);
  OnSignedOut();
}
