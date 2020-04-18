// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cdm/browser/media_drm_storage_impl.h"

#include <memory>

#include "base/logging.h"
#include "base/value_conversions.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/navigation_handle.h"

// The storage will be managed by PrefService. All data will be stored in a
// dictionary under the key "media.media_drm_storage". The dictionary is
// structured as follows:
//
// {
//     $origin: {
//         "origin_id": $unguessable_origin_id
//         "creation_time": $creation_time
//         "sessions" : {
//             $session_id: {
//                 "key_set_id": $key_set_id,
//                 "mime_type": $mime_type,
//                 "creation_time": $creation_time
//             },
//             # more session_id map...
//         }
//     },
//     # more origin map...
// }

namespace cdm {

namespace {

const char kMediaDrmStorage[] = "media.media_drm_storage";
const char kCreationTime[] = "creation_time";
const char kSessions[] = "sessions";
const char kKeySetId[] = "key_set_id";
const char kMimeType[] = "mime_type";
const char kOriginId[] = "origin_id";

// Extract base::Time from |dict| with key kCreationTime. Returns true if |dict|
// contains a valid time value.
bool GetTimeFromDict(const base::DictionaryValue& dict, base::Time* time) {
  DCHECK(time);

  double time_double = 0.;
  if (!dict.GetDouble(kCreationTime, &time_double))
    return false;

  base::Time time_maybe_null = base::Time::FromDoubleT(time_double);
  if (time_maybe_null.is_null())
    return false;

  *time = time_maybe_null;
  return true;
}

// Data in origin dict without sessions dict.
class OriginData {
 public:
  explicit OriginData(const base::UnguessableToken& origin_id)
      : OriginData(origin_id, base::Time::Now()) {
    DCHECK(origin_id_);
  }

  const base::UnguessableToken& origin_id() const { return origin_id_; }

  base::Time provision_time() const { return provision_time_; }

  std::unique_ptr<base::DictionaryValue> ToDictValue() const {
    auto dict = std::make_unique<base::DictionaryValue>();

    dict->Set(kOriginId, base::CreateUnguessableTokenValue(origin_id_));
    dict->SetDouble(kCreationTime, provision_time_.ToDoubleT());

    return dict;
  }

  // Convert |origin_dict| to OriginData. |origin_dict| contains information
  // related to origin provision. Return nullptr if |origin_dict| has any
  // corruption, e.g. format error, missing fields, invalid value.
  static std::unique_ptr<OriginData> FromDictValue(
      const base::DictionaryValue& origin_dict) {
    const base::Value* origin_id_value = nullptr;
    if (!origin_dict.Get(kOriginId, &origin_id_value))
      return nullptr;

    base::UnguessableToken origin_id;
    if (!base::GetValueAsUnguessableToken(*origin_id_value, &origin_id))
      return nullptr;

    base::Time time;
    if (!GetTimeFromDict(origin_dict, &time))
      return nullptr;

    return base::WrapUnique(new OriginData(origin_id, time));
  }

 private:
  OriginData(const base::UnguessableToken& origin_id, base::Time time)
      : origin_id_(origin_id), provision_time_(time) {}

  base::UnguessableToken origin_id_;
  base::Time provision_time_;
};

// Data in session dict.
class SessionData {
 public:
  SessionData(const std::vector<uint8_t>& key_set_id,
              const std::string& mime_type)
      : SessionData(key_set_id, mime_type, base::Time::Now()) {}

  base::Time creation_time() const { return creation_time_; }

  std::unique_ptr<base::DictionaryValue> ToDictValue() const {
    auto dict = std::make_unique<base::DictionaryValue>();

    dict->SetString(
        kKeySetId,
        std::string(reinterpret_cast<const char*>(key_set_id_.data()),
                    key_set_id_.size()));
    dict->SetString(kMimeType, mime_type_);
    dict->SetDouble(kCreationTime, creation_time_.ToDoubleT());

    return dict;
  }

  media::mojom::SessionDataPtr ToMojo() const {
    return media::mojom::SessionData::New(key_set_id_, mime_type_);
  }

  // Convert |session_dict| to SessionData. |session_dict| contains information
  // for an offline license session. Return nullptr if |session_dict| has any
  // corruption, e.g. format error, missing fields, invalid data.
  static std::unique_ptr<SessionData> FromDictValue(
      const base::DictionaryValue& session_dict) {
    std::string key_set_id_string;
    if (!session_dict.GetString(kKeySetId, &key_set_id_string))
      return nullptr;

    std::string mime_type;
    if (!session_dict.GetString(kMimeType, &mime_type))
      return nullptr;

    base::Time time;
    if (!GetTimeFromDict(session_dict, &time))
      return nullptr;

    return base::WrapUnique(
        new SessionData(std::vector<uint8_t>(key_set_id_string.begin(),
                                             key_set_id_string.end()),
                        std::move(mime_type), time));
  }

 private:
  SessionData(std::vector<uint8_t> key_set_id,
              std::string mime_type,
              base::Time time)
      : key_set_id_(std::move(key_set_id)),
        mime_type_(std::move(mime_type)),
        creation_time_(time) {}

  std::vector<uint8_t> key_set_id_;
  std::string mime_type_;
  base::Time creation_time_;
};

// Get sessions dict for |origin_string| in |storage_dict|. This is a helper
// function that works for both const and non const access.
template <typename DictValue>
DictValue* GetSessionsDictFromStorageDict(DictValue* storage_dict,
                                          const std::string& origin_string) {
  if (storage_dict == nullptr) {
    DVLOG(1) << __func__ << ": No storage dict for origin " << origin_string;
    return nullptr;
  }

  DictValue* origin_dict = nullptr;
  // The origin string may contain dots. Do not use path expansion.
  storage_dict->GetDictionaryWithoutPathExpansion(origin_string, &origin_dict);
  if (!origin_dict) {
    DVLOG(1) << __func__ << ": No entry for origin " << origin_string;
    return nullptr;
  }

  DictValue* sessions_dict = nullptr;
  if (!origin_dict->GetDictionary(kSessions, &sessions_dict)) {
    DVLOG(1) << __func__ << ": No sessions entry for origin " << origin_string;
    return nullptr;
  }

  return sessions_dict;
}

// Create origin dict with empty sessions dict. It returns the sessions dict for
// caller to write session information.
base::Value* CreateOriginDictAndReturnSessionsDict(
    base::Value* storage_dict,
    const std::string& origin,
    const base::UnguessableToken& origin_id) {
  DCHECK(storage_dict);

  // TODO(yucliu): Change to base::Value::SetKey.
  return storage_dict
      ->SetKey(origin, base::Value::FromUniquePtrValue(
                           OriginData(origin_id).ToDictValue()))
      ->SetKey(kSessions, base::Value(base::Value::Type::DICTIONARY));
}

// Clear sessions whose creation time falls in [start, end] from
// |sessions_dict|. This function also cleans corruption data and should never
// fail.
void ClearSessionDataForTimePeriod(base::DictionaryValue* sessions_dict,
                                   base::Time start,
                                   base::Time end) {
  std::vector<std::string> sessions_to_clear;
  for (const auto& key_value : *sessions_dict) {
    const std::string& session_id = key_value.first;

    base::DictionaryValue* session_dict;
    if (!key_value.second->GetAsDictionary(&session_dict)) {
      DLOG(WARNING) << "Session dict for " << session_id
                    << " is corrupted, removing.";
      sessions_to_clear.push_back(session_id);
      continue;
    }

    std::unique_ptr<SessionData> session_data =
        SessionData::FromDictValue(*session_dict);
    if (!session_data) {
      DLOG(WARNING) << "Session data for " << session_id
                    << " is corrupted, removing.";
      sessions_to_clear.push_back(session_id);
      continue;
    }

    if (session_data->creation_time() >= start &&
        session_data->creation_time() <= end) {
      sessions_to_clear.push_back(session_id);
      continue;
    }
  }

  // Remove session data.
  for (const auto& session_id : sessions_to_clear)
    sessions_dict->RemoveWithoutPathExpansion(session_id, nullptr);
}

// 1. Removes the session data from origin dict if the session's creation time
// falls in [|start|, |end|] and |filter| returns true on its origin.
// 2. Removes the origin data if all of the sessions are removed.
// 3. Returns a list of origin IDs to unprovision.
std::vector<base::UnguessableToken> ClearMatchingLicenseData(
    base::DictionaryValue* storage_dict,
    base::Time start,
    base::Time end,
    const base::RepeatingCallback<bool(const GURL&)>& filter) {
  std::vector<std::string> origins_to_delete;
  std::vector<base::UnguessableToken> origin_ids_to_unprovision;

  for (const auto& key_value : *storage_dict) {
    const std::string& origin_str = key_value.first;

    if (filter && !filter.Run(GURL(origin_str)))
      continue;

    base::DictionaryValue* origin_dict;
    if (!key_value.second->GetAsDictionary(&origin_dict)) {
      DLOG(WARNING) << "Origin dict for " << origin_str
                    << " is corrupted, removing.";
      origins_to_delete.push_back(origin_str);
      continue;
    }

    std::unique_ptr<OriginData> origin_data =
        OriginData::FromDictValue(*origin_dict);
    if (!origin_data) {
      DLOG(WARNING) << "Origin data for " << origin_str
                    << " is corrupted, removing.";
      origins_to_delete.push_back(origin_str);
      continue;
    }

    if (origin_data->provision_time() > end)
      continue;

    base::DictionaryValue* sessions;
    if (!origin_dict->GetDictionary(kSessions, &sessions)) {
      // The origin is provisioned, but no persistent license is installed.
      origins_to_delete.push_back(origin_str);
      origin_ids_to_unprovision.push_back(origin_data->origin_id());
      continue;
    }

    ClearSessionDataForTimePeriod(sessions, start, end);

    if (sessions->empty()) {
      // Session data will be removed when removing origin data.
      origins_to_delete.push_back(origin_str);
      origin_ids_to_unprovision.push_back(origin_data->origin_id());
    }
  }

  // Remove origin data.
  for (const auto& origin_str : origins_to_delete)
    storage_dict->RemoveWithoutPathExpansion(origin_str, nullptr);

  return origin_ids_to_unprovision;
}

}  // namespace

// static
void MediaDrmStorageImpl::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kMediaDrmStorage);
}

// static
std::set<GURL> MediaDrmStorageImpl::GetAllOrigins(
    const PrefService* pref_service) {
  DCHECK(pref_service);

  const base::DictionaryValue* storage_dict =
      pref_service->GetDictionary(kMediaDrmStorage);
  if (!storage_dict)
    return std::set<GURL>();

  std::set<GURL> origin_set;
  for (const auto& key_value : *storage_dict) {
    GURL origin(key_value.first);
    if (origin.is_valid())
      origin_set.insert(origin);
  }

  return origin_set;
}

// static
std::vector<base::UnguessableToken> MediaDrmStorageImpl::ClearMatchingLicenses(
    PrefService* pref_service,
    base::Time start,
    base::Time end,
    const base::RepeatingCallback<bool(const GURL&)>& filter) {
  DVLOG(1) << __func__ << ": Clear licenses [" << start << ", " << end << "]";

  DictionaryPrefUpdate update(pref_service, kMediaDrmStorage);

  return ClearMatchingLicenseData(update.Get(), start, end, filter);
}

// MediaDrmStorageImpl

MediaDrmStorageImpl::MediaDrmStorageImpl(
    content::RenderFrameHost* render_frame_host,
    PrefService* pref_service,
    media::mojom::MediaDrmStorageRequest request)
    : FrameServiceBase(render_frame_host, std::move(request)),
      pref_service_(pref_service) {
  DVLOG(1) << __func__ << ": origin = " << origin();
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(pref_service_);
  DCHECK(!origin().unique());
}

MediaDrmStorageImpl::~MediaDrmStorageImpl() {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void MediaDrmStorageImpl::Initialize(InitializeCallback callback) {
  DVLOG(1) << __func__ << ": origin = " << origin();
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!origin_id_);

  const base::DictionaryValue* storage_dict =
      pref_service_->GetDictionary(kMediaDrmStorage);

  const base::DictionaryValue* origin_dict = nullptr;
  // The origin string may contain dots. Do not use path expansion.
  bool exist = storage_dict && storage_dict->GetDictionaryWithoutPathExpansion(
                                   origin().Serialize(), &origin_dict);

  base::UnguessableToken origin_id;
  if (exist) {
    DCHECK(origin_dict);

    std::unique_ptr<OriginData> origin_data =
        OriginData::FromDictValue(*origin_dict);
    if (origin_data)
      origin_id = origin_data->origin_id();
  }

  // |origin_id| can be empty even if |origin_dict| exists. This can happen if
  // |origin_dict| is created with an old version app.
  if (origin_id.is_empty())
    origin_id = base::UnguessableToken::Create();

  origin_id_ = origin_id;

  DCHECK(origin_id);
  std::move(callback).Run(origin_id);
}

void MediaDrmStorageImpl::OnProvisioned(OnProvisionedCallback callback) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!IsInitialized()) {
    DVLOG(1) << __func__ << ": Not initialized.";
    std::move(callback).Run(false);
    return;
  }

  DictionaryPrefUpdate update(pref_service_, kMediaDrmStorage);
  base::DictionaryValue* storage_dict = update.Get();
  DCHECK(storage_dict);

  // The origin string may contain dots. Do not use path expansion.
  DVLOG_IF(1, storage_dict->FindKey(origin().Serialize()))
      << __func__ << ": Entry for origin " << origin()
      << " already exists and will be cleared";

  CreateOriginDictAndReturnSessionsDict(storage_dict, origin().Serialize(),
                                        origin_id_);
  std::move(callback).Run(true);
}

void MediaDrmStorageImpl::SavePersistentSession(
    const std::string& session_id,
    media::mojom::SessionDataPtr session_data,
    SavePersistentSessionCallback callback) {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!IsInitialized()) {
    DVLOG(1) << __func__ << ": Not initialized.";
    std::move(callback).Run(false);
    return;
  }

  DictionaryPrefUpdate update(pref_service_, kMediaDrmStorage);
  base::DictionaryValue* storage_dict = update.Get();
  DCHECK(storage_dict);

  base::Value* sessions_dict =
      GetSessionsDictFromStorageDict<base::DictionaryValue>(
          storage_dict, origin().Serialize());

  // This could happen if the profile is removed, but the device is still
  // provisioned for the origin. In this case, just create a new entry.
  // Since we're using random origin ID in MediaDrm, it's rare to enter the if
  // branch. Deleting the profile causes reprovisioning of the origin.
  if (!sessions_dict) {
    DVLOG(1) << __func__ << ": No entry for origin " << origin();
    sessions_dict = CreateOriginDictAndReturnSessionsDict(
        storage_dict, origin().Serialize(), origin_id_);
    DCHECK(sessions_dict);
  }

  DVLOG_IF(1, sessions_dict->FindKey(session_id))
      << __func__ << ": Session ID already exists and will be replaced.";

  sessions_dict->SetKey(session_id, base::Value::FromUniquePtrValue(
                                        SessionData(session_data->key_set_id,
                                                    session_data->mime_type)
                                            .ToDictValue()));

  std::move(callback).Run(true);
}

void MediaDrmStorageImpl::LoadPersistentSession(
    const std::string& session_id,
    LoadPersistentSessionCallback callback) {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!IsInitialized()) {
    DVLOG(1) << __func__ << ": Not initialized.";
    std::move(callback).Run(nullptr);
    return;
  }

  const base::DictionaryValue* sessions_dict =
      GetSessionsDictFromStorageDict<const base::DictionaryValue>(
          pref_service_->GetDictionary(kMediaDrmStorage), origin().Serialize());
  if (!sessions_dict) {
    std::move(callback).Run(nullptr);
    return;
  }

  const base::DictionaryValue* session_dict = nullptr;
  if (!sessions_dict->GetDictionaryWithoutPathExpansion(session_id,
                                                        &session_dict)) {
    DVLOG(1) << __func__ << ": No session " << session_id << " for origin "
             << origin();
    std::move(callback).Run(nullptr);
    return;
  }

  std::unique_ptr<SessionData> session_data =
      SessionData::FromDictValue(*session_dict);
  if (!session_data) {
    DLOG(WARNING) << __func__ << ": Failed to read session data.";
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(session_data->ToMojo());
}

void MediaDrmStorageImpl::RemovePersistentSession(
    const std::string& session_id,
    RemovePersistentSessionCallback callback) {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!IsInitialized()) {
    DVLOG(1) << __func__ << ": Not initialized.";
    std::move(callback).Run(false);
    return;
  }

  DictionaryPrefUpdate update(pref_service_, kMediaDrmStorage);

  base::DictionaryValue* sessions_dict =
      GetSessionsDictFromStorageDict<base::DictionaryValue>(
          update.Get(), origin().Serialize());

  if (!sessions_dict) {
    std::move(callback).Run(true);
    return;
  }

  sessions_dict->RemoveWithoutPathExpansion(session_id, nullptr);
  std::move(callback).Run(true);
}

}  // namespace cdm
