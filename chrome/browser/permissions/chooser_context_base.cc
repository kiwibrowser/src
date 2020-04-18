// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/permissions/chooser_context_base.h"

#include <utility>

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

const char kObjectListKey[] = "chosen-objects";

ChooserContextBase::ChooserContextBase(
    Profile* profile,
    const ContentSettingsType guard_content_settings_type,
    const ContentSettingsType data_content_settings_type)
    : host_content_settings_map_(
          HostContentSettingsMapFactory::GetForProfile(profile)),
      guard_content_settings_type_(guard_content_settings_type),
      data_content_settings_type_(data_content_settings_type) {
  DCHECK(host_content_settings_map_);
}

ChooserContextBase::~ChooserContextBase() = default;

ChooserContextBase::Object::Object(GURL requesting_origin,
                                   GURL embedding_origin,
                                   base::DictionaryValue* object,
                                   const std::string& source,
                                   bool incognito)
    : requesting_origin(requesting_origin),
      embedding_origin(embedding_origin),
      source(source),
      incognito(incognito) {
  this->object.Swap(object);
}

ChooserContextBase::Object::~Object() = default;

bool ChooserContextBase::CanRequestObjectPermission(
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  ContentSetting content_setting =
      host_content_settings_map_->GetContentSetting(
          requesting_origin, embedding_origin, guard_content_settings_type_,
          std::string());
  DCHECK(content_setting == CONTENT_SETTING_ASK ||
         content_setting == CONTENT_SETTING_BLOCK);
  return content_setting == CONTENT_SETTING_ASK;
}

std::vector<std::unique_ptr<base::DictionaryValue>>
ChooserContextBase::GetGrantedObjects(const GURL& requesting_origin,
                                      const GURL& embedding_origin) {
  DCHECK_EQ(requesting_origin, requesting_origin.GetOrigin());
  DCHECK_EQ(embedding_origin, embedding_origin.GetOrigin());

  if (!CanRequestObjectPermission(requesting_origin, embedding_origin))
    return {};

  std::vector<std::unique_ptr<base::DictionaryValue>> results;
  std::unique_ptr<base::DictionaryValue> setting =
      GetWebsiteSetting(requesting_origin, embedding_origin);
  std::unique_ptr<base::Value> objects;
  if (!setting->Remove(kObjectListKey, &objects))
    return results;

  std::unique_ptr<base::ListValue> object_list =
      base::ListValue::From(std::move(objects));
  if (!object_list)
    return results;

  for (auto& object : *object_list) {
    // Steal ownership of |object| from |object_list|.
    std::unique_ptr<base::DictionaryValue> object_dict =
        base::DictionaryValue::From(
            base::Value::ToUniquePtrValue(std::move(object)));
    if (object_dict && IsValidObject(*object_dict))
      results.push_back(std::move(object_dict));
  }
  return results;
}

std::vector<std::unique_ptr<ChooserContextBase::Object>>
ChooserContextBase::GetAllGrantedObjects() {
  ContentSettingsForOneType content_settings;
  host_content_settings_map_->GetSettingsForOneType(
      data_content_settings_type_, std::string(), &content_settings);

  std::vector<std::unique_ptr<Object>> results;
  for (const ContentSettingPatternSource& content_setting : content_settings) {
    GURL requesting_origin(content_setting.primary_pattern.ToString());
    GURL embedding_origin(content_setting.secondary_pattern.ToString());
    if (!requesting_origin.is_valid() || !embedding_origin.is_valid())
      continue;

    if (!CanRequestObjectPermission(requesting_origin, embedding_origin))
      continue;

    std::unique_ptr<base::DictionaryValue> setting =
        GetWebsiteSetting(requesting_origin, embedding_origin);
    base::ListValue* object_list;
    if (!setting->GetList(kObjectListKey, &object_list))
      continue;

    for (auto& object : *object_list) {
      base::DictionaryValue* object_dict;
      if (!object.GetAsDictionary(&object_dict) ||
          !IsValidObject(*object_dict)) {
        continue;
      }

      results.push_back(std::make_unique<Object>(
          requesting_origin, embedding_origin, object_dict,
          content_setting.source, content_setting.incognito));
    }
  }

  return results;
}

void ChooserContextBase::GrantObjectPermission(
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    std::unique_ptr<base::DictionaryValue> object) {
  DCHECK_EQ(requesting_origin, requesting_origin.GetOrigin());
  DCHECK_EQ(embedding_origin, embedding_origin.GetOrigin());
  DCHECK(object);
  DCHECK(IsValidObject(*object));

  std::unique_ptr<base::DictionaryValue> setting =
      GetWebsiteSetting(requesting_origin, embedding_origin);
  base::ListValue* object_list;
  if (!setting->GetList(kObjectListKey, &object_list)) {
    object_list =
        setting->SetList(kObjectListKey, std::make_unique<base::ListValue>());
  }
  object_list->AppendIfNotPresent(std::move(object));
  SetWebsiteSetting(requesting_origin, embedding_origin, std::move(setting));
}

void ChooserContextBase::RevokeObjectPermission(
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    const base::DictionaryValue& object) {
  DCHECK_EQ(requesting_origin, requesting_origin.GetOrigin());
  DCHECK_EQ(embedding_origin, embedding_origin.GetOrigin());
  DCHECK(IsValidObject(object));

  std::unique_ptr<base::DictionaryValue> setting =
      GetWebsiteSetting(requesting_origin, embedding_origin);
  base::ListValue* object_list;
  if (!setting->GetList(kObjectListKey, &object_list))
    return;
  object_list->Remove(object, nullptr);
  SetWebsiteSetting(requesting_origin, embedding_origin, std::move(setting));
}

std::unique_ptr<base::DictionaryValue> ChooserContextBase::GetWebsiteSetting(
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(host_content_settings_map_->GetWebsiteSetting(
          requesting_origin, embedding_origin, data_content_settings_type_,
          std::string(), nullptr));
  if (!value)
    value.reset(new base::DictionaryValue());

  return value;
}

void ChooserContextBase::SetWebsiteSetting(const GURL& requesting_origin,
                                           const GURL& embedding_origin,
                                           std::unique_ptr<base::Value> value) {
  host_content_settings_map_->SetWebsiteSettingDefaultScope(
      requesting_origin, embedding_origin, data_content_settings_type_,
      std::string(), std::move(value));
}
