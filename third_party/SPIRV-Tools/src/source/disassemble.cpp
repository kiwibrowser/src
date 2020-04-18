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

// This file contains a disassembler:  It converts a SPIR-V binary
// to text.

#include <cassert>
#include <cstring>
#include <iomanip>
#include <unordered_map>

#include "assembly_grammar.h"
#include "binary.h"
#include "diagnostic.h"
#include "ext_inst.h"
#include "opcode.h"
#include "print.h"
#include "spirv-tools/libspirv.h"
#include "spirv_constant.h"
#include "spirv_endian.h"
#include "util/hex_float.h"

namespace {

// A Disassembler instance converts a SPIR-V binary to its assembly
// representation.
class Disassembler {
 public:
  Disassembler(const libspirv::AssemblyGrammar& grammar, uint32_t options)
      : grammar_(grammar),
        print_(spvIsInBitfield(SPV_BINARY_TO_TEXT_OPTION_PRINT, options)),
        color_(print_ &&
               spvIsInBitfield(SPV_BINARY_TO_TEXT_OPTION_COLOR, options)),
        indent_(spvIsInBitfield(SPV_BINARY_TO_TEXT_OPTION_INDENT, options)
                    ? kStandardIndent
                    : 0),
        text_(),
        out_(print_ ? out_stream() : out_stream(text_)),
        stream_(out_.get()),
        show_byte_offset_(spvIsInBitfield(
            SPV_BINARY_TO_TEXT_OPTION_SHOW_BYTE_OFFSET, options)),
        byte_offset_(0) {}

  // Emits the assembly header for the module, and sets up internal state
  // so subsequent callbacks can handle the cases where the entire module
  // is either big-endian or little-endian.
  spv_result_t HandleHeader(spv_endianness_t endian, uint32_t version,
                            uint32_t generator, uint32_t id_bound,
                            uint32_t schema);
  // Emits the assembly text for the given instruction.
  spv_result_t HandleInstruction(const spv_parsed_instruction_t& inst);

  // If not printing, populates text_result with the accumulated text.
  // Returns SPV_SUCCESS on success.
  spv_result_t SaveTextResult(spv_text* text_result) const;

 private:
  enum { kStandardIndent = 15 };

  using out_stream = libspirv::out_stream;

  // Emits an operand for the given instruction, where the instruction
  // is at offset words from the start of the binary.
  void EmitOperand(const spv_parsed_instruction_t& inst,
                   const uint16_t operand_index);

  // Emits a mask expression for the given mask word of the specified type.
  void EmitMaskOperand(const spv_operand_type_t type, const uint32_t word);

  // Resets the output color, if color is turned on.
  void ResetColor() {
    if (color_) out_.get() << libspirv::clr::reset();
  }
  // Sets the output to grey, if color is turned on.
  void SetGrey() {
    if (color_) out_.get() << libspirv::clr::grey();
  }
  // Sets the output to blue, if color is turned on.
  void SetBlue() {
    if (color_) out_.get() << libspirv::clr::blue();
  }
  // Sets the output to yellow, if color is turned on.
  void SetYellow() {
    if (color_) out_.get() << libspirv::clr::yellow();
  }
  // Sets the output to red, if color is turned on.
  void SetRed() {
    if (color_) out_.get() << libspirv::clr::red();
  }
  // Sets the output to green, if color is turned on.
  void SetGreen() {
    if (color_) out_.get() << libspirv::clr::green();
  }

  const libspirv::AssemblyGrammar& grammar_;
  const bool print_;  // Should we also print to the standard output stream?
  const bool color_;  // Should we print in colour?
  const int indent_;  // How much to indent. 0 means don't indent
  spv_endianness_t endian_;  // The detected endianness of the binary.
  std::stringstream text_;   // Captures the text, if not printing.
  out_stream out_;  // The Output stream.  Either to text_ or standard output.
  std::ostream& stream_;  // The output std::stream.
  const bool show_byte_offset_;  // Should we print byte offset, in hex?
  size_t byte_offset_; // The number of bytes processed so far.
};

spv_result_t Disassembler::HandleHeader(spv_endianness_t endian,
                                        uint32_t version, uint32_t generator,
                                        uint32_t id_bound, uint32_t schema) {
  endian_ = endian;

  SetGrey();
  const char* generator_tool =
      spvGeneratorStr(SPV_GENERATOR_TOOL_PART(generator));
  stream_ << "; SPIR-V\n"
          << "; Version: " << SPV_SPIRV_VERSION_MAJOR_PART(version) << "."
          << SPV_SPIRV_VERSION_MINOR_PART(version) << "\n"
          << "; Generator: " << generator_tool;
  // For unknown tools, print the numeric tool value.
  if (0 == strcmp("Unknown", generator_tool)) {
    stream_ << "(" << SPV_GENERATOR_TOOL_PART(generator) << ")";
  }
  // Print the miscellaneous part of the generator word on the same
  // line as the tool name.
  stream_ << "; " << SPV_GENERATOR_MISC_PART(generator) << "\n"
          << "; Bound: " << id_bound << "\n"
          << "; Schema: " << schema << "\n";
  ResetColor();

  byte_offset_ = SPV_INDEX_INSTRUCTION * sizeof(uint32_t);

  return SPV_SUCCESS;
}

// Returns the number of digits in n.
int NumDigits(uint32_t n) {
  if (n < 10) return 0;
  if (n < 100) return 1;
  if (n < 1000) return 2;
  if (n < 10000) return 3;
  if (n < 100000) return 4;
  if (n < 1000000) return 5;
  if (n < 10000000) return 6;
  if (n < 100000000) return 7;
  if (n < 1000000000) return 8;
  return 9;
}

spv_result_t Disassembler::HandleInstruction(
    const spv_parsed_instruction_t& inst) {
  if (inst.result_id) {
    SetBlue();
    // Indent if needed, but account for the 4 characters in "%" and " = "
    if (indent_) stream_ << std::setw(indent_ - 4 - NumDigits(inst.result_id));
    stream_ << "%" << inst.result_id;
    ResetColor();
    stream_ << " = ";
  } else {
    stream_ << std::string(indent_, ' ');
  }

  stream_ << "Op" << spvOpcodeString(static_cast<SpvOp>(inst.opcode));

  for (uint16_t i = 0; i < inst.num_operands; i++) {
    const spv_operand_type_t type = inst.operands[i].type;
    assert(type != SPV_OPERAND_TYPE_NONE);
    if (type == SPV_OPERAND_TYPE_RESULT_ID) continue;
    stream_ << " ";
    EmitOperand(inst, i);
  }

  if (show_byte_offset_) {
    SetGrey();
    auto saved_flags = stream_.flags();
    auto saved_fill = stream_.fill();
    stream_ << " ; 0x" << std::setw(8) << std::hex << std::setfill('0')
            << byte_offset_;
    stream_.flags(saved_flags);
    stream_.fill(saved_fill);
    ResetColor();
  }

  byte_offset_ += inst.num_words * sizeof(uint32_t);

  stream_ << "\n";
  return SPV_SUCCESS;
}

void Disassembler::EmitOperand(const spv_parsed_instruction_t& inst,
                               const uint16_t operand_index) {
  assert(operand_index < inst.num_operands);
  const spv_parsed_operand_t& operand = inst.operands[operand_index];
  const uint32_t word = inst.words[operand.offset];
  switch (operand.type) {
    case SPV_OPERAND_TYPE_RESULT_ID:
      assert(false && "<result-id> is not supposed to be handled here");
      SetBlue();
      stream_ << "%" << word;
      break;
    case SPV_OPERAND_TYPE_ID:
    case SPV_OPERAND_TYPE_TYPE_ID:
    case SPV_OPERAND_TYPE_SCOPE_ID:
    case SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID:
      SetYellow();
      stream_ << "%" << word;
      break;
    case SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER: {
      spv_ext_inst_desc ext_inst;
      if (grammar_.lookupExtInst(inst.ext_inst_type, word, &ext_inst))
        assert(false && "should have caught this earlier");
      SetRed();
      stream_ << ext_inst->name;
    } break;
    case SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER: {
      spv_opcode_desc opcode_desc;
      if (grammar_.lookupOpcode(SpvOp(word), &opcode_desc))
        assert(false && "should have caught this earlier");
      SetRed();
      stream_ << opcode_desc->name;
    } break;
    case SPV_OPERAND_TYPE_LITERAL_INTEGER:
    case SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER: {
      SetRed();
      if (operand.num_words == 1) {
        switch (operand.number_kind) {
          case SPV_NUMBER_SIGNED_INT:
            stream_ << int32_t(word);
            break;
          case SPV_NUMBER_UNSIGNED_INT:
            stream_ << word;
            break;
          case SPV_NUMBER_FLOATING:
            if (operand.number_bit_width == 16) {
              stream_ << spvutils::FloatProxy<spvutils::Float16>(uint16_t(word & 0xFFFF));
            } else {
              // Assume 32-bit floats.
              stream_ << spvutils::FloatProxy<float>(word);
            }
            break;
          default:
            assert(false && "Unreachable");
        }
      } else if (operand.num_words == 2) {
        // Multi-word numbers are presented with lower order words first.
        uint64_t bits =
            uint64_t(word) | (uint64_t(inst.words[operand.offset + 1]) << 32);
        switch (operand.number_kind) {
          case SPV_NUMBER_SIGNED_INT:
            stream_ << int64_t(bits);
            break;
          case SPV_NUMBER_UNSIGNED_INT:
            stream_ << bits;
            break;
          case SPV_NUMBER_FLOATING:
            // Assume only 64-bit floats.
            stream_ << spvutils::FloatProxy<double>(bits);
            break;
          default:
            assert(false && "Unreachable");
        }
      } else {
        // TODO(dneto): Support more than 64-bits at a time.
        assert(false && "Unhandled");
      }
    } break;
    case SPV_OPERAND_TYPE_LITERAL_STRING: {
      stream_ << "\"";
      SetGreen();
      // Strings are always little-endian, and null-terminated.
      // Write out the characters, escaping as needed, and without copying
      // the entire string.
      auto c_str = reinterpret_cast<const char*>(inst.words + operand.offset);
      for (auto p = c_str; *p; ++p) {
        if (*p == '"' || *p == '\\') stream_ << '\\';
        stream_ << *p;
      }
      ResetColor();
      stream_ << '"';
    } break;
    case SPV_OPERAND_TYPE_CAPABILITY:
    case SPV_OPERAND_TYPE_SOURCE_LANGUAGE:
    case SPV_OPERAND_TYPE_EXECUTION_MODEL:
    case SPV_OPERAND_TYPE_ADDRESSING_MODEL:
    case SPV_OPERAND_TYPE_MEMORY_MODEL:
    case SPV_OPERAND_TYPE_EXECUTION_MODE:
    case SPV_OPERAND_TYPE_STORAGE_CLASS:
    case SPV_OPERAND_TYPE_DIMENSIONALITY:
    case SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE:
    case SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE:
    case SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT:
    case SPV_OPERAND_TYPE_FP_ROUNDING_MODE:
    case SPV_OPERAND_TYPE_LINKAGE_TYPE:
    case SPV_OPERAND_TYPE_ACCESS_QUALIFIER:
    case SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE:
    case SPV_OPERAND_TYPE_DECORATION:
    case SPV_OPERAND_TYPE_BUILT_IN:
    case SPV_OPERAND_TYPE_GROUP_OPERATION:
    case SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS:
    case SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO: {
      spv_operand_desc entry;
      if (grammar_.lookupOperand(operand.type, word, &entry))
        assert(false && "should have caught this earlier");
      stream_ << entry->name;
    } break;
    case SPV_OPERAND_TYPE_FP_FAST_MATH_MODE:
    case SPV_OPERAND_TYPE_FUNCTION_CONTROL:
    case SPV_OPERAND_TYPE_LOOP_CONTROL:
    case SPV_OPERAND_TYPE_IMAGE:
    case SPV_OPERAND_TYPE_MEMORY_ACCESS:
    case SPV_OPERAND_TYPE_SELECTION_CONTROL:
      EmitMaskOperand(operand.type, word);
      break;
    default:
      assert(false && "unhandled or invalid case");
  }
  ResetColor();
}

void Disassembler::EmitMaskOperand(const spv_operand_type_t type,
                                   const uint32_t word) {
  // Scan the mask from least significant bit to most significant bit.  For each
  // set bit, emit the name of that bit. Separate multiple names with '|'.
  uint32_t remaining_word = word;
  uint32_t mask;
  int num_emitted = 0;
  for (mask = 1; remaining_word; mask <<= 1) {
    if (remaining_word & mask) {
      remaining_word ^= mask;
      spv_operand_desc entry;
      if (grammar_.lookupOperand(type, mask, &entry))
        assert(false && "should have caught this earlier");
      if (num_emitted) stream_ << "|";
      stream_ << entry->name;
      num_emitted++;
    }
  }
  if (!num_emitted) {
    // An operand value of 0 was provided, so represent it by the name
    // of the 0 value. In many cases, that's "None".
    spv_operand_desc entry;
    if (SPV_SUCCESS == grammar_.lookupOperand(type, 0, &entry))
      stream_ << entry->name;
  }
}

spv_result_t Disassembler::SaveTextResult(spv_text* text_result) const {
  if (!print_) {
    size_t length = text_.str().size();
    char* str = new char[length + 1];
    if (!str) return SPV_ERROR_OUT_OF_MEMORY;
    strncpy(str, text_.str().c_str(), length + 1);
    spv_text text = new spv_text_t();
    if (!text) {
      delete[] str;
      return SPV_ERROR_OUT_OF_MEMORY;
    }
    text->str = str;
    text->length = length;
    *text_result = text;
  }
  return SPV_SUCCESS;
}

spv_result_t DisassembleHeader(void* user_data, spv_endianness_t endian,
                               uint32_t /* magic */, uint32_t version,
                               uint32_t generator, uint32_t id_bound,
                               uint32_t schema) {
  assert(user_data);
  auto disassembler = static_cast<Disassembler*>(user_data);
  return disassembler->HandleHeader(endian, version, generator, id_bound,
                                    schema);
}

spv_result_t DisassembleInstruction(
    void* user_data, const spv_parsed_instruction_t* parsed_instruction) {
  assert(user_data);
  auto disassembler = static_cast<Disassembler*>(user_data);
  return disassembler->HandleInstruction(*parsed_instruction);
}

}  // anonymous namespace

spv_result_t spvBinaryToText(const spv_const_context context,
                             const uint32_t* code, const size_t wordCount,
                             const uint32_t options, spv_text* pText,
                             spv_diagnostic* pDiagnostic) {
  // Invalid arguments return error codes, but don't necessarily generate
  // diagnostics.  These are programmer errors, not user errors.
  if (!pDiagnostic) return SPV_ERROR_INVALID_DIAGNOSTIC;
  const libspirv::AssemblyGrammar grammar(context);
  if (!grammar.isValid()) return SPV_ERROR_INVALID_TABLE;

  Disassembler disassembler(grammar, options);
  if (auto error = spvBinaryParse(context, &disassembler, code, wordCount,
                                  DisassembleHeader, DisassembleInstruction,
                                  pDiagnostic)) {
    return error;
  }

  return disassembler.SaveTextResult(pText);
}
