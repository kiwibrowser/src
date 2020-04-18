// Copyright (c) 2015-2016 The Khronos Group Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

#ifndef SPIRV_TOOLS_LIBSPIRV_H_
#define SPIRV_TOOLS_LIBSPIRV_H_

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

// Helpers

#define SPV_BIT(shift) (1 << (shift))

#define SPV_FORCE_16_BIT_ENUM(name) _##name = 0x7fff
#define SPV_FORCE_32_BIT_ENUM(name) _##name = 0x7fffffff

// Enumerations

typedef enum spv_result_t {
  SPV_SUCCESS = 0,
  SPV_UNSUPPORTED = 1,
  SPV_END_OF_STREAM = 2,
  SPV_WARNING = 3,
  SPV_FAILED_MATCH = 4,
  SPV_REQUESTED_TERMINATION = 5,  // Success, but signals early termination.
  SPV_ERROR_INTERNAL = -1,
  SPV_ERROR_OUT_OF_MEMORY = -2,
  SPV_ERROR_INVALID_POINTER = -3,
  SPV_ERROR_INVALID_BINARY = -4,
  SPV_ERROR_INVALID_TEXT = -5,
  SPV_ERROR_INVALID_TABLE = -6,
  SPV_ERROR_INVALID_VALUE = -7,
  SPV_ERROR_INVALID_DIAGNOSTIC = -8,
  SPV_ERROR_INVALID_LOOKUP = -9,
  SPV_ERROR_INVALID_ID = -10,
  SPV_ERROR_INVALID_CFG = -11,
  SPV_ERROR_INVALID_LAYOUT = -12,
  SPV_ERROR_INVALID_CAPABILITY = -13,
  SPV_FORCE_32_BIT_ENUM(spv_result_t)
} spv_result_t;

typedef enum spv_endianness_t {
  SPV_ENDIANNESS_LITTLE,
  SPV_ENDIANNESS_BIG,
  SPV_FORCE_32_BIT_ENUM(spv_endianness_t)
} spv_endianness_t;

// The kinds of operands that an instruction may have.
//
// Some operand types are "concrete".  The binary parser uses a concrete
// operand type to describe an operand of a parsed instruction.
//
// The assembler uses all operand types.  In addition to determining what
// kind of value an operand may be, non-concrete operand types capture the
// fact that an operand might be optional (may be absent, or present exactly
// once), or might occur zero or more times.
//
// Sometimes we also need to be able to express the fact that an operand
// is a member of an optional tuple of values.  In that case the first member
// would be optional, and the subsequent members would be required.
typedef enum spv_operand_type_t {
  // A sentinel value.
  SPV_OPERAND_TYPE_NONE = 0,

#define FIRST_CONCRETE(ENUM) ENUM, SPV_OPERAND_TYPE_FIRST_CONCRETE_TYPE = ENUM
#define LAST_CONCRETE(ENUM) ENUM, SPV_OPERAND_TYPE_LAST_CONCRETE_TYPE = ENUM

  // Set 1:  Operands that are IDs.
  FIRST_CONCRETE(SPV_OPERAND_TYPE_ID),
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID,  // SPIR-V Sec 3.25
  SPV_OPERAND_TYPE_SCOPE_ID,             // SPIR-V Sec 3.27

  // Set 2:  Operands that are literal numbers.
  SPV_OPERAND_TYPE_LITERAL_INTEGER,  // Always unsigned 32-bits.
  // The Instruction argument to OpExtInst. It's an unsigned 32-bit literal
  // number indicating which instruction to use from an extended instruction
  // set.
  SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER,
  // The Opcode argument to OpSpecConstantOp. It determines the operation
  // to be performed on constant operands to compute a specialization constant
  // result.
  SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER,
  // A literal number whose format and size are determined by a previous operand
  // in the same instruction.  It's a signed integer, an unsigned integer, or a
  // floating point number.  It also has a specified bit width.  The width
  // may be larger than 32, which would require such a typed literal value to
  // occupy multiple SPIR-V words.
  SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER,

  // Set 3:  The literal string operand type.
  SPV_OPERAND_TYPE_LITERAL_STRING,

  // Set 4:  Operands that are a single word enumerated value.
  SPV_OPERAND_TYPE_SOURCE_LANGUAGE,               // SPIR-V Sec 3.2
  SPV_OPERAND_TYPE_EXECUTION_MODEL,               // SPIR-V Sec 3.3
  SPV_OPERAND_TYPE_ADDRESSING_MODEL,              // SPIR-V Sec 3.4
  SPV_OPERAND_TYPE_MEMORY_MODEL,                  // SPIR-V Sec 3.5
  SPV_OPERAND_TYPE_EXECUTION_MODE,                // SPIR-V Sec 3.6
  SPV_OPERAND_TYPE_STORAGE_CLASS,                 // SPIR-V Sec 3.7
  SPV_OPERAND_TYPE_DIMENSIONALITY,                // SPIR-V Sec 3.8
  SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE,       // SPIR-V Sec 3.9
  SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE,           // SPIR-V Sec 3.10
  SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT,          // SPIR-V Sec 3.11
  SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER,           // SPIR-V Sec 3.12
  SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE,       // SPIR-V Sec 3.13
  SPV_OPERAND_TYPE_FP_ROUNDING_MODE,              // SPIR-V Sec 3.16
  SPV_OPERAND_TYPE_LINKAGE_TYPE,                  // SPIR-V Sec 3.17
  SPV_OPERAND_TYPE_ACCESS_QUALIFIER,              // SPIR-V Sec 3.18
  SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE,  // SPIR-V Sec 3.19
  SPV_OPERAND_TYPE_DECORATION,                    // SPIR-V Sec 3.20
  SPV_OPERAND_TYPE_BUILT_IN,                      // SPIR-V Sec 3.21
  SPV_OPERAND_TYPE_GROUP_OPERATION,               // SPIR-V Sec 3.28
  SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS,              // SPIR-V Sec 3.29
  SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO,         // SPIR-V Sec 3.30
  SPV_OPERAND_TYPE_CAPABILITY,                    // SPIR-V Sec 3.31

// Set 5:  Operands that are a single word bitmask.
// Sometimes a set bit indicates the instruction requires still more operands.
#define FIRST_CONCRETE_MASK(ENUM) \
  ENUM, SPV_OPERAND_TYPE_FIRST_CONCRETE_MASK_TYPE = ENUM
  FIRST_CONCRETE_MASK(SPV_OPERAND_TYPE_IMAGE),    // SPIR-V Sec 3.14
  SPV_OPERAND_TYPE_FP_FAST_MATH_MODE,             // SPIR-V Sec 3.15
  SPV_OPERAND_TYPE_SELECTION_CONTROL,             // SPIR-V Sec 3.22
  SPV_OPERAND_TYPE_LOOP_CONTROL,                  // SPIR-V Sec 3.23
  SPV_OPERAND_TYPE_FUNCTION_CONTROL,              // SPIR-V Sec 3.24
  LAST_CONCRETE(SPV_OPERAND_TYPE_MEMORY_ACCESS),  // SPIR-V Sec 3.26
  SPV_OPERAND_TYPE_LAST_CONCRETE_MASK_TYPE =
      SPV_OPERAND_TYPE_LAST_CONCRETE_TYPE,
#undef FIRST_CONCRETE_MASK
#undef FIRST_CONCRETE
#undef LAST_CONCRETE

// The remaining operand types are only used internally by the assembler.
// There are two categories:
//    Optional : expands to 0 or 1 operand, like ? in regular expressions.
//    Variable : expands to 0, 1 or many operands or pairs of operands.
//               This is similar to * in regular expressions.

// Macros for defining bounds on optional and variable operand types.
// Any variable operand type is also optional.
#define FIRST_OPTIONAL(ENUM) ENUM, SPV_OPERAND_TYPE_FIRST_OPTIONAL_TYPE = ENUM
#define FIRST_VARIABLE(ENUM) ENUM, SPV_OPERAND_TYPE_FIRST_VARIABLE_TYPE = ENUM
#define LAST_VARIABLE(ENUM)                         \
  ENUM, SPV_OPERAND_TYPE_LAST_VARIABLE_TYPE = ENUM, \
        SPV_OPERAND_TYPE_LAST_OPTIONAL_TYPE = ENUM

  // An optional operand represents zero or one logical operands.
  // In an instruction definition, this may only appear at the end of the
  // operand types.
  FIRST_OPTIONAL(SPV_OPERAND_TYPE_OPTIONAL_ID),
  // An optional image operand type.
  SPV_OPERAND_TYPE_OPTIONAL_IMAGE,
  // An optional memory access type.
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  // An optional literal integer.
  SPV_OPERAND_TYPE_OPTIONAL_LITERAL_INTEGER,
  // An optional literal number, which may be either integer or floating point.
  SPV_OPERAND_TYPE_OPTIONAL_LITERAL_NUMBER,
  // Like SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER, but optional, and integral.
  SPV_OPERAND_TYPE_OPTIONAL_TYPED_LITERAL_INTEGER,
  // An optional literal string.
  SPV_OPERAND_TYPE_OPTIONAL_LITERAL_STRING,
  // An optional access qualifier
  SPV_OPERAND_TYPE_OPTIONAL_ACCESS_QUALIFIER,
  // An optional context-independent value, or CIV.  CIVs are tokens that we can
  // assemble regardless of where they occur -- literals, IDs, immediate
  // integers, etc.
  SPV_OPERAND_TYPE_OPTIONAL_CIV,

  // A variable operand represents zero or more logical operands.
  // In an instruction definition, this may only appear at the end of the
  // operand types.
  FIRST_VARIABLE(SPV_OPERAND_TYPE_VARIABLE_ID),
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER,
  // A sequence of zero or more pairs of (typed literal integer, Id).
  // Expands to zero or more:
  //  (SPV_OPERAND_TYPE_TYPED_LITERAL_INTEGER, SPV_OPERAND_TYPE_ID)
  // where the literal number must always be an integer of some sort.
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER_ID,
  // A sequence of zero or more pairs of (Id, Literal integer)
  LAST_VARIABLE(SPV_OPERAND_TYPE_VARIABLE_ID_LITERAL_INTEGER),

  // This is a sentinel value, and does not represent an operand type.
  // It should come last.
  SPV_OPERAND_TYPE_NUM_OPERAND_TYPES,

  SPV_FORCE_32_BIT_ENUM(spv_operand_type_t)
} spv_operand_type_t;

typedef enum spv_ext_inst_type_t {
  SPV_EXT_INST_TYPE_NONE = 0,
  SPV_EXT_INST_TYPE_GLSL_STD_450,
  SPV_EXT_INST_TYPE_OPENCL_STD,

  SPV_FORCE_32_BIT_ENUM(spv_ext_inst_type_t)
} spv_ext_inst_type_t;

// This determines at a high level the kind of a binary-encoded literal
// number, but not the bit width.
// In principle, these could probably be folded into new entries in
// spv_operand_type_t.  But then we'd have some special case differences
// between the assembler and disassembler.
typedef enum spv_number_kind_t {
  SPV_NUMBER_NONE = 0,  // The default for value initialization.
  SPV_NUMBER_UNSIGNED_INT,
  SPV_NUMBER_SIGNED_INT,
  SPV_NUMBER_FLOATING,
} spv_number_kind_t;

typedef enum spv_binary_to_text_options_t {
  SPV_BINARY_TO_TEXT_OPTION_NONE = SPV_BIT(0),
  SPV_BINARY_TO_TEXT_OPTION_PRINT = SPV_BIT(1),
  SPV_BINARY_TO_TEXT_OPTION_COLOR = SPV_BIT(2),
  SPV_BINARY_TO_TEXT_OPTION_INDENT = SPV_BIT(3),
  SPV_BINARY_TO_TEXT_OPTION_SHOW_BYTE_OFFSET = SPV_BIT(4),
  SPV_FORCE_32_BIT_ENUM(spv_binary_to_text_options_t)
} spv_binary_to_text_options_t;

// Structures

// Information about an operand parsed from a binary SPIR-V module.
// Note that the values are not included.  You still need access to the binary
// to extract the values.
typedef struct spv_parsed_operand_t {
  // Location of the operand, in words from the start of the instruction.
  uint16_t offset;
  // Number of words occupied by this operand.
  uint16_t num_words;
  // The "concrete" operand type.  See the definition of spv_operand_type_t
  // for details.
  spv_operand_type_t type;
  // If type is a literal number type, then number_kind says whether it's
  // a signed integer, an unsigned integer, or a floating point number.
  spv_number_kind_t number_kind;
  // The number of bits for a literal number type.
  uint32_t number_bit_width;
} spv_parsed_operand_t;

// An instruction parsed from a binary SPIR-V module.
typedef struct spv_parsed_instruction_t {
  // An array of words for this instruction, in native endianness.
  const uint32_t* words;
  // The number of words in this instruction.
  uint16_t num_words;
  uint16_t opcode;
  // The extended instruction type, if opcode is OpExtInst.  Otherwise
  // this is the "none" value.
  spv_ext_inst_type_t ext_inst_type;
  // The type id, or 0 if this instruction doesn't have one.
  uint32_t type_id;
  // The result id, or 0 if this instruction doesn't have one.
  uint32_t result_id;
  // The array of parsed operands.
  const spv_parsed_operand_t* operands;
  uint16_t num_operands;
} spv_parsed_instruction_t;

typedef struct spv_const_binary_t {
  const uint32_t* code;
  const size_t wordCount;
} spv_const_binary_t;

typedef struct spv_binary_t {
  uint32_t* code;
  size_t wordCount;
} spv_binary_t;

typedef struct spv_text_t {
  const char* str;
  size_t length;
} spv_text_t;

typedef struct spv_position_t {
  size_t line;
  size_t column;
  size_t index;
} spv_position_t;

typedef struct spv_diagnostic_t {
  spv_position_t position;
  char* error;
  bool isTextSource;
} spv_diagnostic_t;

// Opaque struct containing the context used to operate on a SPIR-V module.
// Its object is used by various translation API functions.
typedef struct spv_context_t spv_context_t;

// Type Definitions

typedef spv_const_binary_t* spv_const_binary;
typedef spv_binary_t* spv_binary;
typedef spv_text_t* spv_text;
typedef spv_position_t* spv_position;
typedef spv_diagnostic_t* spv_diagnostic;
typedef const spv_context_t* spv_const_context;
typedef spv_context_t* spv_context;

// Platform API

// Returns the SPIRV-Tools software version as a null-terminated string.
// The contents of the underlying storage is valid for the remainder of
// the process.
const char* spvSoftwareVersionString();
// Returns a null-terminated string containing the name of the project,
// the software version string, and commit details.
// The contents of the underlying storage is valid for the remainder of
// the process.
const char* spvSoftwareVersionDetailsString();

// Certain target environments impose additional restrictions on SPIR-V, so it's
// often necessary to specify which one applies.  SPV_ENV_UNIVERSAL means
// environment-agnostic SPIR-V.
typedef enum {
  SPV_ENV_UNIVERSAL_1_0,  // SPIR-V 1.0 latest revision, no other restrictions.
  SPV_ENV_VULKAN_1_0,     // Vulkan 1.0 latest revision.
  SPV_ENV_UNIVERSAL_1_1,  // SPIR-V 1.1 latest revision, no other restrictions.
} spv_target_env;

// Returns a string describing the given SPIR-V target environment.
const char* spvTargetEnvDescription(spv_target_env env);

// Creates a context object.  Returns null if env is invalid.
spv_context spvContextCreate(spv_target_env env);

// Destroys the given context object.
void spvContextDestroy(spv_context context);

// Encodes the given SPIR-V assembly text to its binary representation. The
// length parameter specifies the number of bytes for text. Encoded binary will
// be stored into *binary. Any error will be written into *diagnostic. The
// generated binary is independent of the context and may outlive it.
spv_result_t spvTextToBinary(const spv_const_context context, const char* text,
                             const size_t length, spv_binary* binary,
                             spv_diagnostic* diagnostic);

// Frees an allocated text stream. This is a no-op if the text parameter
// is a null pointer.
void spvTextDestroy(spv_text text);

// Decodes the given SPIR-V binary representation to its assembly text. The
// word_count parameter specifies the number of words for binary. The options
// parameter is a bit field of spv_binary_to_text_options_t. Decoded text will
// be stored into *text. Any error will be written into *diagnostic.
spv_result_t spvBinaryToText(const spv_const_context context,
                             const uint32_t* binary, const size_t word_count,
                             const uint32_t options, spv_text* text,
                             spv_diagnostic* diagnostic);

// Frees a binary stream from memory. This is a no-op if binary is a null
// pointer.
void spvBinaryDestroy(spv_binary binary);

// Validates a SPIR-V binary for correctness. Any errors will be written into
// *diagnostic.
spv_result_t spvValidate(const spv_const_context context,
                         const spv_const_binary binary,
                         spv_diagnostic* diagnostic);

// Creates a diagnostic object. The position parameter specifies the location in
// the text/binary stream. The message parameter, copied into the diagnostic
// object, contains the error message to display.
spv_diagnostic spvDiagnosticCreate(const spv_position position,
                                   const char* message);

// Destroys a diagnostic object.  This is a no-op if diagnostic is a null
// pointer.
void spvDiagnosticDestroy(spv_diagnostic diagnostic);

// Prints the diagnostic to stderr.
spv_result_t spvDiagnosticPrint(const spv_diagnostic diagnostic);

// The binary parser interface.

// A pointer to a function that accepts a parsed SPIR-V header.
// The integer arguments are the 32-bit words from the header, as specified
// in SPIR-V 1.0 Section 2.3 Table 1.
// The function should return SPV_SUCCESS if parsing should continue.
typedef spv_result_t (*spv_parsed_header_fn_t)(
    void* user_data, spv_endianness_t endian, uint32_t magic, uint32_t version,
    uint32_t generator, uint32_t id_bound, uint32_t reserved);

// A pointer to a function that accepts a parsed SPIR-V instruction.
// The parsed_instruction value is transient: it may be overwritten
// or released immediately after the function has returned.  That also
// applies to the words array member of the parsed instruction.  The
// function should return SPV_SUCCESS if and only if parsing should
// continue.
typedef spv_result_t (*spv_parsed_instruction_fn_t)(
    void* user_data, const spv_parsed_instruction_t* parsed_instruction);

// Parses a SPIR-V binary, specified as counted sequence of 32-bit words.
// Parsing feedback is provided via two callbacks provided as function
// pointers.  Each callback function pointer can be a null pointer, in
// which case it is never called.  Otherwise, in a valid parse the
// parsed-header callback is called once, and then the parsed-instruction
// callback once for each instruction in the stream.  The user_data parameter
// is supplied as context to the callbacks.  Returns SPV_SUCCESS on successful
// parse where the callbacks always return SPV_SUCCESS.  For an invalid parse,
// returns a status code other than SPV_SUCCESS and emits a diagnostic.  If a
// callback returns anything other than SPV_SUCCESS, then that status code
// is returned, no further callbacks are issued, and no additional diagnostics
// are emitted.
spv_result_t spvBinaryParse(const spv_const_context context, void* user_data,
                            const uint32_t* words, const size_t num_words,
                            spv_parsed_header_fn_t parse_header,
                            spv_parsed_instruction_fn_t parse_instruction,
                            spv_diagnostic* diagnostic);

#ifdef __cplusplus
}
#endif

#endif  // SPIRV_TOOLS_LIBSPIRV_H_
