// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/prefs/pref_connector_service.h"

#include "base/bind.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context.h"

AshPrefConnector::AshPrefConnector() : weak_factory_(this) {
  registry_.AddInterface<ash::mojom::PrefConnector>(base::Bind(
      &AshPrefConnector::BindConnectorRequest, base::Unretained(this)));
}

AshPrefConnector::~AshPrefConnector() = default;

void AshPrefConnector::GetPrefStoreConnectorForSigninScreen(
    prefs::mojom::PrefStoreConnectorRequest request) {
  // The signin screen profile is incognito and is not associated with a
  // specific user.
  Profile* profile = chromeos::ProfileHelper::Get()->GetSigninProfile();
  DCHECK(profile->IsOffTheRecord());
  content::BrowserContext::GetConnectorFor(profile)->BindInterface(
      prefs::mojom::kServiceName, std::move(request));
}

void AshPrefConnector::GetPrefStoreConnectorForUser(
    const AccountId& account_id,
    prefs::mojom::PrefStoreConnectorRequest request) {
  user_manager::User* user =
      user_manager::UserManager::Get()->FindUserAndModify(account_id);
  if (!user)
    return;

  Profile* profile = chromeos::ProfileHelper::Get()->GetProfileByUser(user);
  if (!profile) {
    user->AddProfileCreatedObserver(base::BindOnce(
        &AshPrefConnector::GetPrefStoreConnectorForUser,
        weak_factory_.GetWeakPtr(), account_id, std::move(request)));
    return;
  }

  content::BrowserContext::GetConnectorFor(profile)->BindInterface(
      prefs::mojom::kServiceName, std::move(request));
}

void AshPrefConnector::BindConnectorRequest(
    ash::mojom::PrefConnectorRequest request) {
  connector_bindings_.AddBinding(this, std::move(request));
}

void AshPrefConnector::OnStart() {}

void AshPrefConnector::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}
