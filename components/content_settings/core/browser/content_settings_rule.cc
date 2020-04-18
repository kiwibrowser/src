// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/content_settings/core/browser/content_settings_rule.h"

#include <utility>

#include "base/logging.h"

namespace content_settings {

Rule::Rule() {}

Rule::Rule(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    base::Value* value)
    : primary_pattern(primary_pattern),
      secondary_pattern(secondary_pattern),
      value(value) {
  DCHECK(value);
}

Rule::Rule(const Rule& other) = default;

Rule::~Rule() {}

RuleIterator::~RuleIterator() {}

ConcatenationIterator::ConcatenationIterator(
    std::vector<std::unique_ptr<RuleIterator>> iterators,
    base::AutoLock* auto_lock)
    : iterators_(std::move(iterators)), auto_lock_(auto_lock) {
  auto it = iterators_.begin();
  while (it != iterators_.end()) {
    if (!(*it)->HasNext())
      it = iterators_.erase(it);
    else
      ++it;
  }
}

ConcatenationIterator::~ConcatenationIterator() {}

bool ConcatenationIterator::HasNext() const {
  return !iterators_.empty();
}

Rule ConcatenationIterator::Next() {
  auto current_iterator = iterators_.begin();
  DCHECK(current_iterator != iterators_.end());
  DCHECK((*current_iterator)->HasNext());
  const Rule& to_return = (*current_iterator)->Next();
  if (!(*current_iterator)->HasNext())
    iterators_.erase(current_iterator);
  return to_return;
}

}  // namespace content_settings
