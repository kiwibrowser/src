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

#ifndef LIBSPIRV_VALIDATE_H_
#define LIBSPIRV_VALIDATE_H_

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "assembly_grammar.h"
#include "binary.h"
#include "diagnostic.h"
#include "instruction.h"
#include "spirv-tools/libspirv.h"
#include "spirv_definition.h"
#include "table.h"

// Structures

// Info about a result ID.
typedef struct spv_id_info_t {
  // Id value.
  uint32_t id;
  // Type id, or 0 if no type.
  uint32_t type_id;
  // Opcode of the instruction defining the id.
  SpvOp opcode;
  // Binary words of the instruction defining the id.
  std::vector<uint32_t> words;
} spv_id_info_t;

namespace libspirv {

// This enum represents the sections of a SPIRV module. See section 2.4
// of the SPIRV spec for additional details of the order. The enumerant values
// are in the same order as the vector returned by GetModuleOrder
enum ModuleLayoutSection {
  kLayoutCapabilities,          // < Section 2.4 #1
  kLayoutExtensions,            // < Section 2.4 #2
  kLayoutExtInstImport,         // < Section 2.4 #3
  kLayoutMemoryModel,           // < Section 2.4 #4
  kLayoutEntryPoint,            // < Section 2.4 #5
  kLayoutExecutionMode,         // < Section 2.4 #6
  kLayoutDebug1,                // < Section 2.4 #7 > 1
  kLayoutDebug2,                // < Section 2.4 #7 > 2
  kLayoutAnnotations,           // < Section 2.4 #8
  kLayoutTypes,                 // < Section 2.4 #9
  kLayoutFunctionDeclarations,  // < Section 2.4 #10
  kLayoutFunctionDefinitions    // < Section 2.4 #11
};

enum class FunctionDecl {
  kFunctionDeclUnknown,      // < Unknown function declaration
  kFunctionDeclDeclaration,  // < Function declaration
  kFunctionDeclDefinition    // < Function definition
};

class ValidationState_t;

// This class manages all function declaration and definitions in a module. It
// handles the state and id information while parsing a function in the SPIR-V
// binary.
//
// NOTE: This class is designed to be a Structure of Arrays. Therefore each
// member variable is a vector whose elements represent the values for the
// corresponding function in a SPIR-V module. Variables that are not vector
// types are used to manage the state while parsing the function.
class Functions {
 public:
  explicit Functions(ValidationState_t& module);

  // Registers the function in the module. Subsequent instructions will be
  // called against this function
  spv_result_t RegisterFunction(uint32_t id, uint32_t ret_type_id,
                                uint32_t function_control,
                                uint32_t function_type_id);

  // Registers a function parameter in the current function
  spv_result_t RegisterFunctionParameter(uint32_t id, uint32_t type_id);

  // Register a function end instruction
  spv_result_t RegisterFunctionEnd();

  // Sets the declaration type of the current function
  spv_result_t RegisterSetFunctionDeclType(FunctionDecl type);

  // Registers a block in the current function. Subsequent block instructions
  // will target this block
  // @param id The ID of the label of the block
  spv_result_t RegisterBlock(uint32_t id);

  // Registers a variable in the current block
  spv_result_t RegisterBlockVariable(uint32_t type_id, uint32_t id,
                                     SpvStorageClass storage, uint32_t init_id);

  spv_result_t RegisterBlockLoopMerge(uint32_t merge_id, uint32_t continue_id,
                                      SpvLoopControlMask control);

  spv_result_t RegisterBlockSelectionMerge(uint32_t merge_id,
                                           SpvSelectionControlMask control);

  // Registers the end of the block
  spv_result_t RegisterBlockEnd();

  // Returns the number of blocks in the current function being parsed
  size_t get_block_count() const;

  // Returns true if called after a function instruction but before the
  // function end instruction
  bool in_function_body() const;

  // Returns true if called after a label instruction but before a branch
  // instruction
  bool in_block() const;

  libspirv::DiagnosticStream diag(spv_result_t error_code) const;

 private:
  // Parent module
  ValidationState_t& module_;

  // Function IDs in a module
  std::vector<uint32_t> id_;

  // OpTypeFunction IDs of each of the id_ functions
  std::vector<uint32_t> type_id_;

  // The type of declaration of each function
  std::vector<FunctionDecl> declaration_type_;

  // TODO(umar): Probably needs better abstractions
  // The beginning of the block of functions
  std::vector<std::vector<uint32_t>> block_ids_;

  // The variable IDs of the functions
  std::vector<std::vector<uint32_t>> variable_ids_;

  // The function parameter ids of the functions
  std::vector<std::vector<uint32_t>> parameter_ids_;

  // NOTE: See correspoding getter functions
  bool in_function_;
  bool in_block_;
};

class ValidationState_t {
 public:
  ValidationState_t(spv_diagnostic* diagnostic,
                    const spv_const_context context);

  // Forward declares the id in the module
  spv_result_t forwardDeclareId(uint32_t id);

  // Removes a forward declared ID if it has been defined
  spv_result_t removeIfForwardDeclared(uint32_t id);

  // Assigns a name to an ID
  void assignNameToId(uint32_t id, std::string name);

  // Returns a string representation of the ID in the format <id>[Name] where
  // the <id> is the numeric valid of the id and the Name is a name assigned by
  // the OpName instruction
  std::string getIdName(uint32_t id) const;

  // Returns the number of ID which have been forward referenced but not defined
  size_t unresolvedForwardIdCount() const;

  // Returns a list of unresolved forward ids.
  std::vector<uint32_t> unresolvedForwardIds() const;

  // Returns true if the id has been defined
  bool isDefinedId(uint32_t id) const;

  // Increments the instruction count. Used for diagnostic
  int incrementInstructionCount();

  // Returns the current layout section which is being processed
  ModuleLayoutSection getLayoutSection() const;

  // Increments the module_layout_order_section_
  void progressToNextLayoutSectionOrder();

  // Determines if the op instruction is part of the current section
  bool isOpcodeInCurrentLayoutSection(SpvOp op);

  libspirv::DiagnosticStream diag(spv_result_t error_code) const;

  // Returns the function states
  Functions& get_functions();

  // Returns true if the called after a function instruction but before the
  // function end instruction
  bool in_function_body() const;

  // Returns true if called after a label instruction but before a branch
  // instruction
  bool in_block() const;

  // Keeps track of ID definitions and uses.
  class UseDefTracker {
   public:
    void AddDef(const spv_id_info_t& def) { defs_[def.id] = def; }

    void AddUse(uint32_t id) { uses_.insert(id); }

    // Finds id's def, if it exists.  If found, returns <true, def>.  Otherwise,
    // returns <false, something>.
    std::pair<bool, spv_id_info_t> FindDef(uint32_t id) const {
      if (defs_.count(id) == 0) {
        return std::make_pair(false, spv_id_info_t{});
      } else {
        // We are in a const function, so we cannot use defs.operator[]().
        // Luckily we know the key exists, so defs_.at() won't throw an
        // exception.
        return std::make_pair(true, defs_.at(id));
      }
    }

    // Returns uses of IDs lacking defs.
    std::unordered_set<uint32_t> FindUsesWithoutDefs() const {
      auto diff = uses_;
      for (const auto d : defs_) diff.erase(d.first);
      return diff;
    }

   private:
    std::unordered_set<uint32_t> uses_;
    std::unordered_map<uint32_t, spv_id_info_t> defs_;
  };

  UseDefTracker& usedefs() { return usedefs_; }
  const UseDefTracker& usedefs() const { return usedefs_; }

  std::vector<uint32_t>& entry_points() { return entry_points_; }
  const std::vector<uint32_t>& entry_points() const { return entry_points_; }

  // Registers the capability and its dependent capabilities
  void registerCapability(SpvCapability cap);

  // Returns true if the capability is enabled in the module.
  bool hasCapability(SpvCapability cap) const;

  // Returns true if any of the capabilities are enabled.  Always true for
  // capabilities==0.
  bool HasAnyOf(spv_capability_mask_t capabilities) const;

  AssemblyGrammar& grammar() { return grammar_; }

 private:
  spv_diagnostic* diagnostic_;
  // Tracks the number of instructions evaluated by the validator
  int instruction_counter_;

  // IDs which have been forward declared but have not been defined
  std::unordered_set<uint32_t> unresolved_forward_ids_;

  std::map<uint32_t, std::string> operand_names_;

  // The section of the code being processed
  ModuleLayoutSection current_layout_section_;

  Functions module_functions_;

  spv_capability_mask_t module_capabilities_;  // Module's declared capabilities.

  // Definitions and uses of all the IDs in the module.
  UseDefTracker usedefs_;

  // IDs that are entry points, ie, arguments to OpEntryPoint.
  std::vector<uint32_t> entry_points_;

  AssemblyGrammar grammar_;
};

}  // namespace libspirv

// Functions

/// @brief Validate the ID usage of the instruction stream
///
/// @param[in] pInsts stream of instructions
/// @param[in] instCount number of instructions
/// @param[in] opcodeTable table of specified Opcodes
/// @param[in] operandTable table of specified operands
/// @param[in] usedefs use-def info from module parsing
/// @param[in,out] position current position in the stream
/// @param[out] pDiag contains diagnostic on failure
///
/// @return result code
spv_result_t spvValidateInstructionIDs(const spv_instruction_t* pInsts,
                                       const uint64_t instCount,
                                       const spv_opcode_table opcodeTable,
                                       const spv_operand_table operandTable,
                                       const spv_ext_inst_table extInstTable,
                                       const libspirv::ValidationState_t& state,
                                       spv_position position,
                                       spv_diagnostic* pDiag);

/// @brief Validate the ID's within a SPIR-V binary
///
/// @param[in] pInstructions array of instructions
/// @param[in] count number of elements in instruction array
/// @param[in] bound the binary header
/// @param[in] opcodeTable table of specified Opcodes
/// @param[in] operandTable table of specified operands
/// @param[in,out] position current word in the binary
/// @param[out] pDiagnostic contains diagnostic on failure
///
/// @return result code
spv_result_t spvValidateIDs(const spv_instruction_t* pInstructions,
                            const uint64_t count, const uint32_t bound,
                            const spv_opcode_table opcodeTable,
                            const spv_operand_table operandTable,
                            const spv_ext_inst_table extInstTable,
                            spv_position position, spv_diagnostic* pDiagnostic);

#define spvCheckReturn(expression) \
  if (spv_result_t error = (expression)) return error;

#endif  // LIBSPIRV_VALIDATE_H_
