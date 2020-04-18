/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 * Copyright 2012, Google Inc.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_DECODE_H
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_DECODE_H

#include "native_client/src/trusted/validator_mips/model.h"
#include "native_client/src/trusted/validator_mips/inst_classes.h"

namespace nacl_mips_dec {

struct DecoderState;

/*
 * Creates a new DecodeState instance that can be used to make calls to
 * decode.  The caller owns the result and should delete it when appropriate,
 * by using delete_state below.
 */
const DecoderState *init_decode();

/*
 * Frees any resources previously allocated by a call to init_decode.
 */
void delete_state(const DecoderState *);

/*
 * Chooses a ClassDecoder that can answer questions about the given Instruction.
 */
const ClassDecoder &decode(const Instruction, const DecoderState *);


}  // namespace

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_DECODE_H
