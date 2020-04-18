// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_ORIGIN_IDENTIFIER_VALUE_MAP_H_
#define COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_ORIGIN_IDENTIFIER_VALUE_MAP_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/time/time.h"
#include "components/content_settings/core/common/content_settings.h"

class GURL;

namespace base {
class Lock;
class Value;
}

namespace content_settings {

class RuleIterator;

class OriginIdentifierValueMap {
 public:
  struct EntryMapKey {
    ContentSettingsType content_type;
    ResourceIdentifier resource_identifier;
    EntryMapKey(ContentSettingsType content_type,
                const ResourceIdentifier& resource_identifier);
    bool operator<(const OriginIdentifierValueMap::EntryMapKey& other) const;
  };

  struct PatternPair {
    ContentSettingsPattern primary_pattern;
    ContentSettingsPattern secondary_pattern;
    PatternPair(const ContentSettingsPattern& primary_pattern,
                const ContentSettingsPattern& secondary_pattern);
    bool operator<(const OriginIdentifierValueMap::PatternPair& other) const;
  };

  struct ValueEntry {
    base::Time last_modified;
    linked_ptr<base::Value> value;
    ValueEntry();
    ~ValueEntry();
  };

  typedef std::map<PatternPair, ValueEntry> Rules;
  typedef std::map<EntryMapKey, Rules> EntryMap;

  EntryMap::iterator begin() {
    return entries_.begin();
  }

  EntryMap::iterator end() {
    return entries_.end();
  }

  EntryMap::const_iterator begin() const {
    return entries_.begin();
  }

  EntryMap::const_iterator end() const {
    return entries_.end();
  }

  bool empty() const {
    return size() == 0u;
  }

  size_t size() const;

  // Returns an iterator for reading the rules for |content_type| and
  // |resource_identifier|. It is not allowed to call functions of
  // |OriginIdentifierValueMap| (also |GetRuleIterator|) before the iterator
  // has been destroyed. If |lock| is non-NULL, the returned |RuleIterator|
  // locks it and releases it when it is destroyed.
  // Returns nullptr to indicate the RuleIterator is empty.
  std::unique_ptr<RuleIterator> GetRuleIterator(
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier,
      base::Lock* lock) const;

  OriginIdentifierValueMap();
  ~OriginIdentifierValueMap();

  // Returns a weak pointer to the value for the given |primary_pattern|,
  // |secondary_pattern|, |content_type|, |resource_identifier| tuple. If
  // no value is stored for the passed parameter |NULL| is returned.
  base::Value* GetValue(
      const GURL& primary_url,
      const GURL& secondary_url,
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier) const;

  base::Time GetLastModified(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier) const;

  // Sets the |value| for the given |primary_pattern|, |secondary_pattern|,
  // |content_type|, |resource_identifier| tuple. The method takes the ownership
  // of the passed |value|. The caller can also store a |last_modified| date
  // for each value.
  void SetValue(const ContentSettingsPattern& primary_pattern,
                const ContentSettingsPattern& secondary_pattern,
                ContentSettingsType content_type,
                const ResourceIdentifier& resource_identifier,
                base::Time last_modified,
                base::Value* value);

  // Deletes the map entry for the given |primary_pattern|,
  // |secondary_pattern|, |content_type|, |resource_identifier| tuple.
  void DeleteValue(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier);

  // Deletes all map entries for the given |content_type| and
  // |resource_identifier|.
  void DeleteValues(
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier);

  // Clears all map entries.
  void clear();

 private:
  EntryMap entries_;

  DISALLOW_COPY_AND_ASSIGN(OriginIdentifierValueMap);
};

}  // namespace content_settings

#endif  // COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_ORIGIN_IDENTIFIER_VALUE_MAP_H_
