// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_PUBLIC_MEDIA_CAST_DECRYPT_CONFIG_H_
#define CHROMECAST_PUBLIC_MEDIA_CAST_DECRYPT_CONFIG_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace chromecast {
namespace media {

// The Common Encryption spec provides for subsample encryption, where portions
// of a sample are set in cleartext. A SubsampleEntry specifies the number of
// clear and encrypted bytes in each subsample. For decryption, all of the
// encrypted bytes in a sample should be considered a single logical stream,
// regardless of how they are divided into subsamples, and the clear bytes
// should not be considered as part of decryption. This is logically equivalent
// to concatenating all 'cypher_bytes' portions of subsamples, decrypting that
// result, and then copying each byte from the decrypted block over the
// position of the corresponding encrypted byte.
struct SubsampleEntry {
  SubsampleEntry() : clear_bytes(0), cypher_bytes(0) {}
  SubsampleEntry(uint32_t clear_bytes, uint32_t cypher_bytes)
      : clear_bytes(clear_bytes), cypher_bytes(cypher_bytes) {}
  uint32_t clear_bytes;
  uint32_t cypher_bytes;
};

// Contains all metadata needed to decrypt a media sample.
class CastDecryptConfig {
 public:
  virtual ~CastDecryptConfig() {}

  // Returns the ID for this sample's decryption key.
  virtual const std::string& key_id() const = 0;

  // Returns the initialization vector as defined by the encryption format.
  virtual const std::string& iv() const = 0;

  // Returns the clear and encrypted portions of the sample as described above.
  virtual const std::vector<SubsampleEntry>& subsamples() const = 0;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_PUBLIC_MEDIA_CAST_DECRYPT_CONFIG_H_
