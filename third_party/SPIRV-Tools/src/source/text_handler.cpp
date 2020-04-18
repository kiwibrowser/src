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

#include "text_handler.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <tuple>

#include "assembly_grammar.h"
#include "binary.h"
#include "ext_inst.h"
#include "instruction.h"
#include "opcode.h"
#include "text.h"
#include "util/bitutils.h"
#include "util/hex_float.h"

namespace {

using spvutils::BitwiseCast;
using spvutils::FloatProxy;
using spvutils::HexFloat;

// Advances |text| to the start of the next line and writes the new position to
// |position|.
spv_result_t advanceLine(spv_text text, spv_position position) {
  while (true) {
    if (position->index >= text->length) return SPV_END_OF_STREAM;
    switch (text->str[position->index]) {
      case '\0':
        return SPV_END_OF_STREAM;
      case '\n':
        position->column = 0;
        position->line++;
        position->index++;
        return SPV_SUCCESS;
      default:
        position->column++;
        position->index++;
        break;
    }
  }
}

// Advances |text| to first non white space character and writes the new
// position to |position|.
// If a null terminator is found during the text advance, SPV_END_OF_STREAM is
// returned, SPV_SUCCESS otherwise. No error checking is performed on the
// parameters, its the users responsibility to ensure these are non null.
spv_result_t advance(spv_text text, spv_position position) {
  // NOTE: Consume white space, otherwise don't advance.
  if (position->index >= text->length) return SPV_END_OF_STREAM;
  switch (text->str[position->index]) {
    case '\0':
      return SPV_END_OF_STREAM;
    case ';':
      if (spv_result_t error = advanceLine(text, position)) return error;
      return advance(text, position);
    case ' ':
    case '\t':
    case '\r':
      position->column++;
      position->index++;
      return advance(text, position);
    case '\n':
      position->column = 0;
      position->line++;
      position->index++;
      return advance(text, position);
    default:
      break;
  }
  return SPV_SUCCESS;
}

// Fetches the next word from the given text stream starting from the given
// *position. On success, writes the decoded word into *word and updates
// *position to the location past the returned word.
//
// A word ends at the next comment or whitespace.  However, double-quoted
// strings remain intact, and a backslash always escapes the next character.
spv_result_t getWord(spv_text text, spv_position position, std::string* word) {
  if (!text->str || !text->length) return SPV_ERROR_INVALID_TEXT;
  if (!position) return SPV_ERROR_INVALID_POINTER;

  const size_t start_index = position->index;

  bool quoting = false;
  bool escaping = false;

  // NOTE: Assumes first character is not white space!
  while (true) {
    if (position->index >= text->length) {
      word->assign(text->str + start_index, text->str + position->index);
      return SPV_SUCCESS;
    }
    const char ch = text->str[position->index];
    if (ch == '\\')
      escaping = !escaping;
    else {
      switch (ch) {
        case '"':
          if (!escaping) quoting = !quoting;
          break;
        case ' ':
        case ';':
        case '\t':
        case '\n':
        case '\r':
          if (escaping || quoting) break;
        // Fall through.
        case '\0': {  // NOTE: End of word found!
          word->assign(text->str + start_index, text->str + position->index);
          return SPV_SUCCESS;
        }
        default:
          break;
      }
      escaping = false;
    }

    position->column++;
    position->index++;
  }
}

// Returns true if the characters in the text as position represent
// the start of an Opcode.
bool startsWithOp(spv_text text, spv_position position) {
  if (text->length < position->index + 3) return false;
  char ch0 = text->str[position->index];
  char ch1 = text->str[position->index + 1];
  char ch2 = text->str[position->index + 2];
  return ('O' == ch0 && 'p' == ch1 && ('A' <= ch2 && ch2 <= 'Z'));
}

}  // anonymous namespace

namespace libspirv {

const IdType kUnknownType = {0, false, IdTypeClass::kBottom};

// TODO(dneto): Reorder AssemblyContext definitions to match declaration order.

// This represents all of the data that is only valid for the duration of
// a single compilation.
uint32_t AssemblyContext::spvNamedIdAssignOrGet(const char* textValue) {
  if (named_ids_.end() == named_ids_.find(textValue)) {
    named_ids_[std::string(textValue)] = bound_++;
  }
  return named_ids_[textValue];
}
uint32_t AssemblyContext::getBound() const { return bound_; }

spv_result_t AssemblyContext::advance() {
  return ::advance(text_, &current_position_);
}

spv_result_t AssemblyContext::getWord(std::string* word,
                                      spv_position next_position) {
  *next_position = current_position_;
  return ::getWord(text_, next_position, word);
}

bool AssemblyContext::startsWithOp() {
  return ::startsWithOp(text_, &current_position_);
}

bool AssemblyContext::isStartOfNewInst() {
  spv_position_t pos = current_position_;
  if (::advance(text_, &pos)) return false;
  if (::startsWithOp(text_, &pos)) return true;

  std::string word;
  pos = current_position_;
  if (::getWord(text_, &pos, &word)) return false;
  if ('%' != word.front()) return false;

  if (::advance(text_, &pos)) return false;
  if (::getWord(text_, &pos, &word)) return false;
  if ("=" != word) return false;

  if (::advance(text_, &pos)) return false;
  if (::startsWithOp(text_, &pos)) return true;
  return false;
}

char AssemblyContext::peek() const {
  return text_->str[current_position_.index];
}

bool AssemblyContext::hasText() const {
  return text_->length > current_position_.index;
}

void AssemblyContext::seekForward(uint32_t size) {
  current_position_.index += size;
  current_position_.column += size;
}

spv_result_t AssemblyContext::binaryEncodeU32(const uint32_t value,
                                              spv_instruction_t* pInst) {
  pInst->words.insert(pInst->words.end(), value);
  return SPV_SUCCESS;
}

spv_result_t AssemblyContext::binaryEncodeU64(const uint64_t value,
                                              spv_instruction_t* pInst) {
  uint32_t low = uint32_t(0x00000000ffffffff & value);
  uint32_t high = uint32_t((0xffffffff00000000 & value) >> 32);
  binaryEncodeU32(low, pInst);
  binaryEncodeU32(high, pInst);
  return SPV_SUCCESS;
}

spv_result_t AssemblyContext::binaryEncodeNumericLiteral(
    const char* val, spv_result_t error_code, const IdType& type,
    spv_instruction_t* pInst) {
  const bool is_bottom = type.type_class == libspirv::IdTypeClass::kBottom;
  const bool is_floating = libspirv::isScalarFloating(type);
  const bool is_integer = libspirv::isScalarIntegral(type);

  if (!is_bottom && !is_floating && !is_integer) {
    return diagnostic(SPV_ERROR_INTERNAL)
           << "The expected type is not a scalar integer or float type";
  }

  // If this is bottom, but looks like a float, we should treat it like a
  // float.
  const bool looks_like_float = is_bottom && strchr(val, '.');

  // If we explicitly expect a floating-point number, we should handle that
  // first.
  if (is_floating || looks_like_float)
    return binaryEncodeFloatingPointLiteral(val, error_code, type, pInst);

  return binaryEncodeIntegerLiteral(val, error_code, type, pInst);
}

spv_result_t AssemblyContext::binaryEncodeString(const char* value,
                                                 spv_instruction_t* pInst) {
  const size_t length = strlen(value);
  const size_t wordCount = (length / 4) + 1;
  const size_t oldWordCount = pInst->words.size();
  const size_t newWordCount = oldWordCount + wordCount;

  // TODO(dneto): We can just defer this check until later.
  if (newWordCount > SPV_LIMIT_INSTRUCTION_WORD_COUNT_MAX) {
    return diagnostic() << "Instruction too long: more than "
                        << SPV_LIMIT_INSTRUCTION_WORD_COUNT_MAX << " words.";
  }

  pInst->words.resize(newWordCount);

  // Make sure all the bytes in the last word are 0, in case we only
  // write a partial word at the end.
  pInst->words.back() = 0;

  char* dest = (char*)&pInst->words[oldWordCount];
  strncpy(dest, value, length);

  return SPV_SUCCESS;
}

spv_result_t AssemblyContext::recordTypeDefinition(
    const spv_instruction_t* pInst) {
  uint32_t value = pInst->words[1];
  if (types_.find(value) != types_.end()) {
    return diagnostic() << "Value " << value
                        << " has already been used to generate a type";
  }

  if (pInst->opcode == SpvOpTypeInt) {
    if (pInst->words.size() != 4)
      return diagnostic() << "Invalid OpTypeInt instruction";
    types_[value] = {pInst->words[2], pInst->words[3] != 0,
                     IdTypeClass::kScalarIntegerType};
  } else if (pInst->opcode == SpvOpTypeFloat) {
    if (pInst->words.size() != 3)
      return diagnostic() << "Invalid OpTypeFloat instruction";
    types_[value] = {pInst->words[2], false, IdTypeClass::kScalarFloatType};
  } else {
    types_[value] = {0, false, IdTypeClass::kOtherType};
  }
  return SPV_SUCCESS;
}

IdType AssemblyContext::getTypeOfTypeGeneratingValue(uint32_t value) const {
  auto type = types_.find(value);
  if (type == types_.end()) {
    return kUnknownType;
  }
  return std::get<1>(*type);
}

IdType AssemblyContext::getTypeOfValueInstruction(uint32_t value) const {
  auto type_value = value_types_.find(value);
  if (type_value == value_types_.end()) {
    return {0, false, IdTypeClass::kBottom};
  }
  return getTypeOfTypeGeneratingValue(std::get<1>(*type_value));
}

spv_result_t AssemblyContext::recordTypeIdForValue(uint32_t value,
                                                   uint32_t type) {
  bool successfully_inserted = false;
  std::tie(std::ignore, successfully_inserted) =
      value_types_.insert(std::make_pair(value, type));
  if (!successfully_inserted)
    return diagnostic() << "Value is being defined a second time";
  return SPV_SUCCESS;
}

spv_result_t AssemblyContext::recordIdAsExtInstImport(
    uint32_t id, spv_ext_inst_type_t type) {
  bool successfully_inserted = false;
  std::tie(std::ignore, successfully_inserted) =
      import_id_to_ext_inst_type_.insert(std::make_pair(id, type));
  if (!successfully_inserted)
    return diagnostic() << "Import Id is being defined a second time";
  return SPV_SUCCESS;
}

spv_ext_inst_type_t AssemblyContext::getExtInstTypeForId(uint32_t id) const {
  auto type = import_id_to_ext_inst_type_.find(id);
  if (type == import_id_to_ext_inst_type_.end()) {
    return SPV_EXT_INST_TYPE_NONE;
  }
  return std::get<1>(*type);
}

spv_result_t AssemblyContext::binaryEncodeFloatingPointLiteral(
    const char* val, spv_result_t error_code, const IdType& type,
    spv_instruction_t* pInst) {
  const auto bit_width = assumedBitWidth(type);
  switch (bit_width) {
    case 16: {
      spvutils::HexFloat<FloatProxy<spvutils::Float16>> hVal(0);
      if (auto error = parseNumber(val, error_code, &hVal,
                                   "Invalid 16-bit float literal: "))
        return error;
      // getAsFloat will return the spvutils::Float16 value, and get_value
      // will return a uint16_t representing the bits of the float.
      // The encoding is therefore correct from the perspective of the SPIR-V
      // spec since the top 16 bits will be 0.
      return binaryEncodeU32(
          static_cast<uint32_t>(hVal.value().getAsFloat().get_value()), pInst);
    } break;
    case 32: {
      spvutils::HexFloat<FloatProxy<float>> fVal(0.0f);
      if (auto error = parseNumber(val, error_code, &fVal,
                                   "Invalid 32-bit float literal: "))
        return error;
      return binaryEncodeU32(BitwiseCast<uint32_t>(fVal), pInst);
    } break;
    case 64: {
      spvutils::HexFloat<FloatProxy<double>> dVal(0.0);
      if (auto error = parseNumber(val, error_code, &dVal,
                                   "Invalid 64-bit float literal: "))
        return error;
      return binaryEncodeU64(BitwiseCast<uint64_t>(dVal), pInst);
    } break;
    default:
      break;
  }
  return diagnostic() << "Unsupported " << bit_width << "-bit float literals";
}

// Returns SPV_SUCCESS if the given value fits within the target scalar
// integral type.  The target type may have an unusual bit width.
// If the value was originally specified as a hexadecimal number, then
// the overflow bits should be zero.  If it was hex and the target type is
// signed, then return the sign-extended value through the
// updated_value_for_hex pointer argument.
// On failure, return the given error code and emit a diagnostic if that error
// code is not SPV_FAILED_MATCH.
template <typename T>
spv_result_t checkRangeAndIfHexThenSignExtend(T value, spv_result_t error_code,
                                              const IdType& type, bool is_hex,
                                              T* updated_value_for_hex) {
  // The encoded result has three regions of bits that are of interest, from
  // least to most significant:
  //   - magnitude bits, where the magnitude of the number would be stored if
  //     we were using a signed-magnitude representation.
  //   - an optional sign bit
  //   - overflow bits, up to bit 63 of a 64-bit number
  // For example:
  //   Type                Overflow      Sign       Magnitude
  //   ---------------     --------      ----       ---------
  //   unsigned 8 bit      8-63          n/a        0-7
  //   signed 8 bit        8-63          7          0-6
  //   unsigned 16 bit     16-63         n/a        0-15
  //   signed 16 bit       16-63         15         0-14

  // We'll use masks to define the three regions.
  // At first we'll assume the number is unsigned.
  const uint32_t bit_width = assumedBitWidth(type);
  uint64_t magnitude_mask =
      (bit_width == 64) ? -1 : ((uint64_t(1) << bit_width) - 1);
  uint64_t sign_mask = 0;
  uint64_t overflow_mask = ~magnitude_mask;

  if (value < 0 || type.isSigned) {
    // Accommodate the sign bit.
    magnitude_mask >>= 1;
    sign_mask = magnitude_mask + 1;
  }

  bool failed = false;
  if (value < 0) {
    // The top bits must all be 1 for a negative signed value.
    failed = ((value & overflow_mask) != overflow_mask) ||
             ((value & sign_mask) != sign_mask);
  } else {
    if (is_hex) {
      // Hex values are a bit special. They decode as unsigned values, but
      // may represent a negative number.  In this case, the overflow bits
      // should be zero.
      failed = (value & overflow_mask) != 0;
    } else {
      const uint64_t value_as_u64 = static_cast<uint64_t>(value);
      // Check overflow in the ordinary case.
      failed = (value_as_u64 & magnitude_mask) != value_as_u64;
    }
  }

  if (failed) {
    return error_code;
  }

  // Sign extend hex the number.
  if (is_hex && (value & sign_mask))
    *updated_value_for_hex = (value | overflow_mask);

  return SPV_SUCCESS;
}

spv_result_t AssemblyContext::binaryEncodeIntegerLiteral(
    const char* val, spv_result_t error_code, const IdType& type,
    spv_instruction_t* pInst) {
  const bool is_bottom = type.type_class == libspirv::IdTypeClass::kBottom;
  const uint32_t bit_width = assumedBitWidth(type);

  if (bit_width > 64)
    return diagnostic(SPV_ERROR_INTERNAL) << "Unsupported " << bit_width
                                          << "-bit integer literals";

  // Either we are expecting anything or integer.
  bool is_negative = val[0] == '-';
  bool can_be_signed = is_bottom || type.isSigned;

  if (is_negative && !can_be_signed) {
    return diagnostic()
           << "Cannot put a negative number in an unsigned literal";
  }

  const bool is_hex = val[0] == '0' && (val[1] == 'x' || val[1] == 'X');

  uint64_t decoded_bits;
  if (is_negative) {
    int64_t decoded_signed = 0;

    if (auto error = parseNumber(val, error_code, &decoded_signed,
                                 "Invalid signed integer literal: "))
      return error;
    if (auto error = checkRangeAndIfHexThenSignExtend(
            decoded_signed, error_code, type, is_hex, &decoded_signed)) {
      diagnostic(error_code)
          << "Integer " << (is_hex ? std::hex : std::dec) << std::showbase
          << decoded_signed << " does not fit in a " << std::dec << bit_width
          << "-bit " << (type.isSigned ? "signed" : "unsigned") << " integer";
      return error;
    }
    decoded_bits = decoded_signed;
  } else {
    // There's no leading minus sign, so parse it as an unsigned integer.
    if (auto error = parseNumber(val, error_code, &decoded_bits,
                                 "Invalid unsigned integer literal: "))
      return error;
    if (auto error = checkRangeAndIfHexThenSignExtend(
            decoded_bits, error_code, type, is_hex, &decoded_bits)) {
      diagnostic(error_code)
          << "Integer " << (is_hex ? std::hex : std::dec) << std::showbase
          << decoded_bits << " does not fit in a " << std::dec << bit_width
          << "-bit " << (type.isSigned ? "signed" : "unsigned") << " integer";
      return error;
    }
  }
  if (bit_width > 32) {
    return binaryEncodeU64(decoded_bits, pInst);
  } else {
    return binaryEncodeU32(uint32_t(decoded_bits), pInst);
  }
}
}  // namespace libspirv
