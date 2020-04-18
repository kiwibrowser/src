// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <vector>

#include "base/logging.h"
#include "components/webcrypto/algorithm_implementation.h"
#include "components/webcrypto/algorithms/util.h"
#include "components/webcrypto/crypto_data.h"
#include "components/webcrypto/status.h"
#include "crypto/openssl_util.h"
#include "third_party/boringssl/src/include/openssl/digest.h"

namespace webcrypto {

namespace {

// Implementation of blink::WebCryptoDigester, an internal Blink detail not
// part of WebCrypto, that allows chunks of data to be streamed in before
// computing a SHA-* digest (as opposed to ShaImplementation, which computes
// digests over complete messages)
class DigestorImpl : public blink::WebCryptoDigestor {
 public:
  explicit DigestorImpl(blink::WebCryptoAlgorithmId algorithm_id)
      : initialized_(false),
        algorithm_id_(algorithm_id) {}

  bool Consume(const unsigned char* data, unsigned int size) override {
    return ConsumeWithStatus(data, size).IsSuccess();
  }

  Status ConsumeWithStatus(const unsigned char* data, unsigned int size) {
    crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);
    Status error = Init();
    if (!error.IsSuccess())
      return error;

    if (!EVP_DigestUpdate(digest_context_.get(), data, size))
      return Status::OperationError();

    return Status::Success();
  }

  bool Finish(unsigned char*& result_data,
              unsigned int& result_data_size) override {
    Status error = FinishInternal(result_, &result_data_size);
    if (!error.IsSuccess())
      return false;
    result_data = result_;
    return true;
  }

  Status FinishWithVectorAndStatus(std::vector<uint8_t>* result) {
    const size_t hash_expected_size = EVP_MD_CTX_size(digest_context_.get());
    result->resize(hash_expected_size);
    unsigned int hash_buffer_size;  // ignored
    return FinishInternal(result->data(), &hash_buffer_size);
  }

 private:
  Status Init() {
    if (initialized_)
      return Status::Success();

    const EVP_MD* digest_algorithm = GetDigest(algorithm_id_);
    if (!digest_algorithm)
      return Status::ErrorUnsupported();

    if (!EVP_DigestInit_ex(digest_context_.get(), digest_algorithm, nullptr))
      return Status::OperationError();

    initialized_ = true;
    return Status::Success();
  }

  Status FinishInternal(unsigned char* result, unsigned int* result_size) {
    crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);
    Status error = Init();
    if (!error.IsSuccess())
      return error;

    const size_t hash_expected_size = EVP_MD_CTX_size(digest_context_.get());
    if (hash_expected_size == 0)
      return Status::ErrorUnexpected();
    DCHECK_LE(hash_expected_size, static_cast<unsigned>(EVP_MAX_MD_SIZE));

    if (!EVP_DigestFinal_ex(digest_context_.get(), result, result_size) ||
        *result_size != hash_expected_size)
      return Status::OperationError();

    return Status::Success();
  }

  bool initialized_;
  bssl::ScopedEVP_MD_CTX digest_context_;
  blink::WebCryptoAlgorithmId algorithm_id_;
  unsigned char result_[EVP_MAX_MD_SIZE];
};

class ShaImplementation : public AlgorithmImplementation {
 public:
  Status Digest(const blink::WebCryptoAlgorithm& algorithm,
                const CryptoData& data,
                std::vector<uint8_t>* buffer) const override {
    DigestorImpl digestor(algorithm.Id());
    Status error = digestor.ConsumeWithStatus(data.bytes(), data.byte_length());
    // http://crbug.com/366427: the spec does not define any other failures for
    // digest, so none of the subsequent errors are spec compliant.
    if (!error.IsSuccess())
      return error;
    return digestor.FinishWithVectorAndStatus(buffer);
  }
};

}  // namespace

std::unique_ptr<AlgorithmImplementation> CreateShaImplementation() {
  return std::make_unique<ShaImplementation>();
}

std::unique_ptr<blink::WebCryptoDigestor> CreateDigestorImplementation(
    blink::WebCryptoAlgorithmId algorithm) {
  return std::unique_ptr<blink::WebCryptoDigestor>(new DigestorImpl(algorithm));
}

}  // namespace webcrypto
