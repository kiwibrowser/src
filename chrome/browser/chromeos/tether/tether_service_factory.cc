// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/tether/tether_service_factory.h"

#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/chromeos/cryptauth/chrome_cryptauth_service_factory.h"
#include "chrome/browser/chromeos/tether/fake_tether_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state_handler.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/session_manager/core/session_manager.h"

// static
TetherServiceFactory* TetherServiceFactory::GetInstance() {
  return base::Singleton<TetherServiceFactory>::get();
}

// static
TetherService* TetherServiceFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<TetherService*>(
      TetherServiceFactory::GetInstance()->GetServiceForBrowserContext(
          browser_context, true));
}

TetherServiceFactory::TetherServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "TetherService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(chromeos::ChromeCryptAuthServiceFactory::GetInstance());
}

TetherServiceFactory::~TetherServiceFactory() {}

KeyedService* TetherServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  DCHECK(chromeos::NetworkHandler::IsInitialized());

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(chromeos::switches::kTetherStub)) {
    FakeTetherService* fake_tether_service = new FakeTetherService(
        Profile::FromBrowserContext(context),
        chromeos::DBusThreadManager::Get()->GetPowerManagerClient(),
        chromeos::ChromeCryptAuthServiceFactory::GetForBrowserContext(
            Profile::FromBrowserContext(context)),
        chromeos::NetworkHandler::Get()->network_state_handler(),
        session_manager::SessionManager::Get());

    int num_tether_networks = 0;
    base::StringToInt(
        command_line->GetSwitchValueASCII(chromeos::switches::kTetherStub),
        &num_tether_networks);
    fake_tether_service->set_num_tether_networks(num_tether_networks);

    return fake_tether_service;
  }

  return new TetherService(
      Profile::FromBrowserContext(context),
      chromeos::DBusThreadManager::Get()->GetPowerManagerClient(),
      chromeos::ChromeCryptAuthServiceFactory::GetForBrowserContext(
          Profile::FromBrowserContext(context)),
      chromeos::NetworkHandler::Get()->network_state_handler(),
      session_manager::SessionManager::Get());
}

void TetherServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  TetherService::RegisterProfilePrefs(registry);
}
