#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Global environment and expression parsing for the PNaCl driver


# This dictionary initializes a shell-like environment.
# Shell escaping and ${} substitution are provided.
# See "class Environment" defined later for the implementation.

from driver_log import Log, DriverExit
from shelltools import shell
import types

INITIAL_ENV = {
  # Set by DriverMain
  'DRIVER_PATH'     : '', # Absolute path to this driver invocation
  'DRIVER_BIN'      : '', # PNaCl driver bin/ directory
  'DRIVER_REV_FILE' : '${BASE}/REV',

  'BASE_NACL'       : '${@FindBaseNaCl}',      # Absolute path of native_client/
  'BASE_TOOLCHAIN'  : '${@FindBaseToolchain}', # Absolute path to toolchain/OS_ARCH/
  'BASE'            : '${@FindBasePNaCl}',     # Absolute path to PNaCl
  'BUILD_OS'        : '${@GetBuildOS}',        # "linux", "nacl", "darwin"
                                               # or "windows"
  'BUILD_ARCH'      : '${@GetBuildArch}',      # "x86_64" or "i686" or "i386"

  # Directories
  'CLANG_VER'       : '3.7.0', # Included in path to compiler-owned libs/headers
  'BPREFIXES'       : '', # Prefixes specified using the -B flag.
  'BASE_LLVM'       : '${@FindBaseHost:clang}',
  'BASE_BINUTILS'   : '${@FindBaseHost:arm-nacl-ar}',

  'BASE_LIB_NATIVE' : '${BASE}/translator/',

  'BASE_USR'        : '${BASE}/le32-nacl',
  'BASE_SDK'        : '${BASE}/sdk',
  'BASE_LIB'        : '${BASE}/lib/clang/${CLANG_VER}/lib/le32-nacl',
  'BASE_USR_ARCH'   : '${BASE_USR_%BCLIB_ARCH%}',
  'BASE_USR_X8632'  : '${BASE}/i686_bc-nacl',
  'BASE_USR_X8664'  : '${BASE}/x86_64_bc-nacl',
  'BASE_USR_ARM'    : '${BASE}/arm_bc-nacl',
  'BASE_LIB_ARCH'   : '${BASE_LIB_%BCLIB_ARCH%}',
  'BASE_LIB_X8632'  : '${BASE}/lib/clang/${CLANG_VER}/lib/i686_bc-nacl',
  'BASE_LIB_X8664'  : '${BASE}/lib/clang/${CLANG_VER}/lib/x86_64_bc-nacl',
  'BASE_LIB_ARM'    : '${BASE}/lib/clang/${CLANG_VER}/lib/arm_bc-nacl',

  'LIBS_NATIVE_ARCH' : '${LIBS_NATIVE_%ARCH%}',
  'LIBS_NATIVE_ARM' : '${BASE_LIB_NATIVE}arm/lib',
  'LIBS_NATIVE_ARM_NONSFI' : '${BASE_LIB_NATIVE}arm-nonsfi/lib',
  'LIBS_NATIVE_X8632' : '${BASE_LIB_NATIVE}x86-32/lib',
  'LIBS_NATIVE_X8632_NONSFI' : '${BASE_LIB_NATIVE}x86-32-nonsfi/lib',
  'LIBS_NATIVE_X8664' : '${BASE_LIB_NATIVE}x86-64/lib',
  'LIBS_NATIVE_MIPS32' : '${BASE_LIB_NATIVE}mips32/lib',

  'BASE_LLVM_BIN'   : '${BASE_LLVM}/bin',
  'TRANSLATOR_BIN'  :
    '${BASE_TOOLCHAIN}/pnacl_translator/translator/${TRANSLATOR_ARCH}/bin',

  # TODO(dschuff): Switch these directories to be triple-style arches,
  # to match the main toolchain?
  'TRANSLATOR_ARCH'       : '${TRANSLATOR_ARCH_%ARCH%}',
  'TRANSLATOR_ARCH_X8632' : 'x86-32',
  'TRANSLATOR_ARCH_X8664' : 'x86-64',
  'TRANSLATOR_ARCH_ARM'   : 'arm',
  'TRANSLATOR_ARCH_MIPS32': 'mips32',

  'SCONS_OUT'       : '${BASE_NACL}/scons-out',

  # Driver settings
  'ARCH'        : '',     # Target architecture, including optional
                          # suffixes such as '_NONSFI' or '_LINUX'.
  'BASE_ARCH'   : '',     # Target architecture without any '_NONSFI' suffix.
                          # Derived from ARCH field.
  'NONSFI_NACL' : '0',    # Whether targeting Non-SFI Mode.  Derived from
                          # ARCH field.
  'BIAS'        : 'NONE', # This can be 'NONE', 'ARM', 'MIPS32', 'X8632' or
                          # 'X8664'.
                          # When not set to none, this causes the front-end to
                          # act like a target-specific compiler. This bias is
                          # currently needed while compiling newlib,
                          # and some scons tests.
  'DRY_RUN'     : '0',
  'SAVE_TEMPS'  : '0',    # Do not clean up temporary files
  'SANDBOXED'   : '0',    # Use sandboxed toolchain for this arch. (main switch)
  'HAS_FRONTEND': '',     # Set by ReadConfig().  '1' if the driver install
                          # has support for front-end bitcode tools, or '0'
                          # if it only has the backend translator.

  'USE_EMULATOR'        : '0',
  # Args passed from one driver invocation to another
  'INHERITED_DRIVER_ARGS' : '',

  'BCLIB_ARCH'          : '',
  # Logging settings
  'LOG_VERBOSE'        : '0', # Log to stdout (--pnacl-driver-verbose)

  # Conventions
  'SO_EXT'          : '${SO_EXT_%BUILD_OS%}',
  'SO_EXT_darwin'   : '.dylib',
  'SO_EXT_linux'    : '.so',
  'SO_EXT_nacl'     : '.so',
  'SO_EXT_windows'  : '.dll',

  'SO_DIR'          : '${SO_DIR_%BUILD_OS%}',
  'SO_DIR_darwin'   : 'lib',
  'SO_DIR_linux'    : 'lib',
  'SO_DIR_nacl'     : 'lib',
  'SO_DIR_windows'  : 'bin',  # On Windows, DLLs are placed in bin/
                              # because the dynamic loader searches %PATH%

  'EXEC_EXT'        : '${EXEC_EXT_%BUILD_OS%}',
  'EXEC_EXT_darwin' : '',
  'EXEC_EXT_linux'  : '',
  'EXEC_EXT_nacl'   : '',
  'EXEC_EXT_windows': '.exe',

  'SCONS_OS'            : '${SCONS_OS_%BUILD_OS%}',
  'SCONS_OS_linux'      : 'linux',
  'SCONS_OS_nacl'       : 'nacl',
  'SCONS_OS_darwin'     : 'mac',
  'SCONS_OS_windows'    : 'win',

  # llvm goldplugin
  'GOLD_PLUGIN_SO'  : '${BASE_LLVM}/${SO_DIR}/LLVMgold${SO_EXT}',

  'SCONS_STAGING'       : '${SCONS_STAGING_%ARCH%}',
  'SCONS_STAGING_X8632' : '${SCONS_OUT}/opt-${SCONS_OS}-x86-32/staging',
  'SCONS_STAGING_X8664' : '${SCONS_OUT}/opt-${SCONS_OS}-x86-64/staging',
  'SCONS_STAGING_ARM'   : '${SCONS_OUT}/opt-${SCONS_OS}-arm/staging',
  'SCONS_STAGING_MIPS32': '${SCONS_OUT}/opt-${SCONS_OS}-mips32/staging',

  'SEL_LDR_PREFIX'      : '${USE_EMULATOR ? ${EMULATOR}}',
  # NOTE: -Q skips sel_ldr qualification tests, -c -c skips validation
  'SEL_LDR_FLAGS'       : '-B ${IRT_BLOB} ' +
                          '${USE_EMULATOR ? -Q -c -c}',

  'IRT_STAGING'         : '${IRT_STAGING_%ARCH%}',
  'IRT_STAGING_X8632'   : '${SCONS_OUT}/nacl_irt-x86-32/staging',
  'IRT_STAGING_X8664'   : '${SCONS_OUT}/nacl_irt-x86-64/staging',
  'IRT_STAGING_ARM'     : '${SCONS_OUT}/nacl_irt-arm/staging',
  'IRT_STAGING_MIPS32'  : '${SCONS_OUT}/nacl_irt-mips32/staging',
  'IRT_BLOB'            : '${IRT_STAGING}/irt_core.nexe',

  'EMULATOR'            : '${EMULATOR_%ARCH%}',
  'EMULATOR_X8632'      : '',
  'EMULATOR_X8664'      : '',
  # NOTE: this is currently the only dependency on the arm trusted TC
  'EMULATOR_ARM'        :
      '${BASE_NACL}/toolchain/linux_x86/arm_trusted/run_under_qemu_arm',
  'EMULATOR_MIPS32'     :
      '${BASE_NACL}/toolchain/linux_x86/mips_trusted/run_under_qemu_mips32',

  'SEL_LDR'       : '${SCONS_STAGING}/sel_ldr${EXEC_EXT}',
  'BOOTSTRAP_LDR' : '${SCONS_STAGING}/nacl_helper_bootstrap${EXEC_EXT}',

  # sandboxed LLVM backend
  'LLC_SB'      : '${TRANSLATOR_BIN}/pnacl-llc.nexe',
  # sandboxed linker (gold based)
  'LD_SB'       : '${TRANSLATOR_BIN}/ld.nexe',
  # sandboxed Subzero backend
  'PNACL_SZ_SB'      : '${TRANSLATOR_BIN}/pnacl-sz.nexe',

  # Bitcode LLVM tools
  'CLANG'         : '${BASE_LLVM_BIN}/clang${EXEC_EXT}',
  # 'clang++' doesn't work on Windows (outside of Cygwin),
  # because it is a symlink.
  'CLANGXX'       : '${BASE_LLVM_BIN}/clang${EXEC_EXT} --driver-mode=g++',
  'LLVM_OPT'      : '${BASE_LLVM_BIN}/opt${EXEC_EXT}',
  'LLVM_DIS'      : '${BASE_LLVM_BIN}/llvm-dis${EXEC_EXT}',
  'LLVM_NM'       : '${BASE_LLVM_BIN}/llvm-nm${EXEC_EXT}',
  # llvm-as compiles llvm assembly (.ll) to bitcode (.bc/.po)
  'LLVM_AS'       : '${BASE_LLVM_BIN}/llvm-as${EXEC_EXT}',
  'PNACL_ABICHECK': '${BASE_LLVM_BIN}/pnacl-abicheck${EXEC_EXT}',
  'PNACL_COMPRESS': '${BASE_LLVM_BIN}/pnacl-bccompress${EXEC_EXT}',

  # Native LLVM tools
  'LLVM_PNACL_LLC': '${BASE_LLVM_BIN}/pnacl-llc${EXEC_EXT}',
  'LLVM_PNACL_SZ':  '${BASE_LLVM_BIN}/pnacl-sz${EXEC_EXT}',
  # llvm-mc is llvm's native assembler
  'LLVM_MC'       : '${BASE_LLVM_BIN}/llvm-mc${EXEC_EXT}',

  # Binutils
  'BINUTILS_BASE'  : '${BASE_BINUTILS}/bin/arm-nacl-',
  'OBJDUMP'        : '${BINUTILS_BASE}objdump${EXEC_EXT}',
  'NM'             : '${BINUTILS_BASE}nm${EXEC_EXT}',
  'AR'             : '${BINUTILS_BASE}ar${EXEC_EXT}',
  'RANLIB'         : '${BINUTILS_BASE}ranlib${EXEC_EXT}',
  'READELF'        : '${BINUTILS_BASE}readelf${EXEC_EXT}',
  'STRIP'          : '${BINUTILS_BASE}strip${EXEC_EXT}',
  # linker (used for both bitcode and ELF linking)
  'LD'        : '${BINUTILS_BASE}ld.gold${EXEC_EXT}',
}

######################################################################
#
# Environment
#
######################################################################

def ParseError(s, leftpos, rightpos, msg):
  Log.Error("Parse Error: %s", msg)
  Log.Error('  ' + s)
  Log.Error('  ' + (' '*leftpos) + ('^'*(rightpos - leftpos + 1)))
  DriverExit(1)

# Find the leftmost position in "s" which begins a substring
# in "strset", starting at "pos".
# For example:
#   FindFirst('hello world', 0, ['h','o']) = ('h', 0)
#   FindFirst('hello world', 1, ['h','o']) = ('o', 4)
#   FindFirst('hello world', 0, ['x']) = (None,11)
def FindFirst(s, pos, strset):
  m = {}
  for ss in strset:
    m[s.find(ss, pos)] = ss
  if -1 in m:
    del m[-1]
  if len(m) == 0:
    return (None, len(s))
  pos = min(m)
  return (m[pos], pos)

class Environment(object):
  functions = {}

  @classmethod
  def register(cls, func):
    """ Register a function for use in the evaluator """
    cls.functions[func.__name__] = func
    return func

  def __init__(self):
    self.stack = []
    self.reset()

  def reset(self):
    self.data = dict(INITIAL_ENV)

  def update(self, extra):
    self.data.update(extra)

  def dump(self):
    for (k,v) in self.data.iteritems():
      print '%s == %s' % (k,v)

  def push(self):
    self.stack.append(self.data)
    self.data = dict(self.data) # Make a copy

  def pop(self):
    self.data = self.stack.pop()

  def has(self, varname):
    return varname in self.data

  def getraw(self, varname):
    return self.eval(self.data[varname])

  # Evaluate a variable from the environment.
  # Returns a list of terms.
  def get(self, varname):
    return shell.split(self.getraw(varname))

  # Retrieve a variable from the environment which
  # is a single term. Returns a string.
  def getone(self, varname):
    return shell.unescape(self.getraw(varname))

  def getbool(self, varname):
    return bool(int(self.getone(varname)))

  def setbool(self, varname, val):
    if val:
      self.set(varname, '1')
    else:
      self.set(varname, '0')

  # Set a variable in the environment without shell-escape
  def setraw(self, varname, val):
    self.data[varname] = val

  # Set one or more variables using named arguments
  def setmany(self, **kwargs):
    for k,v in kwargs.iteritems():
      if isinstance(v, types.StringTypes):
        self.set(k, v)
      elif isinstance(v, types.ListType):
        self.set(k, *v)
      else:
        Log.Fatal('env.setmany given a non-string and non-list value')

  def clear(self, varname):
    self.data[varname] = ''

  # Set a variable to one or more terms, applying shell-escape.
  def set(self, varname, *vals):
    self.clear(varname)
    self.append(varname, *vals)

  # Append one or more terms to a variable in the
  # environment, applying shell-escape.
  def append(self, varname, *vals):
    escaped = [ shell.escape(v) for v in vals ]
    if len(self.data[varname]) > 0:
      self.data[varname] += ' '
    self.data[varname] += ' '.join(escaped)

  # Evaluate an expression s
  def eval(self, s):
    (result, i) = self.eval_expr(s, 0, [])
    assert(i == len(s))
    return result

  ######################################################################
  # EXPRESSION EVALUATION CODE
  # Context Free Grammar:
  #
  # str = empty | string literal
  # expr = str | expr '$' '{' bracket_expr '}' expr
  # bracket_expr = varname | boolexpr ? expr | boolexpr ? expr : expr | @call
  # boolexpr = boolval | boolval '&&' boolexpr | boolval '||' boolexpr
  # boolval = varname | !varname | #varname | !#varname | varname '==' str
  # varname = str | varname '%' bracket_expr '%' varname
  # call = func | func ':' arglist
  # func = str
  # arglist = empty | arg ':' arglist
  #
  # Do not call these functions outside of this class.
  # The env.eval method is the external interface to the evaluator.
  ######################################################################

  # Evaluate a string literal
  def eval_str(self, s, pos, terminators):
    (_,i) = FindFirst(s, pos, terminators)
    return (s[pos:i], i)

  # Evaluate %var% substitutions inside a variable name.
  # Returns (the_actual_variable_name, endpos)
  # Terminated by } character
  def eval_varname(self, s, pos, terminators):
    (_,i) = FindFirst(s, pos, ['%'] + terminators)
    leftpart = s[pos:i].strip(' ')
    if i == len(s) or s[i] in terminators:
      return (leftpart, i)

    (middlepart, j) = self.eval_bracket_expr(s, i+1, ['%'])
    if j == len(s) or s[j] != '%':
      ParseError(s, i, j, "Unterminated %")

    (rightpart, k) = self.eval_varname(s, j+1, terminators)

    fullname = leftpart + middlepart + rightpart
    fullname = fullname.strip()
    return (fullname, k)

  # Absorb whitespace
  def eval_whitespace(self, s, pos):
    i = pos
    while i < len(s) and s[i] == ' ':
      i += 1
    return (None, i)

  def eval_bool_val(self, s, pos, terminators):
    (_,i) = self.eval_whitespace(s, pos)

    if s[i] == '!':
      negated = True
      i += 1
    else:
      negated = False

    (_,i) = self.eval_whitespace(s, i)

    if s[i] == '#':
      uselen = True
      i += 1
    else:
      uselen = False

    (varname, j) = self.eval_varname(s, i, ['=']+terminators)
    if j == len(s):
      # This is an error condition one level up. Don't evaluate anything.
      return (False, j)

    if varname not in self.data:
      ParseError(s, i, j, "Undefined variable '%s'" % varname)
    vardata = self.data[varname]

    contents = self.eval(vardata)

    if s[j] == '=':
      # String equality test
      if j+1 == len(s) or s[j+1] != '=':
        ParseError(s, j, j, "Unexpected token")
      if uselen:
        ParseError(s, j, j, "Cannot combine == and #")
      (_,j) = self.eval_whitespace(s, j+2)
      (literal_str,j) = self.eval_str(s, j, [' ']+terminators)
      (_,j) = self.eval_whitespace(s, j)
      if j == len(s):
        return (False, j) # Error one level up
    else:
      literal_str = None

    if uselen:
      val = (len(contents) != 0)
    elif literal_str is not None:
      val = (contents == literal_str)
    else:
      if contents not in ('0','1'):
        ParseError(s, j, j,
          "%s evaluated to %s, which is not a boolean!" % (varname, contents))
      val = bool(int(contents))
    return (negated ^ val, j)

  # Evaluate a boolexpr
  def eval_bool_expr(self, s, pos, terminators):
    (boolval1, i) = self.eval_bool_val(s, pos, ['&','|']+terminators)
    if i == len(s):
      # This is an error condition one level up. Don't evaluate anything.
      return (False, i)

    if s[i] in ('&','|'):
      # and/or expression
      if i+1 == len(s) or s[i+1] != s[i]:
        ParseError(s, i, i, "Unexpected token")
      is_and = (s[i] == '&')

      (boolval2, j) = self.eval_bool_expr(s, i+2, terminators)
      if j == len(s):
        # This is an error condition one level up.
        return (False, j)

      if is_and:
        return (boolval1 and boolval2, j)
      else:
        return (boolval1 or boolval2, j)

    return (boolval1, i)

  # Evaluate the inside of a ${} or %%.
  # Returns the (the_evaluated_string, endpos)
  def eval_bracket_expr(self, s, pos, terminators):
    (_,pos) = self.eval_whitespace(s, pos)

    if s[pos] == '@':
      # Function call: ${@func}
      # or possibly  : ${@func:arg1:arg2...}
      (_,i) = FindFirst(s, pos, [':']+terminators)
      if i == len(s):
        return ('', i) # Error one level up
      funcname = s[pos+1:i]

      if s[i] != ':':
        j = i
        args = []
      else:
        (_,j) = FindFirst(s, i+1, terminators)
        if j == len(s):
          return ('', j) # Error one level up
        args = s[i+1:j].split(':')

      val = self.functions[funcname](*args)
      contents = self.eval(val)
      return (contents, j)

    (m,_) = FindFirst(s, pos, ['?']+terminators)
    if m != '?':
      # Regular variable substitution
      (varname,i) = self.eval_varname(s, pos, terminators)
      if len(s) == i:
        return ('', i)  # Error one level up
      if varname not in self.data:
        ParseError(s, pos, i, "Undefined variable '%s'" % varname)
      vardata = self.data[varname]
      contents = self.eval(vardata)
      return (contents, i)
    else:
      # Ternary Mode
      (is_cond_true,i) = self.eval_bool_expr(s, pos, ['?']+terminators)
      assert(i < len(s) and s[i] == '?')

      (if_true_expr, j) = self.eval_expr(s, i+1, [' : ']+terminators)
      if j == len(s):
        return ('', j) # Error one level up

      if s[j:j+3] == ' : ':
        (if_false_expr,j) = self.eval_expr(s, j+3, terminators)
        if j == len(s):
          # This is an error condition one level up.
          return ('', j)
      else:
        if_false_expr = ''

      if is_cond_true:
        contents = if_true_expr.strip()
      else:
        contents = if_false_expr.strip()

      return (contents, j)

  # Evaluate an expression with ${} in string s, starting at pos.
  # Returns (the_evaluated_expression, endpos)
  def eval_expr(self, s, pos, terminators):
    (m,i) = FindFirst(s, pos, ['${'] + terminators)
    leftpart = s[pos:i]
    if i == len(s) or m in terminators:
      return (leftpart, i)

    (middlepart, j) = self.eval_bracket_expr(s, i+2, ['}'])
    if j == len(s) or s[j] != '}':
      ParseError(s, i, j, 'Unterminated ${')

    (rightpart, k) = self.eval_expr(s, j+1, terminators)
    return (leftpart + middlepart + rightpart, k)


env = Environment()

def override_env(meth_name, func):
  """Override a method in the global |env|, given the method name
  and the new function.
  """
  global env
  setattr(env, meth_name, types.MethodType(func, env, Environment))
