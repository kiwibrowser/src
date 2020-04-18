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

// Source code for logical layout validation as described in section 2.4

#include "spirv-tools/libspirv.h"
#include "validate_passes.h"

#include "diagnostic.h"
#include "opcode.h"
#include "operand.h"

#include <cassert>

using libspirv::ValidationState_t;
using libspirv::kLayoutMemoryModel;
using libspirv::kLayoutFunctionDeclarations;
using libspirv::kLayoutFunctionDefinitions;
using libspirv::FunctionDecl;

namespace {

// Module scoped instructions are processed by determining if the opcode
// is part of the current layout section. If it is not then the next sections is
// checked.
spv_result_t ModuleScopedInstructions(ValidationState_t& _,
                                      const spv_parsed_instruction_t* inst,
                                      SpvOp opcode) {
  while (_.isOpcodeInCurrentLayoutSection(opcode) == false) {
    _.progressToNextLayoutSectionOrder();

    switch (_.getLayoutSection()) {
      case kLayoutMemoryModel:
        if (opcode != SpvOpMemoryModel) {
          return _.diag(SPV_ERROR_INVALID_LAYOUT)
                 << spvOpcodeString(opcode)
                 << " cannot appear before the memory model instruction";
        }
        break;
      case kLayoutFunctionDeclarations:
        // All module sections have been processed. Recursivly call
        // ModuleLayoutPass to process the next section of the module
        return libspirv::ModuleLayoutPass(_, inst);
      default:
        break;
    }
  }
  return SPV_SUCCESS;
}

// Function declaration validation is performed by making sure that the
// FunctionParameter and FunctionEnd instructions only appear inside of
// functions. It also ensures that the Function instruction does not appear
// inside of another function. This stage ends when the first label is
// encountered inside of a function.
spv_result_t FunctionScopedInstructions(ValidationState_t& _,
                                        const spv_parsed_instruction_t* inst,
                                        SpvOp opcode) {
  if (_.isOpcodeInCurrentLayoutSection(opcode)) {
    switch (opcode) {
      case SpvOpFunction:
        if (_.in_function_body()) {
          return _.diag(SPV_ERROR_INVALID_LAYOUT)
                 << "Cannot declare a function in a function body";
        }
        spvCheckReturn(_.get_functions().RegisterFunction(
            inst->result_id, inst->type_id,
            inst->words[inst->operands[2].offset],
            inst->words[inst->operands[3].offset]));
        if (_.getLayoutSection() == kLayoutFunctionDefinitions)
          spvCheckReturn(_.get_functions().RegisterSetFunctionDeclType(
              FunctionDecl::kFunctionDeclDefinition));
        break;

      case SpvOpFunctionParameter:
        if (_.in_function_body() == false) {
          return _.diag(SPV_ERROR_INVALID_LAYOUT) << "Function parameter "
                                                     "instructions must be in "
                                                     "a function body";
        }
        if (_.get_functions().get_block_count() != 0) {
          return _.diag(SPV_ERROR_INVALID_LAYOUT)
                 << "Function parameters must only appear immediately after the "
                    "function definition";
        }
        spvCheckReturn(_.get_functions().RegisterFunctionParameter(
            inst->result_id, inst->type_id));
        break;

      case SpvOpFunctionEnd:
        if (_.in_function_body() == false) {
          return _.diag(SPV_ERROR_INVALID_LAYOUT)
                 << "Function end instructions must be in a function body";
        }
        if (_.in_block()) {
          return _.diag(SPV_ERROR_INVALID_LAYOUT)
                 << "Function end cannot be called in blocks";
        }
        if (_.get_functions().get_block_count() == 0 &&
            _.getLayoutSection() == kLayoutFunctionDefinitions) {
          return _.diag(SPV_ERROR_INVALID_LAYOUT) << "Function declarations "
                                                     "must appear before "
                                                     "function definitions.";
        }
        spvCheckReturn(_.get_functions().RegisterFunctionEnd());
        if (_.getLayoutSection() == kLayoutFunctionDeclarations) {
          spvCheckReturn(_.get_functions().RegisterSetFunctionDeclType(
              FunctionDecl::kFunctionDeclDeclaration));
        }
        break;

      case SpvOpLine:
      case SpvOpNoLine:
        break;
      case SpvOpLabel:
        // If the label is encountered then the current function is a
        // definition so set the function to a declaration and update the
        // module section
        if (_.in_function_body() == false) {
          return _.diag(SPV_ERROR_INVALID_LAYOUT)
                 << "Label instructions must be in a function body";
        }
        if (_.in_block()) {
          return _.diag(SPV_ERROR_INVALID_LAYOUT)
                 << "A block must end with a branch instruction.";
        }
        if (_.getLayoutSection() == kLayoutFunctionDeclarations) {
          _.progressToNextLayoutSectionOrder();
          spvCheckReturn(_.get_functions().RegisterSetFunctionDeclType(
              FunctionDecl::kFunctionDeclDefinition));
        }
        break;

      default:
        if (_.getLayoutSection() == kLayoutFunctionDeclarations) {
          return _.diag(SPV_ERROR_INVALID_LAYOUT)
                 << "A function must begin with a label";
        } else {
          if (_.in_block() == false) {
            return _.diag(SPV_ERROR_INVALID_LAYOUT)
                   << spvOpcodeString(opcode) << " must appear in a block";
          }
        }
        break;
    }
  } else {
    return _.diag(SPV_ERROR_INVALID_LAYOUT)
           << spvOpcodeString(opcode)
           << " cannot appear in a function declaration";
  }
  return SPV_SUCCESS;
}
}

namespace libspirv {
// TODO(umar): Check linkage capabilities for function declarations
// TODO(umar): Better error messages
// NOTE: This function does not handle CFG related validation
// Performs logical layout validation. See Section 2.4
spv_result_t ModuleLayoutPass(ValidationState_t& _,
                              const spv_parsed_instruction_t* inst) {
  const SpvOp opcode = static_cast<SpvOp>(inst->opcode);

  switch (_.getLayoutSection()) {
    case kLayoutCapabilities:
    case kLayoutExtensions:
    case kLayoutExtInstImport:
    case kLayoutMemoryModel:
    case kLayoutEntryPoint:
    case kLayoutExecutionMode:
    case kLayoutDebug1:
    case kLayoutDebug2:
    case kLayoutAnnotations:
    case kLayoutTypes:
      spvCheckReturn(ModuleScopedInstructions(_, inst, opcode));
      break;
    case kLayoutFunctionDeclarations:
    case kLayoutFunctionDefinitions:
      spvCheckReturn(FunctionScopedInstructions(_, inst, opcode));
      break;
  }  // switch(getLayoutSection())
  return SPV_SUCCESS;
}
}
