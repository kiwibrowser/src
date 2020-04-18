// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/public/mojo/notifier_id_struct_traits.h"
#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {

using NotifierIdStructTraits =
    StructTraits<message_center::mojom::NotifierIdDataView,
                 message_center::NotifierId>;

// static
const message_center::NotifierId::NotifierType& NotifierIdStructTraits::type(
    const message_center::NotifierId& n) {
  return n.type;
}

// static
const std::string& NotifierIdStructTraits::id(
    const message_center::NotifierId& n) {
  return n.id;
}

// static
const GURL& NotifierIdStructTraits::url(const message_center::NotifierId& n) {
  return n.url;
}

//  static
const std::string& NotifierIdStructTraits::profile_id(
    const message_center::NotifierId& n) {
  return n.profile_id;
}

// static
bool NotifierIdStructTraits::Read(
    message_center::mojom::NotifierIdDataView data,
    message_center::NotifierId* out) {
  return data.ReadType(&out->type) && data.ReadId(&out->id) &&
         data.ReadUrl(&out->url) && data.ReadProfileId(&out->profile_id);
}

}  // namespace mojo
