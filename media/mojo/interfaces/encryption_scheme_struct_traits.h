// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_INTERFACES_ENCRYPTION_SCHEME_STRUCT_TRAITS_H_
#define MEDIA_MOJO_INTERFACES_ENCRYPTION_SCHEME_STRUCT_TRAITS_H_

#include "media/base/encryption_pattern.h"
#include "media/base/encryption_scheme.h"
#include "media/base/ipc/media_param_traits.h"
#include "media/mojo/interfaces/media_types.mojom.h"

namespace mojo {

template <>
struct StructTraits<media::mojom::EncryptionPatternDataView,
                    media::EncryptionPattern> {
  static uint32_t crypt_byte_block(const media::EncryptionPattern& input) {
    return input.crypt_byte_block();
  }

  static uint32_t skip_byte_block(const media::EncryptionPattern& input) {
    return input.skip_byte_block();
  }

  static bool Read(media::mojom::EncryptionPatternDataView input,
                   media::EncryptionPattern* output);
};

template <>
struct StructTraits<media::mojom::EncryptionSchemeDataView,
                    media::EncryptionScheme> {
  static media::EncryptionScheme::CipherMode mode(
      const media::EncryptionScheme& input) {
    return input.mode();
  }

  static media::EncryptionPattern pattern(
      const media::EncryptionScheme& input) {
    return input.pattern();
  }

  static bool Read(media::mojom::EncryptionSchemeDataView input,
                   media::EncryptionScheme* output);
};

}  // namespace mojo

#endif  // MEDIA_MOJO_INTERFACES_ENCRYPTION_SCHEME_STRUCT_TRAITS_H_
