// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_COMMON_MEDIA_TYPE_CONVERTERS_H_
#define MEDIA_MOJO_COMMON_MEDIA_TYPE_CONVERTERS_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "media/mojo/interfaces/content_decryption_module.mojom.h"
#include "media/mojo/interfaces/media_types.mojom.h"
#include "mojo/public/cpp/bindings/type_converter.h"

namespace media {
class AudioBuffer;
class DecoderBuffer;
class DecryptConfig;
struct CdmKeyInformation;
}

// These are specializations of mojo::TypeConverter and have to be in the mojo
// namespace.
namespace mojo {

template <>
struct TypeConverter<media::mojom::DecryptConfigPtr, media::DecryptConfig> {
  static media::mojom::DecryptConfigPtr Convert(
      const media::DecryptConfig& input);
};
template <>
struct TypeConverter<std::unique_ptr<media::DecryptConfig>,
                     media::mojom::DecryptConfigPtr> {
  static std::unique_ptr<media::DecryptConfig> Convert(
      const media::mojom::DecryptConfigPtr& input);
};

template <>
struct TypeConverter<media::mojom::DecoderBufferPtr, media::DecoderBuffer> {
  static media::mojom::DecoderBufferPtr Convert(
      const media::DecoderBuffer& input);
};
template <>
struct TypeConverter<scoped_refptr<media::DecoderBuffer>,
                     media::mojom::DecoderBufferPtr> {
  static scoped_refptr<media::DecoderBuffer> Convert(
      const media::mojom::DecoderBufferPtr& input);
};

template <>
struct TypeConverter<media::mojom::CdmKeyInformationPtr,
                     media::CdmKeyInformation> {
  static media::mojom::CdmKeyInformationPtr Convert(
      const media::CdmKeyInformation& input);
};
template <>
struct TypeConverter<std::unique_ptr<media::CdmKeyInformation>,
                     media::mojom::CdmKeyInformationPtr> {
  static std::unique_ptr<media::CdmKeyInformation> Convert(
      const media::mojom::CdmKeyInformationPtr& input);
};

template <>
struct TypeConverter<media::mojom::AudioBufferPtr,
                     scoped_refptr<media::AudioBuffer>> {
  static media::mojom::AudioBufferPtr Convert(
      const scoped_refptr<media::AudioBuffer>& input);
};
template <>
struct TypeConverter<scoped_refptr<media::AudioBuffer>,
                     media::mojom::AudioBufferPtr> {
  static scoped_refptr<media::AudioBuffer> Convert(
      const media::mojom::AudioBufferPtr& input);
};

}  // namespace mojo

#endif  // MEDIA_MOJO_COMMON_MEDIA_TYPE_CONVERTERS_H_
