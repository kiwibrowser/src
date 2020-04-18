// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/signed_tree_head_mojom_traits.h"

#include <memory>
#include <vector>

#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "services/network/public/cpp/digitally_signed_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<
    network::mojom::SignedTreeHeadDataView,
    net::ct::SignedTreeHead>::Read(network::mojom::SignedTreeHeadDataView data,
                                   net::ct::SignedTreeHead* out) {
  std::vector<uint8_t> sha256_root_hash;
  if (!data.ReadVersion(&out->version) ||
      !data.ReadTimestamp(&out->timestamp) ||
      !data.ReadSignature(&out->signature) || !data.ReadLogId(&out->log_id) ||
      !data.ReadSha256RootHash(&sha256_root_hash)) {
    return false;
  }
  if (out->log_id.empty()) {
    return false;
  }

  out->tree_size = data.tree_size();

  // The Mojo bindings should have validated the size constraint as part of
  // ReadSha256RootHash().
  DCHECK_EQ(sha256_root_hash.size(), sizeof(out->sha256_root_hash));
  memcpy(out->sha256_root_hash, sha256_root_hash.data(),
         sizeof(out->sha256_root_hash));

  return true;
}

}  // namespace mojo
