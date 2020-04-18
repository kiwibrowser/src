/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_V2_DECODE_H
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_V2_DECODE_H

#include "native_client/src/trusted/validator_arm/model.h"
#include "native_client/src/trusted/validator_arm/inst_classes.h"

namespace nacl_arm_dec {

// Models a arm instruction parser that returns the decoder to use
// to decode an instruction.
struct DecoderState {
  explicit DecoderState() {}
  virtual ~DecoderState() {}

  // Parses the given instruction, returning the decoder to use.
  virtual const class ClassDecoder &decode(const Instruction) const = 0;
};

}  // namespace

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_V2_DECODE_H
