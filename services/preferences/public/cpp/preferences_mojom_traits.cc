// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/public/cpp/preferences_mojom_traits.h"

namespace mojo {

using PrefStoreType = prefs::mojom::PrefStoreType;

PrefStoreType EnumTraits<PrefStoreType, PrefValueStore::PrefStoreType>::ToMojom(
    PrefValueStore::PrefStoreType input) {
  switch (input) {
    case PrefValueStore::INVALID_STORE:
      break;
    case PrefValueStore::MANAGED_STORE:
      return PrefStoreType::MANAGED;
    case PrefValueStore::SUPERVISED_USER_STORE:
      return PrefStoreType::SUPERVISED_USER;
    case PrefValueStore::EXTENSION_STORE:
      return PrefStoreType::EXTENSION;
    case PrefValueStore::COMMAND_LINE_STORE:
      return PrefStoreType::COMMAND_LINE;
    case PrefValueStore::USER_STORE:
      return PrefStoreType::USER;
    case PrefValueStore::RECOMMENDED_STORE:
      return PrefStoreType::RECOMMENDED;
    case PrefValueStore::DEFAULT_STORE:
      return PrefStoreType::DEFAULT;
  }
  NOTREACHED();
  return {};
}

bool EnumTraits<PrefStoreType, PrefValueStore::PrefStoreType>::FromMojom(
    PrefStoreType input,
    PrefValueStore::PrefStoreType* output) {
  switch (input) {
    case PrefStoreType::MANAGED:
      *output = PrefValueStore::MANAGED_STORE;
      return true;
    case PrefStoreType::SUPERVISED_USER:
      *output = PrefValueStore::SUPERVISED_USER_STORE;
      return true;
    case PrefStoreType::EXTENSION:
      *output = PrefValueStore::EXTENSION_STORE;
      return true;
    case PrefStoreType::COMMAND_LINE:
      *output = PrefValueStore::COMMAND_LINE_STORE;
      return true;
    case PrefStoreType::USER:
      *output = PrefValueStore::USER_STORE;
      return true;
    case PrefStoreType::RECOMMENDED:
      *output = PrefValueStore::RECOMMENDED_STORE;
      return true;
    case PrefStoreType::DEFAULT:
      *output = PrefValueStore::DEFAULT_STORE;
      return true;
  }
  return false;
}

using MojomReadError = prefs::mojom::PersistentPrefStoreConnection_ReadError;

MojomReadError
EnumTraits<MojomReadError, PersistentPrefStore::PrefReadError>::ToMojom(
    PersistentPrefStore::PrefReadError input) {
  switch (input) {
    case PersistentPrefStore::PREF_READ_ERROR_NONE:
      return MojomReadError::NONE;
    case PersistentPrefStore::PREF_READ_ERROR_JSON_PARSE:
      return MojomReadError::JSON_PARSE;
    case PersistentPrefStore::PREF_READ_ERROR_JSON_TYPE:
      return MojomReadError::JSON_TYPE;
    case PersistentPrefStore::PREF_READ_ERROR_ACCESS_DENIED:
      return MojomReadError::ACCESS_DENIED;
    case PersistentPrefStore::PREF_READ_ERROR_FILE_OTHER:
      return MojomReadError::FILE_OTHER;
    case PersistentPrefStore::PREF_READ_ERROR_FILE_LOCKED:
      return MojomReadError::FILE_LOCKED;
    case PersistentPrefStore::PREF_READ_ERROR_NO_FILE:
      return MojomReadError::NO_FILE;
    case PersistentPrefStore::PREF_READ_ERROR_JSON_REPEAT:
      return MojomReadError::JSON_REPEAT;
    case PersistentPrefStore::PREF_READ_ERROR_FILE_NOT_SPECIFIED:
      return MojomReadError::FILE_NOT_SPECIFIED;
    case PersistentPrefStore::PREF_READ_ERROR_ASYNCHRONOUS_TASK_INCOMPLETE:
      return MojomReadError::ASYNCHRONOUS_TASK_INCOMPLETE;
    case PersistentPrefStore::PREF_READ_ERROR_MAX_ENUM:
      break;
  }
  NOTREACHED();
  return {};
}

bool EnumTraits<MojomReadError, PersistentPrefStore::PrefReadError>::FromMojom(
    MojomReadError input,
    PersistentPrefStore::PrefReadError* output) {
  switch (input) {
    case MojomReadError::NONE:
      *output = PersistentPrefStore::PREF_READ_ERROR_NONE;
      return true;
    case MojomReadError::JSON_PARSE:
      *output = PersistentPrefStore::PREF_READ_ERROR_JSON_PARSE;
      return true;
    case MojomReadError::JSON_TYPE:
      *output = PersistentPrefStore::PREF_READ_ERROR_JSON_TYPE;
      return true;
    case MojomReadError::ACCESS_DENIED:
      *output = PersistentPrefStore::PREF_READ_ERROR_ACCESS_DENIED;
      return true;
    case MojomReadError::FILE_OTHER:
      *output = PersistentPrefStore::PREF_READ_ERROR_FILE_OTHER;
      return true;
    case MojomReadError::FILE_LOCKED:
      *output = PersistentPrefStore::PREF_READ_ERROR_FILE_LOCKED;
      return true;
    case MojomReadError::NO_FILE:
      *output = PersistentPrefStore::PREF_READ_ERROR_NO_FILE;
      return true;
    case MojomReadError::JSON_REPEAT:
      *output = PersistentPrefStore::PREF_READ_ERROR_JSON_REPEAT;
      return true;
    case MojomReadError::FILE_NOT_SPECIFIED:
      *output = PersistentPrefStore::PREF_READ_ERROR_FILE_NOT_SPECIFIED;
      return true;
    case MojomReadError::ASYNCHRONOUS_TASK_INCOMPLETE:
      *output =
          PersistentPrefStore::PREF_READ_ERROR_ASYNCHRONOUS_TASK_INCOMPLETE;
      return true;
  }
  return false;
}

}  // namespace mojo
