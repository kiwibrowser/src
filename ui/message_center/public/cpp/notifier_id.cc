// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/public/cpp/notifier_id.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"

namespace message_center {

NotifierId::NotifierId() : type(SYSTEM_COMPONENT) {}

NotifierId::NotifierId(NotifierType type, const std::string& id)
    : type(type), id(id) {
  DCHECK(type != WEB_PAGE);
  DCHECK(!id.empty());
}

NotifierId::NotifierId(const GURL& url) : type(WEB_PAGE), url(url) {}

NotifierId::NotifierId(const NotifierId& other) = default;

bool NotifierId::operator==(const NotifierId& other) const {
  if (type != other.type)
    return false;

  if (profile_id != other.profile_id)
    return false;

  if (type == WEB_PAGE)
    return url == other.url;

  return id == other.id;
}

bool NotifierId::operator<(const NotifierId& other) const {
  if (type != other.type)
    return type < other.type;

  if (profile_id != other.profile_id)
    return profile_id < other.profile_id;

  if (type == WEB_PAGE)
    return url < other.url;

  return id < other.id;
}

}  // namespace message_center
