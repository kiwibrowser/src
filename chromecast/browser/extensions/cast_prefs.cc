// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/extensions/cast_prefs.h"

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/api/audio/audio_api.h"
#include "extensions/browser/extension_prefs.h"

using base::FilePath;
using user_prefs::PrefRegistrySyncable;

namespace extensions {
namespace {

// Creates a JsonPrefStore from a file at |filepath| and synchronously loads
// the preferences.
scoped_refptr<JsonPrefStore> CreateAndLoadPrefStore(const FilePath& filepath) {
  scoped_refptr<JsonPrefStore> pref_store =
      base::MakeRefCounted<JsonPrefStore>(filepath);
  pref_store->ReadPrefs();  // Synchronous.
  return pref_store;
}

}  // namespace

namespace cast_prefs {

std::unique_ptr<PrefService> CreateUserPrefService(
    content::BrowserContext* browser_context) {
  FilePath filepath = browser_context->GetPath().AppendASCII("user_prefs.json");
  scoped_refptr<JsonPrefStore> pref_store = CreateAndLoadPrefStore(filepath);

  PrefServiceFactory factory;
  factory.set_user_prefs(pref_store);

  // Prefs should be registered before the PrefService is created.
  PrefRegistrySyncable* pref_registry = new PrefRegistrySyncable;
  ExtensionPrefs::RegisterProfilePrefs(pref_registry);
  AudioAPI::RegisterUserPrefs(pref_registry);

  std::unique_ptr<PrefService> pref_service = factory.Create(pref_registry);
  user_prefs::UserPrefs::Set(browser_context, pref_service.get());
  return pref_service;
}

}  // namespace cast_prefs

}  // namespace extensions
