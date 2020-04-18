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

#ifndef LIBSPIRV_TEXT_HANDLER_H_
#define LIBSPIRV_TEXT_HANDLER_H_

#include <iomanip>
#include <sstream>
#include <type_traits>
#include <unordered_map>

#include "diagnostic.h"
#include "instruction.h"
#include "spirv-tools/libspirv.h"
#include "text.h"

namespace libspirv {
// Structures

// This is a lattice for tracking types.
enum class IdTypeClass {
  kBottom = 0,  // We have no information yet.
  kScalarIntegerType,
  kScalarFloatType,
  kOtherType
};

// Contains ID type information that needs to be tracked across all Ids.
// Bitwidth is only valid when type_class is kScalarIntegerType or
// kScalarFloatType.
struct IdType {
  uint32_t bitwidth;  // Safe to assume that we will not have > 2^32 bits.
  bool isSigned;      // This is only significant if type_class is integral.
  IdTypeClass type_class;
};

// Default equality operator for IdType. Tests if all members are the same.
inline bool operator==(const IdType& first, const IdType& second) {
  return (first.bitwidth == second.bitwidth) &&
         (first.isSigned == second.isSigned) &&
         (first.type_class == second.type_class);
}

// Tests whether any member of the IdTypes do not match.
inline bool operator!=(const IdType& first, const IdType& second) {
  return !(first == second);
}

// A value representing an unknown type.
extern const IdType kUnknownType;

// Returns true if the type is a scalar integer type.
inline bool isScalarIntegral(const IdType& type) {
  return type.type_class == IdTypeClass::kScalarIntegerType;
}

// Returns true if the type is a scalar floating point type.
inline bool isScalarFloating(const IdType& type) {
  return type.type_class == IdTypeClass::kScalarFloatType;
}

// Returns the number of bits in the type.
// This is only valid for bottom, scalar integer, and scalar floating
// classes.  For bottom, assume 32 bits.
inline int assumedBitWidth(const IdType& type) {
  switch (type.type_class) {
    case IdTypeClass::kBottom:
      return 32;
    case IdTypeClass::kScalarIntegerType:
    case IdTypeClass::kScalarFloatType:
      return type.bitwidth;
    default:
      break;
  }
  // We don't care about this case.
  return 0;
}

// A templated class with a static member function Clamp, where Clamp
// sets a referenced value of type T to 0 if T is an unsigned
// integer type, and returns true if it modified the referenced
// value.
template <typename T, typename = void>
class ClampToZeroIfUnsignedType {
 public:
  // The default specialization does not clamp the value.
  static bool Clamp(T*) { return false; }
};

// The specialization of ClampToZeroIfUnsignedType for unsigned integer
// types.
template <typename T>
class ClampToZeroIfUnsignedType<
    T, typename std::enable_if<std::is_unsigned<T>::value>::type> {
 public:
  static bool Clamp(T* value_pointer) {
    if (*value_pointer) {
      *value_pointer = 0;
      return true;
    }
    return false;
  }
};

// Encapsulates the data used during the assembly of a SPIR-V module.
class AssemblyContext {
 public:
  AssemblyContext(spv_text text, spv_diagnostic* diagnostic_arg)
      : current_position_({}),
        pDiagnostic_(diagnostic_arg),
        text_(text),
        bound_(1) {}

  // Assigns a new integer value to the given text ID, or returns the previously
  // assigned integer value if the ID has been seen before.
  uint32_t spvNamedIdAssignOrGet(const char* textValue);

  // Returns the largest largest numeric ID that has been assigned.
  uint32_t getBound() const;

  // Advances position to point to the next word in the input stream.
  // Returns SPV_SUCCESS on success.
  spv_result_t advance();

  // Sets word to the next word in the input text. Fills next_position with
  // the next location past the end of the word.
  spv_result_t getWord(std::string* word, spv_position next_position);

  // Returns true if the next word in the input is the start of a new Opcode.
  bool startsWithOp();

  // Returns true if the next word in the input is the start of a new
  // instruction.
  bool isStartOfNewInst();

  // Returns a diagnostic object initialized with current position in the input
  // stream, and for the given error code. Any data written to this object will
  // show up in pDiagnsotic on destruction.
  DiagnosticStream diagnostic(spv_result_t error) {
    return DiagnosticStream(current_position_, pDiagnostic_, error);
  }

  // Returns a diagnostic object with the default assembly error code.
  DiagnosticStream diagnostic() {
    // The default failure for assembly is invalid text.
    return diagnostic(SPV_ERROR_INVALID_TEXT);
  }

  // Returns then next character in the input stream.
  char peek() const;

  // Returns true if there is more text in the input stream.
  bool hasText() const;

  // Seeks the input stream forward by 'size' characters.
  void seekForward(uint32_t size);

  // Sets the current position in the input stream to the given position.
  void setPosition(const spv_position_t& newPosition) {
    current_position_ = newPosition;
  }

  // Returns the current position in the input stream.
  const spv_position_t& position() const { return current_position_; }

  // Appends the given 32-bit value to the given instruction.
  // Returns SPV_SUCCESS if the value could be correctly inserted in the
  // instruction.
  spv_result_t binaryEncodeU32(const uint32_t value, spv_instruction_t* pInst);

  // Appends the given string to the given instruction.
  // Returns SPV_SUCCESS if the value could be correctly inserted in the
  // instruction.
  spv_result_t binaryEncodeString(const char* value, spv_instruction_t* pInst);

  // Appends the given numeric literal to the given instruction.
  // Validates and respects the bitwidth supplied in the IdType argument.
  // If the type is of class kBottom the value will be encoded as a
  // 32-bit integer.
  // Returns SPV_SUCCESS if the value could be correctly added to the
  // instruction.  Returns the given error code on failure, and emits
  // a diagnostic if that error code is not SPV_FAILED_MATCH.
  spv_result_t binaryEncodeNumericLiteral(const char* numeric_literal,
                                          spv_result_t error_code,
                                          const IdType& type,
                                          spv_instruction_t* pInst);

  // Returns the IdType associated with this type-generating value.
  // If the type has not been previously recorded with recordTypeDefinition,
  // kUnknownType  will be returned.
  IdType getTypeOfTypeGeneratingValue(uint32_t value) const;

  // Returns the IdType that represents the return value of this Value
  // generating instruction.
  // If the value has not been recorded with recordTypeIdForValue, or the type
  // could not be determined kUnknownType will be returned.
  IdType getTypeOfValueInstruction(uint32_t value) const;

  // Tracks the type-defining instruction. The result of the tracking can
  // later be queried using getValueType.
  // pInst is expected to be completely filled in by the time this instruction
  // is called.
  // Returns SPV_SUCCESS on success, or SPV_ERROR_INVALID_VALUE on error.
  spv_result_t recordTypeDefinition(const spv_instruction_t* pInst);

  // Tracks the relationship between the value and its type.
  spv_result_t recordTypeIdForValue(uint32_t value, uint32_t type);

  // Records the given Id as being the import of the given extended instruction
  // type.
  spv_result_t recordIdAsExtInstImport(uint32_t id, spv_ext_inst_type_t type);

  // Returns the extended instruction type corresponding to the import with
  // the given Id, if it exists.  Returns SPV_EXT_INST_TYPE_NONE if the
  // id is not the id for an extended instruction type.
  spv_ext_inst_type_t getExtInstTypeForId(uint32_t id) const;

  // Parses a numeric value of a given type from the given text.  The number
  // should take up the entire string, and should be within bounds for the
  // target type.  On success, returns SPV_SUCCESS and populates the object
  // referenced by value_pointer. On failure, returns the given error code,
  // and emits a diagnostic if that error code is not SPV_FAILED_MATCH.
  template <typename T>
  spv_result_t parseNumber(const char* text, spv_result_t error_code,
                           T* value_pointer,
                           const char* error_message_fragment) {
    // C++11 doesn't define std::istringstream(int8_t&), so calling this method
    // with a single-byte type leads to implementation-defined behaviour.
    // Similarly for uint8_t.
    static_assert(sizeof(T) > 1,
                  "Don't use a single-byte type this parse method");

    std::istringstream text_stream(text);
    // Allow both decimal and hex input for integers.
    // It also allows octal input, but we don't care about that case.
    text_stream >> std::setbase(0);
    text_stream >> *value_pointer;

    // We should have read something.
    bool ok = (text[0] != 0) && !text_stream.bad();
    // It should have been all the text.
    ok = ok && text_stream.eof();
    // It should have been in range.
    ok = ok && !text_stream.fail();

    // Work around a bug in the GNU C++11 library. It will happily parse
    // "-1" for uint16_t as 65535.
    if (ok && text[0] == '-')
      ok = !ClampToZeroIfUnsignedType<T>::Clamp(value_pointer);

    if (ok) return SPV_SUCCESS;
    return diagnostic(error_code) << error_message_fragment << text;
  }

 private:

  // Appends the given floating point literal to the given instruction.
  // Returns SPV_SUCCESS if the value was correctly parsed.  Otherwise
  // returns the given error code, and emits a diagnostic if that error
  // code is not SPV_FAILED_MATCH.
  // Only 32 and 64 bit floating point numbers are supported.
  spv_result_t binaryEncodeFloatingPointLiteral(const char* numeric_literal,
                                                spv_result_t error_code,
                                                const IdType& type,
                                                spv_instruction_t* pInst);

  // Appends the given integer literal to the given instruction.
  // Returns SPV_SUCCESS if the value was correctly parsed.  Otherwise
  // returns the given error code, and emits a diagnostic if that error
  // code is not SPV_FAILED_MATCH.
  // Integers up to 64 bits are supported.
  spv_result_t binaryEncodeIntegerLiteral(const char* numeric_literal,
                                          spv_result_t error_code,
                                          const IdType& type,
                                          spv_instruction_t* pInst);

  // Writes the given 64-bit literal value into the instruction.
  // return SPV_SUCCESS if the value could be written in the instruction.
  spv_result_t binaryEncodeU64(const uint64_t value, spv_instruction_t* pInst);
  // Maps ID names to their corresponding numerical ids.
  using spv_named_id_table = std::unordered_map<std::string, uint32_t>;
  // Maps type-defining IDs to their IdType.
  using spv_id_to_type_map = std::unordered_map<uint32_t, IdType>;
  // Maps Ids to the id of their type.
  using spv_id_to_type_id = std::unordered_map<uint32_t, uint32_t>;

  spv_named_id_table named_ids_;
  spv_id_to_type_map types_;
  spv_id_to_type_id value_types_;
  // Maps an extended instruction import Id to the extended instruction type.
  std::unordered_map<uint32_t, spv_ext_inst_type_t> import_id_to_ext_inst_type_;
  spv_position_t current_position_;
  spv_diagnostic* pDiagnostic_;
  spv_text text_;
  uint32_t bound_;
};
}
#endif  // _LIBSPIRV_TEXT_HANDLER_H_
