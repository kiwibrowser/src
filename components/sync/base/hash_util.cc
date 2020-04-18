// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/hash_util.h"

#include "base/base64.h"
#include "base/sha1.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {

std::string GenerateSyncableHash(ModelType model_type,
                                 const std::string& client_tag) {
  // Blank PB with just the field in it has termination symbol,
  // handy for delimiter.
  sync_pb::EntitySpecifics serialized_type;
  AddDefaultFieldValue(model_type, &serialized_type);
  std::string hash_input;
  serialized_type.AppendToString(&hash_input);
  hash_input.append(client_tag);

  std::string encode_output;
  base::Base64Encode(base::SHA1HashString(hash_input), &encode_output);
  return encode_output;
}

std::string GenerateSyncableBookmarkHash(
    const std::string& originator_cache_guid,
    const std::string& originator_client_item_id) {
  return GenerateSyncableHash(
      BOOKMARKS, originator_cache_guid + originator_client_item_id);
}

}  // namespace syncer
