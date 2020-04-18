// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/reports.h"

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/string_piece.h"
#include "components/rappor/byte_vector_utils.h"
#include "components/rappor/public/rappor_parameters.h"

namespace rappor {

namespace internal {

ByteVector GenerateReport(const std::string& secret,
                          const NoiseParameters& parameters,
                          const ByteVector& value) {
  // Generate a deterministically random mask of fake data using the
  // client's secret key + real data as a seed.  The inclusion of the secret
  // in the seed avoids correlations between real and fake data.
  // The seed isn't a human-readable string.
  const base::StringPiece personalization_string(
      reinterpret_cast<const char*>(&value[0]), value.size());
  HmacByteVectorGenerator hmac_generator(value.size(), secret,
                                         personalization_string);
  const ByteVector fake_mask =
      hmac_generator.GetWeightedRandomByteVector(parameters.fake_prob);
  ByteVector fake_bits =
      hmac_generator.GetWeightedRandomByteVector(parameters.fake_one_prob);

  // Redact most of the real data by replacing it with the fake data, hiding
  // and limiting the amount of information an individual client reports on.
  const ByteVector* fake_and_redacted_bits =
      ByteVectorMerge(fake_mask, value, &fake_bits);

  // Generate biased coin flips for each bit.
  ByteVectorGenerator coin_generator(value.size());
  const ByteVector zero_coins =
      coin_generator.GetWeightedRandomByteVector(parameters.zero_coin_prob);
  ByteVector one_coins =
      coin_generator.GetWeightedRandomByteVector(parameters.one_coin_prob);

  // Create a randomized response report on the fake and redacted data, sending
  // the outcome of flipping a zero coin for the zero bits in that data, and of
  // flipping a one coin for the one bits in that data, as the final report.
  return *ByteVectorMerge(*fake_and_redacted_bits, zero_coins, &one_coins);
}

}  // namespace internal

}  // namespace rappor
