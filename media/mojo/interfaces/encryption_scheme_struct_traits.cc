// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/interfaces/encryption_scheme_struct_traits.h"

namespace mojo {

// static
bool StructTraits<media::mojom::EncryptionPatternDataView,
                  media::EncryptionPattern>::
    Read(media::mojom::EncryptionPatternDataView input,
         media::EncryptionPattern* output) {
  *output = media::EncryptionPattern(input.crypt_byte_block(),
                                     input.skip_byte_block());
  return true;
}

// static
bool StructTraits<
    media::mojom::EncryptionSchemeDataView,
    media::EncryptionScheme>::Read(media::mojom::EncryptionSchemeDataView input,
                                   media::EncryptionScheme* output) {
  media::EncryptionScheme::CipherMode mode;
  if (!input.ReadMode(&mode))
    return false;

  media::EncryptionPattern pattern;
  if (!input.ReadPattern(&pattern))
    return false;

  *output = media::EncryptionScheme(mode, pattern);

  return true;
}

}  // namespace mojo