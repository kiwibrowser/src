// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Interface for objects providing content setting rules.

#ifndef COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_RULE_H_
#define COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_RULE_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/linked_ptr.h"
#include "base/synchronization/lock.h"
#include "base/values.h"
#include "components/content_settings/core/common/content_settings_pattern.h"

namespace content_settings {

struct Rule {
  Rule();
  // Rule takes ownership of |value|.
  Rule(const ContentSettingsPattern& primary_pattern,
       const ContentSettingsPattern& secondary_pattern,
       base::Value* value);
  Rule(const Rule& other);
  ~Rule();

  ContentSettingsPattern primary_pattern;
  ContentSettingsPattern secondary_pattern;
  linked_ptr<base::Value> value;
};

class RuleIterator {
 public:
  virtual ~RuleIterator();
  virtual bool HasNext() const = 0;
  virtual Rule Next() = 0;
};

class ConcatenationIterator : public RuleIterator {
 public:
  // ConcatenationIterator takes ownership of the pointers in the |iterators|
  // list and |auto_lock|. |auto_lock| can be NULL if no locking is needed.
  ConcatenationIterator(std::vector<std::unique_ptr<RuleIterator>> iterators,
                        base::AutoLock* auto_lock);
  ~ConcatenationIterator() override;
  bool HasNext() const override;
  Rule Next() override;

 private:
  std::vector<std::unique_ptr<RuleIterator>> iterators_;
  std::unique_ptr<base::AutoLock> auto_lock_;
};

}  // namespace content_settings

#endif  // COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_RULE_H_
