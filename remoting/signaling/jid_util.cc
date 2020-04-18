// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/jid_util.h"

#include <stddef.h>

#include "base/strings/string_util.h"

namespace remoting {

std::string NormalizeJid(const std::string& jid) {
  std::string bare_jid;
  std::string resource;
  if (SplitJidResource(jid, &bare_jid, &resource)) {
    return base::ToLowerASCII(bare_jid) + "/" + resource;
  }
  return base::ToLowerASCII(bare_jid);
}

bool SplitJidResource(const std::string& full_jid,
                      std::string* bare_jid,
                      std::string* resource) {
  size_t slash_index = full_jid.find('/');
  if (slash_index == std::string::npos) {
    if (bare_jid) {
      *bare_jid = full_jid;
    }
    if (resource) {
      resource->clear();
    }
    return false;
  }

  if (bare_jid) {
    *bare_jid = full_jid.substr(0, slash_index);
  }
  if (resource) {
    *resource = full_jid.substr(slash_index + 1);
  }
  return true;
}

}  // namespace remoting
