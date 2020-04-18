// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/model_type.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "components/sync/protocol/app_notification_specifics.pb.h"
#include "components/sync/protocol/app_setting_specifics.pb.h"
#include "components/sync/protocol/app_specifics.pb.h"
#include "components/sync/protocol/autofill_specifics.pb.h"
#include "components/sync/protocol/bookmark_specifics.pb.h"
#include "components/sync/protocol/extension_setting_specifics.pb.h"
#include "components/sync/protocol/extension_specifics.pb.h"
#include "components/sync/protocol/nigori_specifics.pb.h"
#include "components/sync/protocol/password_specifics.pb.h"
#include "components/sync/protocol/preference_specifics.pb.h"
#include "components/sync/protocol/reading_list_specifics.pb.h"
#include "components/sync/protocol/search_engine_specifics.pb.h"
#include "components/sync/protocol/session_specifics.pb.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/protocol/theme_specifics.pb.h"
#include "components/sync/protocol/typed_url_specifics.pb.h"
#include "components/sync/syncable/syncable_proto_util.h"

namespace syncer {

struct ModelTypeInfo {
  ModelType model_type;
  // Model Type notification string.
  // This needs to match the corresponding proto message name in sync.proto
  const char* notification_type;
  // Root tag for Model Type
  // This should be the same as the model type but all lowercase.
  const char* root_tag;
  // String value for Model Type
  // This should be the same as the model type but space separated and the
  // first letter of every word capitalized.
  const char* model_type_string;
  // Field number of the model type specifics in EntitySpecifics.
  int specifics_field_number;
  // Model type value from SyncModelTypes enum in enums.xml. Must always be in
  // sync with the enum.
  int model_type_histogram_val;
};

// Below struct entries are in the same order as their definition in the
// ModelType enum. Don't forget to update the ModelType enum when you make
// changes to this list.
// Struct field values should be unique across the entire map.
const ModelTypeInfo kModelTypeInfoMap[] = {
    {UNSPECIFIED, "", "", "Unspecified", -1, 0},
    {TOP_LEVEL_FOLDER, "", "", "Top Level Folder", -1, 1},
    {BOOKMARKS, "BOOKMARK", "bookmarks", "Bookmarks",
     sync_pb::EntitySpecifics::kBookmarkFieldNumber, 2},
    {PREFERENCES, "PREFERENCE", "preferences", "Preferences",
     sync_pb::EntitySpecifics::kPreferenceFieldNumber, 3},
    {PASSWORDS, "PASSWORD", "passwords", "Passwords",
     sync_pb::EntitySpecifics::kPasswordFieldNumber, 4},
    {AUTOFILL_PROFILE, "AUTOFILL_PROFILE", "autofill_profiles",
     "Autofill Profiles", sync_pb::EntitySpecifics::kAutofillProfileFieldNumber,
     5},
    {AUTOFILL, "AUTOFILL", "autofill", "Autofill",
     sync_pb::EntitySpecifics::kAutofillFieldNumber, 6},
    {AUTOFILL_WALLET_DATA, "AUTOFILL_WALLET", "autofill_wallet",
     "Autofill Wallet", sync_pb::EntitySpecifics::kAutofillWalletFieldNumber,
     34},
    {AUTOFILL_WALLET_METADATA, "WALLET_METADATA", "autofill_wallet_metadata",
     "Autofill Wallet Metadata",
     sync_pb::EntitySpecifics::kWalletMetadataFieldNumber, 35},
    {THEMES, "THEME", "themes", "Themes",
     sync_pb::EntitySpecifics::kThemeFieldNumber, 7},
    {TYPED_URLS, "TYPED_URL", "typed_urls", "Typed URLs",
     sync_pb::EntitySpecifics::kTypedUrlFieldNumber, 8},
    {EXTENSIONS, "EXTENSION", "extensions", "Extensions",
     sync_pb::EntitySpecifics::kExtensionFieldNumber, 9},
    {SEARCH_ENGINES, "SEARCH_ENGINE", "search_engines", "Search Engines",
     sync_pb::EntitySpecifics::kSearchEngineFieldNumber, 10},
    {SESSIONS, "SESSION", "sessions", "Sessions",
     sync_pb::EntitySpecifics::kSessionFieldNumber, 11},
    {APPS, "APP", "apps", "Apps", sync_pb::EntitySpecifics::kAppFieldNumber,
     12},
    {APP_SETTINGS, "APP_SETTING", "app_settings", "App settings",
     sync_pb::EntitySpecifics::kAppSettingFieldNumber, 13},
    {EXTENSION_SETTINGS, "EXTENSION_SETTING", "extension_settings",
     "Extension settings",
     sync_pb::EntitySpecifics::kExtensionSettingFieldNumber, 14},
    {APP_NOTIFICATIONS, "APP_NOTIFICATION", "app_notifications",
     "App Notifications", sync_pb::EntitySpecifics::kAppNotificationFieldNumber,
     15},
    {HISTORY_DELETE_DIRECTIVES, "HISTORY_DELETE_DIRECTIVE",
     "history_delete_directives", "History Delete Directives",
     sync_pb::EntitySpecifics::kHistoryDeleteDirectiveFieldNumber, 16},
    {SYNCED_NOTIFICATIONS, "SYNCED_NOTIFICATION", "synced_notifications",
     "Synced Notifications",
     sync_pb::EntitySpecifics::kSyncedNotificationFieldNumber, 20},
    {SYNCED_NOTIFICATION_APP_INFO, "SYNCED_NOTIFICATION_APP_INFO",
     "synced_notification_app_info", "Synced Notification App Info",
     sync_pb::EntitySpecifics::kSyncedNotificationAppInfoFieldNumber, 31},
    {DICTIONARY, "DICTIONARY", "dictionary", "Dictionary",
     sync_pb::EntitySpecifics::kDictionaryFieldNumber, 22},
    {FAVICON_IMAGES, "FAVICON_IMAGE", "favicon_images", "Favicon Images",
     sync_pb::EntitySpecifics::kFaviconImageFieldNumber, 23},
    {FAVICON_TRACKING, "FAVICON_TRACKING", "favicon_tracking",
     "Favicon Tracking", sync_pb::EntitySpecifics::kFaviconTrackingFieldNumber,
     24},
    {DEVICE_INFO, "DEVICE_INFO", "device_info", "Device Info",
     sync_pb::EntitySpecifics::kDeviceInfoFieldNumber, 18},
    {PRIORITY_PREFERENCES, "PRIORITY_PREFERENCE", "priority_preferences",
     "Priority Preferences",
     sync_pb::EntitySpecifics::kPriorityPreferenceFieldNumber, 21},
    {SUPERVISED_USER_SETTINGS, "MANAGED_USER_SETTING", "managed_user_settings",
     "Managed User Settings",
     sync_pb::EntitySpecifics::kManagedUserSettingFieldNumber, 26},
    {DEPRECATED_SUPERVISED_USERS, "MANAGED_USER", "managed_users",
     "Managed Users", sync_pb::EntitySpecifics::kManagedUserFieldNumber, 27},
    {DEPRECATED_SUPERVISED_USER_SHARED_SETTINGS, "MANAGED_USER_SHARED_SETTING",
     "managed_user_shared_settings", "Managed User Shared Settings",
     sync_pb::EntitySpecifics::kManagedUserSharedSettingFieldNumber, 30},
    {ARTICLES, "ARTICLE", "articles", "Articles",
     sync_pb::EntitySpecifics::kArticleFieldNumber, 28},
    {APP_LIST, "APP_LIST", "app_list", "App List",
     sync_pb::EntitySpecifics::kAppListFieldNumber, 29},
    {WIFI_CREDENTIALS, "WIFI_CREDENTIAL", "wifi_credentials",
     "WiFi Credentials", sync_pb::EntitySpecifics::kWifiCredentialFieldNumber,
     32},
    {SUPERVISED_USER_WHITELISTS, "MANAGED_USER_WHITELIST",
     "managed_user_whitelists", "Managed User Whitelists",
     sync_pb::EntitySpecifics::kManagedUserWhitelistFieldNumber, 33},
    {ARC_PACKAGE, "ARC_PACKAGE", "arc_package", "Arc Package",
     sync_pb::EntitySpecifics::kArcPackageFieldNumber, 36},
    {PRINTERS, "PRINTER", "printers", "Printers",
     sync_pb::EntitySpecifics::kPrinterFieldNumber, 37},
    {READING_LIST, "READING_LIST", "reading_list", "Reading List",
     sync_pb::EntitySpecifics::kReadingListFieldNumber, 38},
    {USER_EVENTS, "USER_EVENT", "user_events", "User Events",
     sync_pb::EntitySpecifics::kUserEventFieldNumber, 39},
    {MOUNTAIN_SHARES, "MOUNTAIN_SHARE", "mountain_shares", "Mountain Shares",
     sync_pb::EntitySpecifics::kMountainShareFieldNumber, 40},
    // ---- Proxy types ----
    {PROXY_TABS, "", "", "Tabs", -1, 25},
    // ---- Control Types ----
    {NIGORI, "NIGORI", "nigori", "Encryption Keys",
     sync_pb::EntitySpecifics::kNigoriFieldNumber, 17},
    {EXPERIMENTS, "EXPERIMENTS", "experiments", "Experiments",
     sync_pb::EntitySpecifics::kExperimentsFieldNumber, 19},
};

static_assert(arraysize(kModelTypeInfoMap) == MODEL_TYPE_COUNT,
              "kModelTypeInfoMap should have MODEL_TYPE_COUNT elements");

void AddDefaultFieldValue(ModelType type, sync_pb::EntitySpecifics* specifics) {
  switch (type) {
    case UNSPECIFIED:
    case TOP_LEVEL_FOLDER:
      NOTREACHED() << "No default field value for " << ModelTypeToString(type);
      break;
    case BOOKMARKS:
      specifics->mutable_bookmark();
      break;
    case PREFERENCES:
      specifics->mutable_preference();
      break;
    case PASSWORDS:
      specifics->mutable_password();
      break;
    case AUTOFILL_PROFILE:
      specifics->mutable_autofill_profile();
      break;
    case AUTOFILL:
      specifics->mutable_autofill();
      break;
    case AUTOFILL_WALLET_DATA:
      specifics->mutable_autofill_wallet();
      break;
    case AUTOFILL_WALLET_METADATA:
      specifics->mutable_wallet_metadata();
      break;
    case THEMES:
      specifics->mutable_theme();
      break;
    case TYPED_URLS:
      specifics->mutable_typed_url();
      break;
    case EXTENSIONS:
      specifics->mutable_extension();
      break;
    case SEARCH_ENGINES:
      specifics->mutable_search_engine();
      break;
    case SESSIONS:
      specifics->mutable_session();
      break;
    case APPS:
      specifics->mutable_app();
      break;
    case APP_SETTINGS:
      specifics->mutable_app_setting();
      break;
    case EXTENSION_SETTINGS:
      specifics->mutable_extension_setting();
      break;
    case APP_NOTIFICATIONS:
      specifics->mutable_app_notification();
      break;
    case HISTORY_DELETE_DIRECTIVES:
      specifics->mutable_history_delete_directive();
      break;
    case SYNCED_NOTIFICATIONS:
      specifics->mutable_synced_notification();
      break;
    case SYNCED_NOTIFICATION_APP_INFO:
      specifics->mutable_synced_notification_app_info();
      break;
    case DICTIONARY:
      specifics->mutable_dictionary();
      break;
    case FAVICON_IMAGES:
      specifics->mutable_favicon_image();
      break;
    case FAVICON_TRACKING:
      specifics->mutable_favicon_tracking();
      break;
    case DEVICE_INFO:
      specifics->mutable_device_info();
      break;
    case PRIORITY_PREFERENCES:
      specifics->mutable_priority_preference();
      break;
    case SUPERVISED_USER_SETTINGS:
      specifics->mutable_managed_user_setting();
      break;
    case DEPRECATED_SUPERVISED_USERS:
      specifics->mutable_managed_user();
      break;
    case DEPRECATED_SUPERVISED_USER_SHARED_SETTINGS:
      specifics->mutable_managed_user_shared_setting();
      break;
    case ARTICLES:
      specifics->mutable_article();
      break;
    case APP_LIST:
      specifics->mutable_app_list();
      break;
    case WIFI_CREDENTIALS:
      specifics->mutable_wifi_credential();
      break;
    case SUPERVISED_USER_WHITELISTS:
      specifics->mutable_managed_user_whitelist();
      break;
    case ARC_PACKAGE:
      specifics->mutable_arc_package();
      break;
    case PRINTERS:
      specifics->mutable_printer();
      break;
    case READING_LIST:
      specifics->mutable_reading_list();
      break;
    case USER_EVENTS:
      specifics->mutable_user_event();
      break;
    case MOUNTAIN_SHARES:
      specifics->mutable_mountain_share();
      break;
    case PROXY_TABS:
      NOTREACHED() << "No default field value for " << ModelTypeToString(type);
      break;
    case NIGORI:
      specifics->mutable_nigori();
      break;
    case EXPERIMENTS:
      specifics->mutable_experiments();
      break;
    case MODEL_TYPE_COUNT:
      NOTREACHED() << "No default field value for " << ModelTypeToString(type);
      break;
  }
}

ModelType GetModelTypeFromSpecificsFieldNumber(int field_number) {
  ModelTypeSet protocol_types = ProtocolTypes();
  for (ModelTypeSet::Iterator iter = protocol_types.First(); iter.Good();
       iter.Inc()) {
    if (GetSpecificsFieldNumberFromModelType(iter.Get()) == field_number)
      return iter.Get();
  }
  return UNSPECIFIED;
}

int GetSpecificsFieldNumberFromModelType(ModelType model_type) {
  DCHECK(ProtocolTypes().Has(model_type))
      << "Only protocol types have field values.";
  if (ProtocolTypes().Has(model_type))
    return kModelTypeInfoMap[model_type].specifics_field_number;
  NOTREACHED() << "No known extension for model type.";
  return 0;
}

FullModelTypeSet ToFullModelTypeSet(ModelTypeSet in) {
  FullModelTypeSet out;
  for (ModelTypeSet::Iterator i = in.First(); i.Good(); i.Inc()) {
    out.Put(i.Get());
  }
  return out;
}

// Note: keep this consistent with GetModelType in entry.cc!
ModelType GetModelType(const sync_pb::SyncEntity& sync_entity) {
  DCHECK(!IsRoot(sync_entity));  // Root shouldn't ever go over the wire.

  // Backwards compatibility with old (pre-specifics) protocol.
  if (sync_entity.has_bookmarkdata())
    return BOOKMARKS;

  ModelType specifics_type = GetModelTypeFromSpecifics(sync_entity.specifics());
  if (specifics_type != UNSPECIFIED)
    return specifics_type;

  // Loose check for server-created top-level folders that aren't
  // bound to a particular model type.
  if (!sync_entity.server_defined_unique_tag().empty() &&
      IsFolder(sync_entity)) {
    return TOP_LEVEL_FOLDER;
  }

  // This is an item of a datatype we can't understand. Maybe it's
  // from the future?  Either we mis-encoded the object, or the
  // server sent us entries it shouldn't have.
  NOTREACHED() << "Unknown datatype in sync proto.";
  return UNSPECIFIED;
}

ModelType GetModelTypeFromSpecifics(const sync_pb::EntitySpecifics& specifics) {
  static_assert(41 == MODEL_TYPE_COUNT,
                "When adding new protocol types, the following type lookup "
                "logic must be updated.");
  if (specifics.has_bookmark())
    return BOOKMARKS;
  if (specifics.has_preference())
    return PREFERENCES;
  if (specifics.has_password())
    return PASSWORDS;
  if (specifics.has_autofill_profile())
    return AUTOFILL_PROFILE;
  if (specifics.has_autofill())
    return AUTOFILL;
  if (specifics.has_autofill_wallet())
    return AUTOFILL_WALLET_DATA;
  if (specifics.has_wallet_metadata())
    return AUTOFILL_WALLET_METADATA;
  if (specifics.has_theme())
    return THEMES;
  if (specifics.has_typed_url())
    return TYPED_URLS;
  if (specifics.has_extension())
    return EXTENSIONS;
  if (specifics.has_search_engine())
    return SEARCH_ENGINES;
  if (specifics.has_session())
    return SESSIONS;
  if (specifics.has_app())
    return APPS;
  if (specifics.has_app_setting())
    return APP_SETTINGS;
  if (specifics.has_extension_setting())
    return EXTENSION_SETTINGS;
  if (specifics.has_app_notification())
    return APP_NOTIFICATIONS;
  if (specifics.has_history_delete_directive())
    return HISTORY_DELETE_DIRECTIVES;
  if (specifics.has_synced_notification())
    return SYNCED_NOTIFICATIONS;
  if (specifics.has_synced_notification_app_info())
    return SYNCED_NOTIFICATION_APP_INFO;
  if (specifics.has_dictionary())
    return DICTIONARY;
  if (specifics.has_favicon_image())
    return FAVICON_IMAGES;
  if (specifics.has_favicon_tracking())
    return FAVICON_TRACKING;
  if (specifics.has_device_info())
    return DEVICE_INFO;
  if (specifics.has_priority_preference())
    return PRIORITY_PREFERENCES;
  if (specifics.has_managed_user_setting())
    return SUPERVISED_USER_SETTINGS;
  if (specifics.has_managed_user())
    return DEPRECATED_SUPERVISED_USERS;
  if (specifics.has_managed_user_shared_setting())
    return DEPRECATED_SUPERVISED_USER_SHARED_SETTINGS;
  if (specifics.has_article())
    return ARTICLES;
  if (specifics.has_app_list())
    return APP_LIST;
  if (specifics.has_wifi_credential())
    return WIFI_CREDENTIALS;
  if (specifics.has_managed_user_whitelist())
    return SUPERVISED_USER_WHITELISTS;
  if (specifics.has_arc_package())
    return ARC_PACKAGE;
  if (specifics.has_printer())
    return PRINTERS;
  if (specifics.has_reading_list())
    return READING_LIST;
  if (specifics.has_user_event())
    return USER_EVENTS;
  if (specifics.has_mountain_share())
    return MOUNTAIN_SHARES;
  if (specifics.has_nigori())
    return NIGORI;
  if (specifics.has_experiments())
    return EXPERIMENTS;

  return UNSPECIFIED;
}

ModelTypeNameMap GetUserSelectableTypeNameMap() {
  ModelTypeNameMap type_names;
  ModelTypeSet type_set = UserSelectableTypes();
  ModelTypeSet::Iterator it = type_set.First();
  DCHECK_EQ(arraysize(kUserSelectableDataTypeNames), type_set.Size());
  for (size_t i = 0; i < arraysize(kUserSelectableDataTypeNames) && it.Good();
       ++i, it.Inc()) {
    type_names[it.Get()] = kUserSelectableDataTypeNames[i];
  }
  return type_names;
}

ModelTypeSet EncryptableUserTypes() {
  static_assert(41 == MODEL_TYPE_COUNT,
                "If adding an unencryptable type, remove from "
                "encryptable_user_types below.");
  ModelTypeSet encryptable_user_types = UserTypes();
  // Wallet data is not encrypted since it actually originates on the server.
  encryptable_user_types.Remove(AUTOFILL_WALLET_DATA);
  // We never encrypt history delete directives.
  encryptable_user_types.Remove(HISTORY_DELETE_DIRECTIVES);
  // Synced notifications are not encrypted since the server must see changes.
  encryptable_user_types.Remove(SYNCED_NOTIFICATIONS);
  // Synced Notification App Info does not have private data, so it is not
  // encrypted.
  encryptable_user_types.Remove(SYNCED_NOTIFICATION_APP_INFO);
  // Device info data is not encrypted because it might be synced before
  // encryption is ready.
  encryptable_user_types.Remove(DEVICE_INFO);
  // Priority preferences are not encrypted because they might be synced before
  // encryption is ready.
  encryptable_user_types.Remove(PRIORITY_PREFERENCES);
  // Supervised user settings are not encrypted since they are set server-side.
  encryptable_user_types.Remove(SUPERVISED_USER_SETTINGS);
  // Supervised users are not encrypted since they are managed server-side.
  encryptable_user_types.Remove(DEPRECATED_SUPERVISED_USERS);
  // Supervised user shared settings are not encrypted since they are managed
  // server-side and shared between manager and supervised user.
  encryptable_user_types.Remove(DEPRECATED_SUPERVISED_USER_SHARED_SETTINGS);
  // Supervised user whitelists are not encrypted since they are managed
  // server-side.
  encryptable_user_types.Remove(SUPERVISED_USER_WHITELISTS);
  // User events are not encrypted since they are consumed server-side.
  encryptable_user_types.Remove(USER_EVENTS);
  // Proxy types have no sync representation and are therefore not encrypted.
  // Note however that proxy types map to one or more protocol types, which
  // may or may not be encrypted themselves.
  encryptable_user_types.RemoveAll(ProxyTypes());
  return encryptable_user_types;
}

const char* ModelTypeToString(ModelType model_type) {
  // This is used in serialization routines as well as for displaying debug
  // information.  Do not attempt to change these string values unless you know
  // what you're doing.
  if (model_type >= UNSPECIFIED && model_type < MODEL_TYPE_COUNT)
    return kModelTypeInfoMap[model_type].model_type_string;
  NOTREACHED() << "No known extension for model type.";
  return "Invalid";
}

// The normal rules about histograms apply here.  Always append to the bottom of
// the list, and be careful to not reuse integer values that have already been
// assigned.
//
// Don't forget to update the "SyncModelTypes" enum in enums.xml when you make
// changes to this list.
int ModelTypeToHistogramInt(ModelType model_type) {
  if (model_type >= UNSPECIFIED && model_type < MODEL_TYPE_COUNT)
    return kModelTypeInfoMap[model_type].model_type_histogram_val;
  return 0;
}

std::unique_ptr<base::Value> ModelTypeToValue(ModelType model_type) {
  if (model_type >= FIRST_REAL_MODEL_TYPE) {
    return std::make_unique<base::Value>(ModelTypeToString(model_type));
  } else if (model_type == TOP_LEVEL_FOLDER) {
    return std::make_unique<base::Value>("Top-level folder");
  } else if (model_type == UNSPECIFIED) {
    return std::make_unique<base::Value>("Unspecified");
  }
  NOTREACHED();
  return std::make_unique<base::Value>(std::string());
}

ModelType ModelTypeFromString(const std::string& model_type_string) {
  if (model_type_string != "Unspecified" &&
      model_type_string != "Top Level Folder") {
    for (size_t i = 0; i < arraysize(kModelTypeInfoMap); ++i) {
      if (kModelTypeInfoMap[i].model_type_string == model_type_string)
        return kModelTypeInfoMap[i].model_type;
    }
  }
  NOTREACHED() << "No known model type corresponding to " << model_type_string
               << ".";
  return UNSPECIFIED;
}

std::string ModelTypeSetToString(ModelTypeSet model_types) {
  std::string result;
  for (ModelTypeSet::Iterator it = model_types.First(); it.Good(); it.Inc()) {
    if (!result.empty()) {
      result += ", ";
    }
    result += ModelTypeToString(it.Get());
  }
  return result;
}

std::ostream& operator<<(std::ostream& out, ModelTypeSet model_type_set) {
  return out << ModelTypeSetToString(model_type_set);
}

ModelTypeSet ModelTypeSetFromString(const std::string& model_types_string) {
  std::string working_copy = model_types_string;
  ModelTypeSet model_types;
  while (!working_copy.empty()) {
    // Remove any leading spaces.
    working_copy = working_copy.substr(working_copy.find_first_not_of(' '));
    if (working_copy.empty())
      break;
    std::string type_str;
    size_t end = working_copy.find(',');
    if (end == std::string::npos) {
      end = working_copy.length() - 1;
      type_str = working_copy;
    } else {
      type_str = working_copy.substr(0, end);
    }
    ModelType type = ModelTypeFromString(type_str);
    if (IsRealDataType(type))
      model_types.Put(type);
    working_copy = working_copy.substr(end + 1);
  }
  return model_types;
}

std::unique_ptr<base::ListValue> ModelTypeSetToValue(ModelTypeSet model_types) {
  std::unique_ptr<base::ListValue> value(new base::ListValue());
  for (ModelTypeSet::Iterator it = model_types.First(); it.Good(); it.Inc()) {
    value->AppendString(ModelTypeToString(it.Get()));
  }
  return value;
}

// TODO(zea): remove all hardcoded tags in model associators and have them use
// this instead.
// NOTE: Proxy types should return empty strings (so that we don't NOTREACHED
// in tests when we verify they have no root node).
std::string ModelTypeToRootTag(ModelType type) {
  if (IsProxyType(type))
    return std::string();
  if (IsRealDataType(type))
    return "google_chrome_" + std::string(kModelTypeInfoMap[type].root_tag);
  NOTREACHED() << "No known extension for model type.";
  return "INVALID";
}

const char* GetModelTypeRootTag(ModelType model_type) {
  return kModelTypeInfoMap[model_type].root_tag;
}

bool RealModelTypeToNotificationType(ModelType model_type,
                                     std::string* notification_type) {
  if (ProtocolTypes().Has(model_type)) {
    *notification_type = kModelTypeInfoMap[model_type].notification_type;
    return true;
  }
  notification_type->clear();
  return false;
}

bool NotificationTypeToRealModelType(const std::string& notification_type,
                                     ModelType* model_type) {
  if (notification_type.empty()) {
    *model_type = UNSPECIFIED;
    return false;
  }
  for (size_t i = 0; i < arraysize(kModelTypeInfoMap); ++i) {
    if (kModelTypeInfoMap[i].notification_type == notification_type) {
      *model_type = kModelTypeInfoMap[i].model_type;
      return true;
    }
  }
  *model_type = UNSPECIFIED;
  return false;
}

bool IsRealDataType(ModelType model_type) {
  return model_type >= FIRST_REAL_MODEL_TYPE && model_type < MODEL_TYPE_COUNT;
}

bool IsProxyType(ModelType model_type) {
  return model_type >= FIRST_PROXY_TYPE && model_type <= LAST_PROXY_TYPE;
}

bool IsActOnceDataType(ModelType model_type) {
  return model_type == HISTORY_DELETE_DIRECTIVES;
}

bool IsTypeWithServerGeneratedRoot(ModelType model_type) {
  return model_type == BOOKMARKS || model_type == NIGORI;
}

bool IsTypeWithClientGeneratedRoot(ModelType model_type) {
  return IsRealDataType(model_type) &&
         !IsTypeWithServerGeneratedRoot(model_type);
}

bool TypeSupportsHierarchy(ModelType model_type) {
  // TODO(stanisc): crbug/438313: Should this also include TOP_LEVEL_FOLDER?
  return model_type == BOOKMARKS;
}

bool TypeSupportsOrdering(ModelType model_type) {
  return model_type == BOOKMARKS;
}

}  // namespace syncer
