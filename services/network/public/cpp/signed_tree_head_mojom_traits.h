// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_SIGNED_TREE_HEAD_MOJOM_TRAITS_H_
#define SERVICES_NETWORK_PUBLIC_CPP_SIGNED_TREE_HEAD_MOJOM_TRAITS_H_

#include "base/containers/span.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "net/cert/signed_tree_head.h"
#include "services/network/public/mojom/digitally_signed.mojom.h"
#include "services/network/public/mojom/signed_tree_head.mojom.h"

namespace mojo {

template <>
struct EnumTraits<network::mojom::SignedTreeHeadVersion,
                  net::ct::SignedTreeHead::Version> {
  static network::mojom::SignedTreeHeadVersion ToMojom(
      net::ct::SignedTreeHead::Version input) {
    switch (input) {
      case net::ct::SignedTreeHead::V1:
        return network::mojom::SignedTreeHeadVersion::V1;
    }
    NOTREACHED();
    return network::mojom::SignedTreeHeadVersion::kMaxValue;
  }

  static bool FromMojom(network::mojom::SignedTreeHeadVersion input,
                        net::ct::SignedTreeHead::Version* output) {
    switch (input) {
      case network::mojom::SignedTreeHeadVersion::V1:
        *output = net::ct::SignedTreeHead::V1;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct StructTraits<network::mojom::SignedTreeHeadDataView,
                    net::ct::SignedTreeHead> {
  static net::ct::SignedTreeHead::Version version(
      const net::ct::SignedTreeHead& sth) {
    return sth.version;
  }
  static base::Time timestamp(const net::ct::SignedTreeHead& sth) {
    return sth.timestamp;
  }
  static uint64_t tree_size(const net::ct::SignedTreeHead& sth) {
    return sth.tree_size;
  }
  static base::span<const uint8_t> sha256_root_hash(
      const net::ct::SignedTreeHead& sth) {
    return base::as_bytes(base::make_span(sth.sha256_root_hash));
  }
  static const net::ct::DigitallySigned& signature(
      const net::ct::SignedTreeHead& sth) {
    return sth.signature;
  }
  static base::StringPiece log_id(const net::ct::SignedTreeHead& sth) {
    return sth.log_id;
  }

  static bool Read(network::mojom::SignedTreeHeadDataView obj,
                   net::ct::SignedTreeHead* out);
};

}  // namespace mojo

#endif  // SERVICES_NETWORK_PUBLIC_CPP_SIGNED_TREE_HEAD_MOJOM_TRAITS_H_
