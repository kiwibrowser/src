#!/usr/bin/python
#
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""
General routines to help generate actual/baseline decoders.
"""

import dgen_core
import dgen_output

# The list of rebuild decoder functions that will rebuild constructs
# whenever a decoder changes. Format is 'lambda decoder:'.
REBUILD_DECODER_CACHE_FUNCTIONS = set()

def RebuildDecoderCaches(decoder):
  """Rebuild caches defined for the decoder."""
  for fn in REBUILD_DECODER_CACHE_FUNCTIONS:
    fn(decoder)

def commented_decoder_repr(decoder):
  """Returns a C++ comment describing the neutral representation of the
     given decoder."""
  return dgen_output.commented_string(
      repr(decoder).replace('= ', ' ', 1))

def commented_decoder_neutral_repr(decoder):
  """Returns a C++ comment describing the neutral representation of the
     given decoder."""
  return dgen_output.commented_string(
      decoder.neutral_repr().replace('= ', ' ', 1))

def AddImplicitMethodsToDecoder(decoder):
  """Adds implicit methods to actions in decoder, if applicable."""
  return decoder.table_filter(AddImplicitMethodsToTable)

def AddImplicitMethodsToTable(table):
  """Adds implicit methods to actions in table, if applicable."""
  return table.row_filter(AddImplicitMethodsToRow)

def AddImplicitMethodsToRow(row):
  """Adds implicit methods to actions in row, if applicable."""
  action = row.action
  if (isinstance(action, dgen_core.DecoderAction) and
      ActionDefinesDecoder(action)):
    action = action.copy()
    for installer_fcn in _IMPLICIT_METHODS_INSTALLERS:
      installer_fcn(action)
    return dgen_core.Row(list(row.patterns), action)
  else:
    return row

def ActionDefinesDecoder(action):
  """Returns whether the decoder action defines a decoder that should
     be automatically generated.
     """
  return (isinstance(action, dgen_core.DecoderAction) and
          any(_IsMethodDefined(action, m) for m in METHODS))

def BaselineToActual(baseline):
  """Given the baseline action, return the corresponding actual."""
  if not ActionDefinesDecoder(baseline): return None
  return baseline.action_filter(
      [m for m in METHODS if _IsMethodDefined(baseline, m)])

def BaselineName(baseline):
  """Given the baseline action, returns the representative name that
     should be used for the baseline.

     *WARNING*: BaselineName's are not unique. It's the responsibility
     of the caller to add uniqueness.
     """
  name = baseline.find('rule')
  if name == None: name = 'Unnamed'
  pattern = baseline.find('pattern')
  if pattern:
    name = name + "_" + pattern
  return name

def BaselineNameAndBaseline(baseline):
  """Returns a two-tuple of the baseline name, and the baseline action.
     Used to sort baseline classes."""
  return (BaselineName(baseline), baseline)

"""Defines a map from virtual field, to the corresponding declare/define
   functions.
   """
_METHODS_MAP = {}  # Note: filled as declare/define functions are defined.

"""Defines optional implementations for a method if a field isn't defined."""
_OPTIONAL_METHODS_MAP = {}  # Note: filled as declare/define
                            # functions are defined.

"""Defines the set of implicit method installer functions that should be
   applied. These functions work on decoder actions, and add implicit
   methods that are defined by the values of other methods."""
_IMPLICIT_METHODS_INSTALLERS = []

"""Returns the declare function to use for the given (named) method."""
def DeclareMethodFcn(method):
  return _METHODS_MAP.get(method)[0]

"""Returns the define funcdtion to use for the given (named) method."""
def DefineMethodFcn(method):
  return _METHODS_MAP.get(method)[1]


DECODER_CLASS_HEADER="""
class %(decoder_name)s
     : public ClassDecoder {
 public:
  %(decoder_name)s()
     : ClassDecoder() {}"""

DECODER_CLASS_FOOTER="""
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      %(decoder_name)s);
};
"""

def DeclareDecoder(action, decoder_name, out):
  """Generates the c++ class declaration for the given action.

     decoder_name: The name for the C++ decoder class.
     out: The output stream to write the class declation to."""
  values = {'decoder_name': decoder_name}
  out.write(DECODER_CLASS_HEADER % values)
  for method in METHODS:
    if _IsMethodDefined(action, method):
      DeclareMethodFcn(method)(out, values)
  out.write(DECODER_CLASS_FOOTER % values)

def DefineDecoder(action, decoder_name, out):
  """Generates the C++ class definition for the given action.

     action: The decoder action to define.
     decoder_name: The name for the C++ decoder class.
     out: The output stream to write the class declation to.
  """

  values = {'decoder_name': decoder_name}
  for method in METHODS:
    if _IsMethodDefined(action, method):
      method_body = _FindMethodBody(action, method)
      values['neutral_rep'] = (
          '%s: %s' % (method, dgen_output.commented_string(
              repr(dgen_core.neutral_repr(method_body)), '  ')))
      DefineMethodFcn(method)(out, method_body, values)

def _IsMethodDefined(action, field):
  """Returns true if the field defines the corresponding method."""
  defn = _FindMethodBody(action, field)
  return defn != None

def _FindMethodBody(action, method):
  body = action.find(method)
  if not body:
    body = _OPTIONAL_METHODS_MAP.get(method)
  return body

def _DefineMethod(out, format, values, method_exp):
  values['method_exp'] = method_exp
  out.write(format % values)

DECODER_BASE_HEADER="""
  virtual Register base_address_register(Instruction i) const;"""

DECODER_BASE_DEF="""
Register %(decoder_name)s::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareBase(out, values):
  out.write(DECODER_BASE_HEADER % values)

def _DefineBase(out, base, values):
  _DefineMethod(out, DECODER_BASE_DEF, values, base.to_register())

_METHODS_MAP['base'] = [_DeclareBase, _DefineBase]
dgen_core.DefineDecoderFieldType('base', 'register')

DECODER_CLEARS_BITS_HEADER="""
  virtual bool clears_bits(Instruction i, uint32_t clears_mask) const;"""

DECODER_CLEARS_BITS_DEF="""
bool %(decoder_name)s::
clears_bits(Instruction inst, uint32_t clears_mask) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareClearsBits(out, values):
  out.write(DECODER_CLEARS_BITS_HEADER % values)

def _DefineClearsBits(out, clear_bits, values):
  dgen_core.InstallParameter('clears_mask', 'uint32')
  _DefineMethod(out, DECODER_CLEARS_BITS_DEF, values, clear_bits.to_bool())
  dgen_core.UninstallParameter('clears_mask')

_METHODS_MAP['clears_bits'] = [_DeclareClearsBits, _DefineClearsBits]
dgen_core.DefineDecoderFieldType('clears_mask', 'bool')

DECODER_DEFS_HEADER="""
  virtual RegisterList defs(Instruction inst) const;"""

DECODER_DEFS_DEF="""
RegisterList %(decoder_name)s::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareDefs(out, values):
  out.write(DECODER_DEFS_HEADER % values)

def _DefineDefs(out, defs, values):
  _DefineMethod(out, DECODER_DEFS_DEF, values, defs.to_register_list())

_METHODS_MAP['defs'] = [_DeclareDefs, _DefineDefs]
dgen_core.DefineDecoderFieldType('defs', 'register_list')

DECODER_CODE_REPLACE_HEADER="""
  virtual Instruction dynamic_code_replacement_sentinel(Instruction i) const;"""

DECODER_CODE_REPLACE_DEF_HEADER="""
Instruction %(decoder_name)s::
dynamic_code_replacement_sentinel(
     Instruction inst) const {
  if (!defs(inst).ContainsAny(RegisterList::DynCodeReplaceFrozenRegs())) {"""

DECODER_CODE_REPLACE_DEF_UPDATE="""
    // %(neutral_rep)s
    inst.SetBits(%(hi_bit)s, %(lo_bit)s, 0);"""

DECODER_CODE_REPLACE_DEF_FOOTER="""
  }
  return inst;
}
"""

def _DeclareCodeReplace(out, values):
  out.write(DECODER_CODE_REPLACE_HEADER % values)

def _DefineCodeReplace(out, dyn_code, values):
  _DefineMethod(out, DECODER_CODE_REPLACE_DEF_HEADER, values, '???')
  for v in dyn_code.args():
    bitfield = v.to_bitfield(v)
    values['neutral_rep'] = dgen_output.commented_string(
        bitfield.neutral_repr(), '  ')
    values['hi_bit'] = bitfield.hi()
    values['lo_bit'] = bitfield.lo()
    out.write(DECODER_CODE_REPLACE_DEF_UPDATE % values)
  out.write(DECODER_CODE_REPLACE_DEF_FOOTER % values)

_METHODS_MAP['dynamic_code_replace_immediates'] = [
    _DeclareCodeReplace, _DefineCodeReplace]
dgen_core.DefineDecoderFieldType('dynamic_code_replace_immediates', 'bitfield')

DECODER_IS_LITERAL_LOAD_HEADER="""
  virtual bool is_literal_load(Instruction i) const;"""

DECODER_IS_LITERAL_LOAD_DEF="""
bool %(decoder_name)s::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareIsLiteralLoad(out, values):
  out.write(DECODER_IS_LITERAL_LOAD_HEADER % values)

def _DefineIsLiteralLoad(out, is_literal_load, values):
  _DefineMethod(out, DECODER_IS_LITERAL_LOAD_DEF, values,
                is_literal_load.to_bool())

_METHODS_MAP['is_literal_load'] = [_DeclareIsLiteralLoad,
                                   _DefineIsLiteralLoad]
dgen_core.DefineDecoderFieldType('is_literal_load', 'bool')

DECODER_IS_LOAD_TP_HEADER="""
  virtual bool is_load_thread_address_pointer(Instruction i) const;"""

DECODER_IS_LOAD_TP_DEF="""
bool %(decoder_name)s::
is_load_thread_address_pointer(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareIsLoadTp(out, values):
  out.write(DECODER_IS_LOAD_TP_HEADER % values)

def _DefineIsLoadTp(out, is_load_tp, values):
  _DefineMethod(out, DECODER_IS_LOAD_TP_DEF, values, is_load_tp.to_bool())

_METHODS_MAP['is_load_tp'] = [_DeclareIsLoadTp, _DefineIsLoadTp]
dgen_core.DefineDecoderFieldType('is_load_tp', 'bool')

DECODER_POOL_HEAD_HEADER="""
  virtual bool is_literal_pool_head(Instruction i) const;"""

DECODER_POOL_HEAD_DEF="""
bool %(decoder_name)s::
is_literal_pool_head(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclarePoolHead(out, values):
  out.write(DECODER_POOL_HEAD_HEADER % values)

def _DefinePoolHead(out, pool_head, values):
  _DefineMethod(out, DECODER_POOL_HEAD_DEF, values, pool_head.to_bool())

_METHODS_MAP['is_literal_pool_head'] = [_DeclarePoolHead, _DefinePoolHead]
dgen_core.DefineDecoderFieldType('is_literal_pool_head', 'bool')

DECODER_RELATIVE_HEADER="""
  virtual bool is_relative_branch(Instruction i) const;"""

DECODER_RELATIVE_DEF="""
bool %(decoder_name)s::
is_relative_branch(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareRelative(out, values):
  out.write(DECODER_RELATIVE_HEADER % values)

def _DefineRelative(out, relative, values):
  _DefineMethod(out, DECODER_RELATIVE_DEF, values, relative.to_bool())

_METHODS_MAP['relative'] = [_DeclareRelative, _DefineRelative]
dgen_core.DefineDecoderFieldType('relative', 'bool')

DECODER_RELATIVE_OFFSET_HEADER="""
  virtual int32_t branch_target_offset(Instruction i) const;"""

DECODER_RELATIVE_OFFSET_DEF="""
int32_t %(decoder_name)s::
branch_target_offset(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareRelativeOffset(out, values):
  out.write(DECODER_RELATIVE_OFFSET_HEADER % values)

def _DefineRelativeOffset(out, relative_offset, values):
  _DefineMethod(out, DECODER_RELATIVE_OFFSET_DEF, values,
                relative_offset.to_uint32())

_METHODS_MAP['relative_offset'] = [_DeclareRelativeOffset,
                                   _DefineRelativeOffset]
dgen_core.DefineDecoderFieldType('relative_offset', 'uint32')

DECODER_SAFETY_HEADER="""
  virtual SafetyLevel safety(Instruction i) const;"""

DECODER_SAFETY_DEF_HEADER="""
SafetyLevel %(decoder_name)s::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
"""

DECODER_SAFETY_DEF_CHECK="""
  // %(neutral_rep)s
  if (%(safety_test)s)
    return %(safety_action)s;
"""

DECODER_SAFETY_DEF_FOOTER="""
  return MAY_BE_SAFE;
}

"""
def _DeclareSafety(out, values):
  out.write(DECODER_SAFETY_HEADER % values)

def _TypeCheckSafety(safety):
  """Type checks a list of SafetyAction's."""
  for s in safety:
    s.test().to_bool()

def _DefineSafety(out, safety, values):
  # Use baseline to implement action if defined.
  _DefineMethod(out, DECODER_SAFETY_DEF_HEADER, values, '???')
  for s in safety:
    values['neutral_rep'] = dgen_output.commented_string(
        '%s' % s.neutral_repr(), '  ')
    values['safety_test'] = s.test().to_bool()
    values['safety_action'] = s.action()
    out.write(DECODER_SAFETY_DEF_CHECK % values)
  out.write(DECODER_SAFETY_DEF_FOOTER % values)

_METHODS_MAP['safety'] = [_DeclareSafety, _DefineSafety]
_OPTIONAL_METHODS_MAP['safety'] = []
dgen_core.DefineDecoderFieldType('safety', _TypeCheckSafety)

DECODER_SETS_Z_IF_CLEAR_HEADER="""
  virtual bool sets_Z_if_bits_clear(Instruction i,
                                    Register test_register,
                                    uint32_t clears_mask) const;"""

DECODER_SETS_Z_IF_CLEAR_DEF="""
bool %(decoder_name)s::
sets_Z_if_bits_clear(
      Instruction inst, Register test_register,
      uint32_t clears_mask) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareSetsZIfClearBits(out, values):
  out.write(DECODER_SETS_Z_IF_CLEAR_HEADER)

def _DefineSetsZIfClearBits(out, sets_z, values):
  dgen_core.InstallParameter('clears_mask', 'uint32')
  dgen_core.InstallParameter('test_register', 'uint32')
  _DefineMethod(out, DECODER_SETS_Z_IF_CLEAR_DEF, values, sets_z.to_bool())
  dgen_core.UninstallParameter('clears_mask')
  dgen_core.UninstallParameter('test_register')

_METHODS_MAP['sets_Z_if_clear_bits'] = [
    _DeclareSetsZIfClearBits, _DefineSetsZIfClearBits]
dgen_core.DefineDecoderFieldType('sets_Z_if_clear_bits', 'bool')

DECODER_SMALL_IMM_BASE_WB_HEADER="""
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;"""

DECODER_SMALL_IMM_BASE_WB_DEF="""
bool %(decoder_name)s::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareSmallImmBaseWb(out, values):
  out.write(DECODER_SMALL_IMM_BASE_WB_HEADER % values)

def _DefineSmallImmBaseWb(out, small_base_wb, values):
  _DefineMethod(out, DECODER_SMALL_IMM_BASE_WB_DEF, values,
                small_base_wb.to_bool())

_METHODS_MAP['small_imm_base_wb'] = [_DeclareSmallImmBaseWb,
                                     _DefineSmallImmBaseWb]
dgen_core.DefineDecoderFieldType('small_imm_base_wb', 'bool')

DECODER_TARGET_HEADER="""
  virtual Register branch_target_register(Instruction i) const;"""

DECODER_TARGET_DEF="""
Register %(decoder_name)s::
branch_target_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareTarget(out, values):
  out.write(DECODER_TARGET_HEADER % values)

def _DefineTarget(out, target, values):
  _DefineMethod(out, DECODER_TARGET_DEF, values, target.to_register())

_METHODS_MAP['target'] = [_DeclareTarget, _DefineTarget]
dgen_core.DefineDecoderFieldType('target', 'register')

DECODER_USES_HEADER="""
  virtual RegisterList uses(Instruction i) const;"""

DECODER_USES_DEF="""
RegisterList %(decoder_name)s::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // %(neutral_rep)s
  return %(method_exp)s;
}
"""

def _DeclareUses(out, values):
  out.write(DECODER_USES_HEADER % values)

def _DefineUses(out, uses, values):
  _DefineMethod(out, DECODER_USES_DEF, values, uses.to_register_list())

_METHODS_MAP['uses'] = [_DeclareUses, _DefineUses]
dgen_core.DefineDecoderFieldType('uses', 'register_list')



def _ImplicitViolationsInstaller(action):
  """Adds implicit definition for method violations, if applicable."""

  # Get current definition of violations, modify, and then put updated
  # version back.
  violations = action.find('violations')
  action.remove('violations')
  methods = []
  if violations == None:
    violations = []
  else:
    assert isinstance(violations, list)

  if violations:
    # Copy to 'diagnostics', so that both get_violations and
    # 'generate_diagnostics' methods are generated.
    action.define('diagnostics', violations)

  # If action (possibly) defines constant pool header, get_violations
  # should include code to skip over the constant pool.
  if action.find('is_literal_pool_head') != None:
    methods.append('is_literal_pool_head')

  # If action (possibly) defines a branch target, get_violations should
  # include code to check if branch target is properly masked.
  # May also add code if a call is bundle aligned (if Lr is defined).
  if action.find('target') != None:
    methods.append('target')

  # If action (possibly) defines a base, get violations should check
  # if a load/store violation occurs.
  if action.find('base') != None:
    methods.append('base')

  # If action (possibly) defines a relative (direct) branch, get_violations
  # should include code to check if call is bundle aligned.
  if action.find('relative') != None:
    methods.append('relative')

  if methods:
    implicit = dgen_core.Implicit(methods)
    violations.append(implicit)

  if violations:
    action.define('violations', violations)

_IMPLICIT_METHODS_INSTALLERS.append(_ImplicitViolationsInstaller)

VIOLATIONS_DECLARE="""
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;"""

VIOLATIONS_HEADER="""
ViolationSet %(decoder_name)s::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);"""

VIOLATIONS_SUBHEADER="""
  const Instruction& inst = second.inst();
"""

VIOLATIONS_CHECK="""
  // %(neutral_rep)s
  if (%(violation_test)s)
     violations = ViolationUnion(violations, ViolationBit(OTHER_VIOLATION));
"""

VIOLATIONS_POOL="""

  // If a pool head, mark address appropriately and then skip over
  // the constant bundle.
  validate_literal_pool_head(second, sfi, critical, next_inst_addr);
"""

VIOLATIONS_TARGET="""

  // Indirect branches (through a target register) need to be masked,
  // and if they represent a call, they need to be end-of-bundle aligned.
  violations = ViolationUnion(
      violations, get_branch_mask_violations(first, second, sfi, critical));
  violations = ViolationUnion(
      violations, get_call_position_violations(second, sfi));
"""

VIOLATIONS_RELATIVE="""

  // Direct (relative) branches can represent a call. If so, they need
  // to be end-of-bundle aligned.
  violations = ViolationUnion(
      violations, get_call_position_violations(second, sfi));
"""

VIOLATIONS_BASE="""

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));
"""


VIOLATIONS_FOOTER="""
  return violations;
}

"""

def _DeclareViolations(out, values):
  out.write(VIOLATIONS_DECLARE % values)

def _TypeCheckViolations(violations):
  if isinstance(violations, dgen_core.Implicit): return
  for v in violations:
    if not isinstance(v, dgen_core.Violation): continue
    v.test().to_bool()
    v.print_args()[0].to_cstring()
    for a in v.print_args()[1:]:
      a.to_uint32()

def _DefineViolations(out, violations, values):
  out.write(VIOLATIONS_HEADER % values)
  is_first_violation = True
  for v in violations:
    if isinstance(v, dgen_core.Implicit):
      if 'is_literal_pool_head' in v.methods():
        out.write(VIOLATIONS_POOL % values)
      if 'target' in v.methods():
        out.write(VIOLATIONS_TARGET % values)
      if 'relative' in v.methods():
        out.write(VIOLATIONS_RELATIVE % values)
      if 'base' in v.methods():
        out.write(VIOLATIONS_BASE % values)
      continue

    assert isinstance(v, dgen_core.Violation)
    if is_first_violation:
      is_first_violation = False
      out.write(VIOLATIONS_SUBHEADER % values)
    values['neutral_rep'] = dgen_output.commented_string(
        '%s' % v.neutral_repr(), '  ')
    values['violation_test'] = v.test().to_bool()
    out.write(VIOLATIONS_CHECK % values)
  out.write(VIOLATIONS_FOOTER % values)

_METHODS_MAP['violations'] = [_DeclareViolations, _DefineViolations]
dgen_core.DefineDecoderFieldType('violations', _TypeCheckViolations)

DIAGNOSTICS_DECLARE="""
  virtual void generate_diagnostics(
      ViolationSet violations,
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::ProblemSink* out) const;"""

DIAGNOSTICS_HEADER="""
void %(decoder_name)s::
generate_diagnostics(ViolationSet violations,
                     const nacl_arm_val::DecodedInstruction& first,
                     const nacl_arm_val::DecodedInstruction& second,
                     const nacl_arm_val::SfiValidator& sfi,
                     nacl_arm_val::ProblemSink* out) const {
  ClassDecoder::generate_diagnostics(violations, first, second, sfi, out);
  const Instruction& inst = second.inst();
  if (ContainsViolation(violations, OTHER_VIOLATION)) {
"""

DIAGNOSTICS_ADD_HEADER="""
    // %(neutral_rep)s
    if (%(violation_test)s) {
      out->ReportProblemDiagnostic(OTHER_VIOLATION, second.addr()"""

DIAGNOSTICS_ADD_PRINTF_ARG=""",
         %(printf_arg)s"""

DIAGNOSTICS_ADD_FOOTER=""");
    }
"""

DIAGNOSTICS_FOOTER="""
  }
}

"""

def _DeclareDiagnostics(out, values):
  out.write(DIAGNOSTICS_DECLARE % values)

def _DefineDiagnostics(out, violations, values):
  out.write(DIAGNOSTICS_HEADER % values)
  for v in violations:
    if not isinstance(v, dgen_core.Violation): continue

    values['neutral_rep'] = dgen_output.commented_string(
        '%s' % v.neutral_repr(), '    ')
    values['violation_test'] = v.test().to_bool()
    out.write(DIAGNOSTICS_ADD_HEADER % values)
    is_first = True
    for arg in v.print_args():
      if is_first:
        values['printf_arg'] = arg.to_cstring()
        is_first = False
      else:
        values['printf_arg'] = arg.to_uint32()
      out.write(DIAGNOSTICS_ADD_PRINTF_ARG % values)
    out.write(DIAGNOSTICS_ADD_FOOTER % values)
  out.write(DIAGNOSTICS_FOOTER % values)

_METHODS_MAP['diagnostics'] = [_DeclareDiagnostics, _DefineDiagnostics]

"""Defines the set of method fields."""
METHODS = sorted(set(_METHODS_MAP.keys() + _OPTIONAL_METHODS_MAP.keys()))
