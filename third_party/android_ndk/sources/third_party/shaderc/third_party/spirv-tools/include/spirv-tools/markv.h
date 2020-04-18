// Copyright (c) 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// MARK-V is a compression format for SPIR-V binaries. It strips away
// non-essential information (such as result ids which can be regenerated) and
// uses various bit reduction techiniques to reduce the size of the binary.
//
// WIP: MARK-V codec is in early stages of development. At the moment it only
// can encode and decode some SPIR-V files and only if exacly the same build of
// software is used (is doesn't write or handle version numbers yet).

#ifndef SPIRV_TOOLS_MARKV_H_
#define SPIRV_TOOLS_MARKV_H_

#include "libspirv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct spv_markv_binary_t {
  uint8_t* data;
  size_t length;
} spv_markv_binary_t;

typedef spv_markv_binary_t* spv_markv_binary;
typedef const spv_markv_binary_t* const_spv_markv_binary;

typedef struct spv_markv_encoder_options_t spv_markv_encoder_options_t;
typedef spv_markv_encoder_options_t* spv_markv_encoder_options;
typedef const spv_markv_encoder_options_t* spv_const_markv_encoder_options;

typedef struct spv_markv_decoder_options_t spv_markv_decoder_options_t;
typedef spv_markv_decoder_options_t* spv_markv_decoder_options;
typedef const spv_markv_decoder_options_t* spv_const_markv_decoder_options;

// Creates spv_markv_encoder_options with default options. Returns a valid
// options object. The object remains valid until it is passed into
// spvMarkvEncoderOptionsDestroy.
spv_markv_encoder_options spvMarkvEncoderOptionsCreate();

// Destroys the given spv_markv_encoder_options object.
void spvMarkvEncoderOptionsDestroy(spv_markv_encoder_options options);

// Creates spv_markv_decoder_options with default options. Returns a valid
// options object. The object remains valid until it is passed into
// spvMarkvDecoderOptionsDestroy.
spv_markv_decoder_options spvMarkvDecoderOptionsCreate();

// Destroys the given spv_markv_decoder_options object.
void spvMarkvDecoderOptionsDestroy(spv_markv_decoder_options options);

// Encodes the given SPIR-V binary to MARK-V binary.
// If |comments| is not nullptr, it would contain a textual description of
// how encoding was done (with snippets of disassembly and bit sequences).
spv_result_t spvSpirvToMarkv(spv_const_context context,
                             const uint32_t* spirv_words,
                             size_t spirv_num_words,
                             spv_const_markv_encoder_options options,
                             spv_markv_binary* markv_binary,
                             spv_text* comments, spv_diagnostic* diagnostic);

// Decodes a SPIR-V binary from the given MARK-V binary.
// If |comments| is not nullptr, it would contain a textual description of
// how decoding was done (with snippets of disassembly and bit sequences).
spv_result_t spvMarkvToSpirv(spv_const_context context,
                             const uint8_t* markv_data,
                             size_t markv_size_bytes,
                             spv_const_markv_decoder_options options,
                             spv_binary* spirv_binary,
                             spv_text* comments, spv_diagnostic* diagnostic);

// Destroys MARK-V binary created by spvSpirvToMarkv().
void spvMarkvBinaryDestroy(spv_markv_binary binary);

#ifdef __cplusplus
}
#endif

#endif  // SPIRV_TOOLS_MARKV_H_
