// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_ANDROID_CERT_CERT_VERIFIER_CACHE_SERIALIZER_H_
#define COMPONENTS_CRONET_ANDROID_CERT_CERT_VERIFIER_CACHE_SERIALIZER_H_

namespace cronet_pb {
class CertVerificationCache;
}  // namespace cronet_pb

namespace net {
class CachingCertVerifier;
}  // namespace net

namespace cronet {

// Iterates through |verifier|'s cache and returns serialized data. This can be
// used to populate a new net::CachingCertVerifier with
// |DeserializeCertVerifierCache()|.
cronet_pb::CertVerificationCache SerializeCertVerifierCache(
    const net::CachingCertVerifier& verifier);

// Populates |verifier|'s cache. Returns true if the |cert_cache| is
// deserialized correctly.
bool DeserializeCertVerifierCache(
    const cronet_pb::CertVerificationCache& cert_cache,
    net::CachingCertVerifier* verifier);

}  // namespace cronet

#endif  // COMPONENTS_CRONET_ANDROID_CERT_CERT_VERIFIER_CACHE_SERIALIZER_H_
