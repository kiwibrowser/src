// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/metadata_table_chromeos.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"

namespace component_updater {

namespace {

// Path to the DictionaryValue in PrefService.
constexpr char kMetadataPrefPath[] = "component_updater_metadata";

// Schema of the DictionaryValue:
// {
//   |kMetadataContentKey|:
//   {
//     {
//       |kMetadataContentItemHashedUserIdKey|: |hashed_user_id|,
//       |kMetadataContentItemComponentKey|: |component|,
//     },
//     ...
//   }
// }
//
// Key to the content (installed items) in the DictionaryValue.
constexpr char kMetadataContentKey[] = "installed_items";

// Key to the hashed user id that installs the component.
constexpr char kMetadataContentItemHashedUserIdKey[] = "hashed_user_id";
// Key to the component name.
constexpr char kMetadataContentItemComponentKey[] = "component";

// Gets current active user.
const user_manager::User* GetActiveUser() {
  DCHECK(user_manager::UserManager::Get());

  return user_manager::UserManager::Get()->GetActiveUser();
}

// Converts username to a hashed string.
std::string HashUsername(const std::string& username) {
  chromeos::SystemSaltGetter* salt_getter = chromeos::SystemSaltGetter::Get();
  DCHECK(salt_getter);

  // System salt must be defined at this point.
  const chromeos::SystemSaltGetter::RawSalt* salt = salt_getter->GetRawSalt();
  DCHECK(salt);

  unsigned char binmd[base::kSHA1Length];
  std::string lowercase(username);
  std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
                 ::tolower);
  std::vector<uint8_t> data = *salt;
  std::copy(lowercase.begin(), lowercase.end(), std::back_inserter(data));
  base::SHA1HashBytes(data.data(), data.size(), binmd);
  std::string result = base::HexEncode(binmd, sizeof(binmd));
  // Stay compatible with CryptoLib::HexEncodeToBuffer()
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

}  // namespace

MetadataTable::~MetadataTable() = default;

// static
std::unique_ptr<component_updater::MetadataTable> MetadataTable::Create(
    PrefService* pref_service) {
  return base::WrapUnique(new MetadataTable(pref_service));
}

// static
std::unique_ptr<component_updater::MetadataTable>
MetadataTable::CreateForTest() {
  return base::WrapUnique(new MetadataTable());
}

// static
void MetadataTable::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kMetadataPrefPath);
}

bool MetadataTable::AddComponentForCurrentUser(
    const std::string& component_name) {
  const user_manager::User* active_user = GetActiveUser();
  // Return immediately if action is performed when no user is signed in.
  if (!active_user)
    return false;

  const std::string hashed_user_id =
      HashUsername(active_user->GetAccountId().GetUserEmail());
  AddItem(hashed_user_id, component_name);
  Store();
  return true;
}

bool MetadataTable::DeleteComponentForCurrentUser(
    const std::string& component_name) {
  const user_manager::User* active_user = GetActiveUser();
  // Return immediately if action is performed when no user is signed in.
  if (!active_user)
    return false;

  const std::string hashed_user_id =
      HashUsername(active_user->GetAccountId().GetUserEmail());
  if (!DeleteItem(hashed_user_id, component_name))
    return false;
  Store();
  return true;
}

bool MetadataTable::HasComponentForAnyUser(
    const std::string& component_name) const {
  return std::any_of(
      installed_items_.GetList().begin(), installed_items_.GetList().end(),
      [&component_name](const base::Value& item) {
        const base::Value* name = item.FindKeyOfType(
            kMetadataContentItemComponentKey, base::Value::Type::STRING);
        DCHECK(name);
        return name->GetString() == component_name;
      });
}

MetadataTable::MetadataTable(PrefService* pref_service)
    : pref_service_(pref_service) {
  DCHECK(pref_service_);
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Load();
}

MetadataTable::MetadataTable()
    : installed_items_(base::Value::Type::LIST), pref_service_(nullptr) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

void MetadataTable::Load() {
  DCHECK(pref_service_);
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const base::DictionaryValue* dict =
      pref_service_->GetDictionary(kMetadataPrefPath);
  DCHECK(dict);
  const base::Value* installed_items;
  if (dict->Get(kMetadataContentKey, &installed_items) &&
      installed_items->is_list()) {
    installed_items_ = installed_items->Clone();
    return;
  }
  installed_items_ = base::Value(base::Value::Type::LIST);
  Store();
}

void MetadataTable::Store() {
  DCHECK(pref_service_);
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DictionaryPrefUpdate update(pref_service_, kMetadataPrefPath);
  update->SetKey(kMetadataContentKey, installed_items_.Clone());
}

void MetadataTable::AddItem(const std::string& hashed_user_id,
                            const std::string& component_name) {
  if (HasComponentForUser(hashed_user_id, component_name))
    return;

  base::Value item(base::Value::Type::DICTIONARY);
  item.SetKey(kMetadataContentItemHashedUserIdKey, base::Value(hashed_user_id));
  item.SetKey(kMetadataContentItemComponentKey, base::Value(component_name));
  installed_items_.GetList().emplace_back(item.Clone());
}

bool MetadataTable::DeleteItem(const std::string& hashed_user_id,
                               const std::string& component_name) {
  auto item = GetInstalledItemIndex(hashed_user_id, component_name);
  if (item != installed_items_.GetList().end()) {
    installed_items_.GetList().erase(item);
    return true;
  }
  return false;
}

bool MetadataTable::HasComponentForUser(
    const std::string& hashed_user_id,
    const std::string& component_name) const {
  return GetInstalledItemIndex(hashed_user_id, component_name) !=
         installed_items_.GetList().end();
}

base::Value::ListStorage::const_iterator MetadataTable::GetInstalledItemIndex(
    const std::string& hashed_user_id,
    const std::string& component_name) const {
  for (auto it = installed_items_.GetList().begin();
       it != installed_items_.GetList().end(); ++it) {
    const base::Value* user_id = it->FindKeyOfType(
        kMetadataContentItemHashedUserIdKey, base::Value::Type::STRING);
    const base::Value* name = it->FindKeyOfType(
        kMetadataContentItemComponentKey, base::Value::Type::STRING);
    DCHECK(user_id);
    DCHECK(name);
    if (user_id->GetString() == hashed_user_id &&
        name->GetString() == component_name) {
      return it;
    }
  }
  return installed_items_.GetList().end();
}

}  // namespace component_updater
