// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PERMISSIONS_CHOOSER_CONTEXT_BASE_H_
#define CHROME_BROWSER_PERMISSIONS_CHOOSER_CONTEXT_BASE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class HostContentSettingsMap;
class Profile;

// This is the base class for services that manage any type of permission that
// is granted through a chooser-style UI instead of a simple allow/deny prompt.
// Subclasses must define the structure of the objects that are stored.
class ChooserContextBase : public KeyedService {
 public:
  struct Object {
    // The contents of |object| are Swap()ed into the internal dictionary.
    Object(GURL requesting_origin,
           GURL embedding_origin,
           base::DictionaryValue* object,
           const std::string& source,
           bool incognito);
    ~Object();

    GURL requesting_origin;
    GURL embedding_origin;
    base::DictionaryValue object;
    std::string source;
    bool incognito;
  };

  ChooserContextBase(Profile* profile,
                     ContentSettingsType guard_content_settings_type,
                     ContentSettingsType data_content_settings_type);
  ~ChooserContextBase() override;

  // Checks whether |requesting_origin| can request permission to access objects
  // when embedded within |embedding_origin|. This is done by checking
  // |guard_content_settings_type_| which will usually be "ask" by default but
  // could be set by the user or group policy.
  bool CanRequestObjectPermission(const GURL& requesting_origin,
                                  const GURL& embedding_origin);

  // Returns the list of objects that |requesting_origin| has been granted
  // permission to access when embedded within |embedding_origin|.
  //
  // This method may be extended by a subclass to return objects not stored in
  // |host_content_settings_map_|.
  virtual std::vector<std::unique_ptr<base::DictionaryValue>> GetGrantedObjects(
      const GURL& requesting_origin,
      const GURL& embedding_origin);

  // Returns the set of all objects that any origin has been granted permission
  // to access.
  //
  // This method may be extended by a subclass to return objects not stored in
  // |host_content_settings_map_|.
  virtual std::vector<std::unique_ptr<Object>> GetAllGrantedObjects();

  // Grants |requesting_origin| access to |object| when embedded within
  // |embedding_origin| by writing it into |host_content_settings_map_|.
  void GrantObjectPermission(const GURL& requesting_origin,
                             const GURL& embedding_origin,
                             std::unique_ptr<base::DictionaryValue> object);

  // Revokes |requesting_origin|'s permission to access |object| when embedded
  // within |embedding_origin|.
  //
  // This method may be extended by a subclass to revoke permission to access
  // objects returned by GetPreviouslyChosenObjects but not stored in
  // |host_content_settings_map_|.
  virtual void RevokeObjectPermission(const GURL& requesting_origin,
                                      const GURL& embedding_origin,
                                      const base::DictionaryValue& object);

  // Validates the structure of an object read from
  // |host_content_settings_map_|.
  virtual bool IsValidObject(const base::DictionaryValue& object) = 0;

  // Returns the human readable string representing the given object.
  virtual std::string GetObjectName(const base::DictionaryValue& object) = 0;

 private:
  std::unique_ptr<base::DictionaryValue> GetWebsiteSetting(
      const GURL& requesting_origin,
      const GURL& embedding_origin);
  void SetWebsiteSetting(const GURL& requesting_origin,
                         const GURL& embedding_origin,
                         std::unique_ptr<base::Value> value);

  HostContentSettingsMap* const host_content_settings_map_;
  const ContentSettingsType guard_content_settings_type_;
  const ContentSettingsType data_content_settings_type_;
};

#endif  // CHROME_BROWSER_PERMISSIONS_CHOOSER_CONTEXT_BASE_H_
