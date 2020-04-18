# Preference Service User Guide

[TOC]

## What is the Preference Service?

Preferences, also known as "prefs", are key-value pairs stored by
Chrome. Examples include the settings in chrome://settings, all per-extension
metadata, the list of plugins and so on. Individual prefs are keyed by a string
and have a type. E.g., the "browser.enable_spellchecking" pref stores a boolean
indicating whether spell-checking is enabled.

The pref service persists prefs to disk and communicates updates to prefs
between services, including Chrome itself. There is a pref service instance per
profile (prefs are persisted on a per-profile basis).

## Using the service

The service is used through a client library that offers clients fast and
synchronous access to prefs. To connect to the service and start reading and
writing prefs simply use the `ConnectToPrefService` factory function:

``` cpp
#include "services/preferences/public/cpp/pref_service_factory.h"

class MyService : public service_manager::Service {
  void OnStart() {
    auto* connector = context()->connector();
    auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>();
    // Register any preferences you intend to use in |pref_registry|.
    prefs::ConnectToPrefService(
        connector, std::move(pref_registry), {},
        base::Bind(&MyService::OnPrefServiceConnected, base::Unretained(this)));
  }

  void OnPrefServiceConnected(std::unique_ptr<::PrefService> pref_service) {
    // Use |pref_service|.
  }
};
```

The returned `PrefService` class predates the Pref Service and its behavior
hasn't changed (i.e. all existing documentation still applies).

## Semantics

Updates made on the `PrefService` object are reflected immediately in the
originating service and eventually in all other services. In other words,
updates are eventually consistent.

## Registering your preferences

Every pref should be owned by one service. The owning service provides the type
and default value for that pref. Owned prefs can be registered as public,
meaning other services can read and/or write them, or private (the default). Services
that want to access a pref not owned by them must still register those prefs as
"foreign" prefs. Registration happens through the `PrefRegistry` passed to
`ConnectToPrefService`. For example:

**`//services/my_service/my_service.cc`**
``` cpp
void MyService::OnStart() {
  auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>();
  pref_registry->RegisterIntegerPref(kKey, kInitialValue, PrefRegistry::PUBLIC);
  prefs::ConnectToPrefService(...);
}
```

**`//services/other_service/other_service.cc`**
``` cpp
void OtherService::OnStart() {
  auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>();
  pref_registry->RegisterForeignPref(kKey);
  prefs::ConnectToPrefService(...);
}
```

## Design

The design doc is here:

https://docs.google.com/document/d/1JU8QUWxMEXWMqgkvFUumKSxr7Z-nfq0YvreSJTkMVmU/edit?usp=sharing
