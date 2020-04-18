# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# Config file for various nacl compilation scenarios
#
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
import pynacl.platform

TOOLCHAIN_CONFIGS = {}

def AppendDictionary(d1, d2):
  for tag, value in d2.iteritems():
    if tag in d1:
      d1[tag] = d1[tag] + ' ' + value
    else:
      d1[tag] = value


class ToolchainConfig(object):
  def __init__(self, desc, commands=None, tools_needed=None, is_flaky=False,
               attributes=[], base=None, **extra):
    self._desc = desc,
    self._commands = commands or base._commands
    self._tools_needed = tools_needed or base._tools_needed
    self._extra = base._extra.copy() if base else {}
    self._extra.update(extra)
    self._flaky = is_flaky
    self._attributes = attributes

  def Append(self, tag, value):
    assert tag in self._extra
    self._extra[tag] = self._extra[tag] + ' ' + value + ' '

  def SanityCheck(self):
    for t in self._tools_needed:
      if not os.access(t, os.R_OK | os.X_OK):
        print "ERROR: missing tool ", t
        sys.exit(-1)

  def GetDescription(self):
    return self._desc

  def GetCommands(self, extra):
    for tag, val in self._commands:
      d = self._extra.copy()
      AppendDictionary(d, extra)
      yield tag, val % d

  def GetPhases(self):
    return [a for (a, _) in self._commands]

  def IsFlaky(self):
    return self._flaky

  def GetAttributes(self):
    return set(self._attributes)


######################################################################
#
######################################################################

LOCAL_GCC = '/usr/bin/gcc'

EMU_SCRIPT = 'toolchain/linux_x86/arm_trusted/run_under_qemu_arm'

TEMPLATE_DIGITS = 'X' * 16
BOOTSTRAP_ARGS = '--r_debug=0x%s --reserved_at_zero=0x%s' % (TEMPLATE_DIGITS,
                                                             TEMPLATE_DIGITS)

BOOTSTRAP_ARM = 'scons-out/opt-linux-arm/staging/nacl_helper_bootstrap'
SEL_LDR_ARM = 'scons-out/opt-linux-arm/staging/sel_ldr'
IRT_ARM = 'scons-out/nacl_irt-arm/obj/src/untrusted/irt/irt_core.nexe'
RUN_SEL_LDR_ARM = BOOTSTRAP_ARM + ' ' + SEL_LDR_ARM + ' ' + BOOTSTRAP_ARGS

BOOTSTRAP_X32 = 'scons-out/opt-linux-x86-32/staging/nacl_helper_bootstrap'
SEL_LDR_X32 = 'scons-out/opt-linux-x86-32/staging/sel_ldr'
IRT_X32 = 'scons-out/nacl_irt-x86-32/obj/src/untrusted/irt/irt_core.nexe'
RUN_SEL_LDR_X32 = BOOTSTRAP_X32 + ' ' + SEL_LDR_X32 + ' ' + BOOTSTRAP_ARGS

BOOTSTRAP_X64 = 'scons-out/opt-linux-x86-64/staging/nacl_helper_bootstrap'
SEL_LDR_X64 = 'scons-out/opt-linux-x86-64/staging/sel_ldr'
IRT_X64 = 'scons-out/nacl_irt-x86-64/obj/src/untrusted/irt/irt_core.nexe'
RUN_SEL_LDR_X64 = BOOTSTRAP_X64 + ' ' + SEL_LDR_X64 + ' ' + BOOTSTRAP_ARGS

NACL_X86_NEWLIB = 'toolchain/linux_x86/nacl_x86_newlib'
NACL_GCC_X32 = NACL_X86_NEWLIB + '/bin/i686-nacl-gcc'
NACL_GCC_X64 = NACL_X86_NEWLIB + '/bin/x86_64-nacl-gcc'

GLOBAL_CFLAGS = ' '.join(['-DSTACK_SIZE=0x40000',
                          '-D_XOPEN_SOURCE=600',
                          '-DNO_TRAMPOLINES',
                          '-DNO_LABEL_VALUES',])

CLANG_CFLAGS = ' '.join(['-fwrapv',
                         '-fdiagnostics-show-category=name'])

######################################################################
# LOCAL GCC
######################################################################
COMMANDS_local_gcc = [
    ('compile',
     '%(CC)s %(src)s %(CFLAGS)s -o %(tmp)s.exe -lm -lstdc++',
     ),
    ('run',
     '%(tmp)s.exe',
     ),
    ]

TOOLCHAIN_CONFIGS['local_gcc_x8632_O0'] = ToolchainConfig(
    desc='local gcc [x86-32]',
    attributes=['x86-32', 'O0'],
    commands=COMMANDS_local_gcc,
    tools_needed=[LOCAL_GCC],
    CC = LOCAL_GCC,
    CFLAGS = '-O0 -m32 -static ' + GLOBAL_CFLAGS)

TOOLCHAIN_CONFIGS['local_gcc_x8632_O3'] = ToolchainConfig(
    desc='local gcc [x86-32]',
    attributes=['x86-32', 'O3'],
    commands=COMMANDS_local_gcc,
    tools_needed=[LOCAL_GCC],
    CC = LOCAL_GCC,
    CFLAGS = '-O3 -m32 -static ' + GLOBAL_CFLAGS)

TOOLCHAIN_CONFIGS['local_gcc_x8664_O0'] = ToolchainConfig(
    desc='local gcc [x86-64]',
    attributes=['x86-64', 'O0'],
    commands=COMMANDS_local_gcc,
    tools_needed=[LOCAL_GCC],
    CC = LOCAL_GCC,
    CFLAGS = '-O0 -m64 -static ' + GLOBAL_CFLAGS)

TOOLCHAIN_CONFIGS['local_gcc_x8664_O3'] = ToolchainConfig(
    attributes=['x86-64', 'O3'],
    desc='local gcc [x86-64]',
    commands=COMMANDS_local_gcc,
    tools_needed=[LOCAL_GCC],
    CC = LOCAL_GCC,
    CFLAGS = '-O3 -m64 -static ' + GLOBAL_CFLAGS)

######################################################################
# # NACL + SEL_LDR [X86]
######################################################################
COMMANDS_nacl_gcc = [
    ('compile',
     '%(CC)s %(src)s %(CFLAGS)s -o %(tmp)s.exe -lm -lstdc++',
     ),
    ('sel_ldr',
     '%(SEL_LDR)s -B %(IRT)s %(tmp)s.exe',
     )
  ]


TOOLCHAIN_CONFIGS['nacl_gcc_x8632_O0'] = ToolchainConfig(
    desc='nacl gcc [x86-32]',
    attributes=['x86-32', 'O0'],
    commands=COMMANDS_nacl_gcc,
    tools_needed=[NACL_GCC_X32, BOOTSTRAP_X32, SEL_LDR_X32],
    CC = NACL_GCC_X32,
    SEL_LDR = RUN_SEL_LDR_X32,
    IRT = IRT_X32,
    CFLAGS = '-O0 -static -Bscons-out/nacl-x86-32/lib/ ' + GLOBAL_CFLAGS)

TOOLCHAIN_CONFIGS['nacl_gcc_x8632_O3'] = ToolchainConfig(
    desc='nacl gcc with optimizations [x86-32]',
    attributes=['x86-32', 'O3'],
    commands=COMMANDS_nacl_gcc,
    tools_needed=[NACL_GCC_X32, BOOTSTRAP_X32, SEL_LDR_X32],
    CC = NACL_GCC_X32,
    SEL_LDR = RUN_SEL_LDR_X32,
    IRT = IRT_X32,
    CFLAGS = '-O3 -static -Bscons-out/nacl-x86-32/lib/ ' + GLOBAL_CFLAGS)

TOOLCHAIN_CONFIGS['nacl_gcc_x8664_O0'] = ToolchainConfig(
    desc='nacl gcc [x86-64]',
    attributes=['x86-64', 'O0'],
    commands=COMMANDS_nacl_gcc,
    tools_needed=[NACL_GCC_X64, BOOTSTRAP_X64, SEL_LDR_X64],
    CC = NACL_GCC_X64,
    SEL_LDR = RUN_SEL_LDR_X64,
    IRT = IRT_X64,
    CFLAGS = '-O0 -static -Bscons-out/nacl-x86-64/lib/ ' + GLOBAL_CFLAGS)

TOOLCHAIN_CONFIGS['nacl_gcc_x8664_O3'] = ToolchainConfig(
    desc='nacl gcc with optimizations [x86-64]',
    attributes=['x86-32', 'O3'],
    commands=COMMANDS_nacl_gcc,
    tools_needed=[NACL_GCC_X64, BOOTSTRAP_X64, SEL_LDR_X64],
    CC = NACL_GCC_X64,
    SEL_LDR = RUN_SEL_LDR_X64,
    IRT = IRT_X64,
    CFLAGS = '-O3 -static -Bscons-out/nacl-x86-64/lib/ ' + GLOBAL_CFLAGS)

######################################################################
# PNACL + SEL_LDR [ARM]
######################################################################

# Locate the pnacl toolchain.  Path can be overridden externally.
os_name = pynacl.platform.GetOS()
PNACL_TOOLCHAIN_DIR = os.getenv('PNACL_TOOLCHAIN_DIR',
                                '%s_x86/pnacl_newlib' % os_name)
PNACL_ROOT = os.path.join('toolchain', PNACL_TOOLCHAIN_DIR)
PNACL_FRONTEND = PNACL_ROOT + '/bin/pnacl-clang++'
PNACL_FINALIZE = PNACL_ROOT + '/bin/pnacl-finalize'


# NOTE: Our driver supports going from .c to .nexe in one go
#       but it maybe useful to inspect the bitcode file so we
#       split the compilation into two steps.
PNACL_LD = PNACL_ROOT + '/bin/pnacl-translate'

COMMANDS_llvm_pnacl_arm = [
    ('compile-pexe',
     '%(CC)s %(src)s %(CFLAGS)s -o %(tmp)s.nonfinal.pexe',
     ),
    ('finalize-pexe',
     '%(FINALIZE)s %(FINALIZE_FLAGS)s %(tmp)s.nonfinal.pexe'
     ' -o %(tmp)s.final.pexe'
    ),
    ('translate-arm',
     '%(LD)s %(TRANSLATE_FLAGS)s %(tmp)s.final.pexe -o %(tmp)s.nexe',
     ),
    ('qemu-sel_ldr',
     '%(EMU)s %(SEL_LDR)s -B %(IRT)s -Q %(tmp)s.nexe',
     )
  ]


# In the PNaCl ToolchainConfig attributes, the convention for the 'b'
# and 'f' suffixes is that 'O0f' means frontend (clang) compilation
# with -O0, and 'O2b' means backend (llc) translation with -O2.
# In addition, the 'b_sz' suffix means to use pnacl-sz rather than pnacl-llc.
# TODO(stichnot): Add 'b_sz' configs to the missing architectures when Subzero
# supports them.

TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O0'] = ToolchainConfig(
    desc='pnacl llvm [arm]',
    attributes=['arm', 'O0f', 'O2b'],
    commands=COMMANDS_llvm_pnacl_arm,
    tools_needed=[PNACL_FRONTEND, PNACL_FINALIZE, PNACL_LD,
                  EMU_SCRIPT, BOOTSTRAP_ARM, SEL_LDR_ARM],
    is_flaky = True,
    CC = PNACL_FRONTEND,
    FINALIZE = PNACL_FINALIZE,
    LD = PNACL_LD + ' -arch arm',
    EMU = EMU_SCRIPT,
    SEL_LDR = RUN_SEL_LDR_ARM,
    IRT = IRT_ARM,
    CFLAGS = '-O0 -static ' + CLANG_CFLAGS + ' ' + GLOBAL_CFLAGS,
    FINALIZE_FLAGS = '',
    TRANSLATE_FLAGS='')

TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O3'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O0'],
    desc='pnacl llvm with optimizations [arm]',
    attributes=['arm', 'O3f', 'O2b'],
    is_flaky = True,
    CFLAGS = '-O3 -D__OPTIMIZE__ -static ' + CLANG_CFLAGS  + ' '
              + GLOBAL_CFLAGS)

# Based on llvm_pnacl_arm_O3 with TRANSLATE_FLAGS=-translate-fast
TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O3_O0'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O3'],
    desc='pnacl llvm with optimizations and fast translation [arm]',
    attributes=['arm', 'O3f', 'O0b'],
    is_flaky = True,
    TRANSLATE_FLAGS = '-translate-fast')

# Based on llvm_pnacl_arm_O0 with TRANSLATE_FLAGS=-translate-fast
TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O0_O0'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O0'],
    desc='pnacl llvm [arm]',
    attributes=['arm', 'O0f', 'O0b'],
    is_flaky = True,
    TRANSLATE_FLAGS='-translate-fast')

# Based on llvm_pnacl_arm_O0 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O0_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O0'],
    desc='pnacl llvm with Subzero [arm32]',
    attributes=['arm', 'O0f', 'O2b_sz'],
    TRANSLATE_FLAGS = '--use-sz')

# Based on llvm_pnacl_arm_O3 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O3_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O3'],
    desc='pnacl llvm with Subzero [arm32]',
    attributes=['arm', 'O3f', 'O2b_sz'],
    TRANSLATE_FLAGS = '--use-sz')

# Based on llvm_pnacl_arm_O3_O0 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O3_O0_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O3_O0'],
    desc='pnacl llvm with Subzero -Om1 [arm32]',
    attributes=['arm', 'O3f', 'O0b_sz'],
    TRANSLATE_FLAGS = '-translate-fast --use-sz')

# Based on llvm_pnacl_arm_O0_O0 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O0_O0_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_arm_O0_O0'],
    desc='pnacl llvm with Subzero -Om1 [arm32]',
    attributes=['arm', 'O0f', 'O0b_sz'],
    TRANSLATE_FLAGS = '-translate-fast --use-sz')

######################################################################
# PNACL + SEL_LDR [X8632]
######################################################################

# NOTE: this is used for both x86 flavors
COMMANDS_llvm_pnacl_x86 = [
    ('compile-pexe',
     '%(CC)s %(src)s %(CFLAGS)s -o %(tmp)s.nonfinal.pexe',
     ),
    ('finalize-pexe',
     '%(FINALIZE)s %(FINALIZE_FLAGS)s %(tmp)s.nonfinal.pexe'
     ' -o %(tmp)s.final.pexe',
     ),
    ('translate-x86',
     '%(LD)s %(TRANSLATE_FLAGS)s %(tmp)s.final.pexe -o %(tmp)s.nexe ',
     ),
    ('sel_ldr',
     '%(SEL_LDR)s -B %(IRT)s %(tmp)s.nexe',
     )
  ]


TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O0'] = ToolchainConfig(
    desc='pnacl llvm [x8632]',
    attributes=['x86-32', 'O0f', 'O2b'],
    commands=COMMANDS_llvm_pnacl_x86,
    tools_needed=[PNACL_FRONTEND, PNACL_FINALIZE, PNACL_LD,
                  BOOTSTRAP_X32, SEL_LDR_X32],
    CC = PNACL_FRONTEND,
    FINALIZE = PNACL_FINALIZE,
    LD = PNACL_LD + ' -arch x86-32',
    SEL_LDR = RUN_SEL_LDR_X32,
    IRT = IRT_X32,
    CFLAGS = '-O0  -static ' + CLANG_CFLAGS + ' ' + GLOBAL_CFLAGS,
    FINALIZE_FLAGS = '',
    TRANSLATE_FLAGS = '')

# Based on llvm_pnacl_x86-32_O0 with -O3 -D__OPTIMIZE__ instead of -O0
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O3'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O0'],
    desc='pnacl llvm [x8632]',
    attributes=['x86-32', 'O3f', 'O2b'],
    CFLAGS = '-O3 -D__OPTIMIZE__ -static ' + CLANG_CFLAGS + ' '
             + GLOBAL_CFLAGS)

# Based on llvm_pnacl_x86-32_O3 with TRANSLATE_FLAGS=-translate-fast
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O3_O0'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O3'],
    desc='pnacl llvm with fast translation [x8632]',
    attributes=['x86-32', 'O3f', 'O0b'],
    TRANSLATE_FLAGS = '-translate-fast')

# Based on llvm_pnacl_x86-32_O0 with TRANSLATE_FLAGS=-translate-fast
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O0_O0'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O0'],
    desc='pnacl llvm with fast translation [x8632]',
    attributes=['x86-32', 'O0f', 'O0b'],
    TRANSLATE_FLAGS = '-translate-fast')

# Based on llvm_pnacl_x86-32_O0 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O0_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O0'],
    desc='pnacl llvm with Subzero [x8632]',
    attributes=['x86-32', 'O0f', 'O2b_sz'],
    TRANSLATE_FLAGS = '--use-sz')

# Based on llvm_pnacl_x86-32_O3 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O3_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O3'],
    desc='pnacl llvm with Subzero [x8632]',
    attributes=['x86-32', 'O3f', 'O2b_sz'],
    TRANSLATE_FLAGS = '--use-sz')

# Based on llvm_pnacl_x86-32_O3_O0 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O3_O0_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O3_O0'],
    desc='pnacl llvm with Subzero -Om1 [x8632]',
    attributes=['x86-32', 'O3f', 'O0b_sz'],
    TRANSLATE_FLAGS = '-translate-fast --use-sz')

# Based on llvm_pnacl_x86-32_O0_O0 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O0_O0_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-32_O0_O0'],
    desc='pnacl llvm with Subzero -Om1 [x8632]',
    attributes=['x86-32', 'O0f', 'O0b_sz'],
    TRANSLATE_FLAGS = '-translate-fast --use-sz')

######################################################################
# PNACL + SEL_LDR [X8664]
######################################################################

TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O0'] = ToolchainConfig(
    desc='pnacl llvm [x8664]',
    attributes=['x86-64', 'O0f', 'O2b'],
    commands=COMMANDS_llvm_pnacl_x86,
    tools_needed=[PNACL_FRONTEND, PNACL_FINALIZE, PNACL_LD,
                  BOOTSTRAP_X64, SEL_LDR_X64],
    CC = PNACL_FRONTEND,
    FINALIZE = PNACL_FINALIZE,
    LD = PNACL_LD + ' -arch x86-64',
    SEL_LDR = RUN_SEL_LDR_X64,
    IRT = IRT_X64,
    CFLAGS = '-O0 -static ' + CLANG_CFLAGS + ' ' + GLOBAL_CFLAGS,
    FINALIZE_FLAGS = '',
    TRANSLATE_FLAGS = '')

# Based on llvm_pnacl_x86-64_O0 with -O3 -D__OPTIMIZE__ instead of -O0
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O3'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O0'],
    desc='pnacl llvm [x8664]',
    attributes=['x86-64', 'O3f', 'O2b'],
    CFLAGS = '-O3 -D__OPTIMIZE__ -static ' + CLANG_CFLAGS + ' '
             + GLOBAL_CFLAGS)

# Based on llvm_pnacl_x86-64_O3 with TRANSLATE_FLAGS=-translate-fast
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O3_O0'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O3'],
    desc='pnacl llvm with fast translation [x8664]',
    attributes=['x86-64', 'O3f', 'O0b'],
    TRANSLATE_FLAGS = '-translate-fast')

# Based on llvm_pnacl_x86-64_O0 with TRANSLATE_FLAGS=-translate-fast
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O0_O0'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O0'],
    desc='pnacl llvm [x8664]',
    attributes=['x86-64', 'O0f', 'O0b'],
    TRANSLATE_FLAGS = '-translate-fast')

# Based on llvm_pnacl_x86-64_O0 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O0_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O0'],
    desc='pnacl llvm with Subzero [x8664]',
    attributes=['x86-64', 'O0f', 'O2b_sz'],
    TRANSLATE_FLAGS = '--use-sz')

# Based on llvm_pnacl_x86-64_O3 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O3_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O3'],
    desc='pnacl llvm with Subzero [x8664]',
    attributes=['x86-64', 'O3f', 'O2b_sz'],
    TRANSLATE_FLAGS = '--use-sz')

# Based on llvm_pnacl_x86-64_O3_O0 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O3_O0_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O3_O0'],
    desc='pnacl llvm with Subzero -Om1 [x8664]',
    attributes=['x86-64', 'O3f', 'O0b_sz'],
    TRANSLATE_FLAGS = '-translate-fast --use-sz')

# Based on llvm_pnacl_x86-64_O0_O0 with TRANSLATE_FLAGS+=--use-sz
TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O0_O0_sz'] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['llvm_pnacl_x86-64_O0_O0'],
    desc='pnacl llvm with Subzero -Om1 [x8664]',
    attributes=['x86-64', 'O0f', 'O0b_sz'],
    TRANSLATE_FLAGS = '-translate-fast --use-sz')

######################################################################
# CLANG + SEL_LDR
######################################################################

def NaClClangCommands(arch):
  commands = [
      ('compile',
       '%(CC)s %(src)s %(CFLAGS)s -o %(tmp)s.nexe -lm',
      ),
  ]
  if arch == 'arm':
    commands.append(
        ('qemu-sel_ldr',
         '%(EMU)s %(SEL_LDR)s -B %(IRT)s -Q %(tmp)s.nexe',
        ))
  else:
    commands.append(
        ('sel_ldr',
         '%(SEL_LDR)s -B %(IRT)s %(tmp)s.nexe',
        ))
  return commands

def NaClClang(arch, cpp=False):
  arch_map = {'x86-64': 'x86_64', 'x86-32': 'i686', 'arm': 'arm'}
  suffix = '++' if cpp else ''
  return os.path.join(PNACL_ROOT, 'bin',
                      arch_map[arch] + '-nacl-clang' + suffix)

# TODO(dschuff): the other uses of SEL_LDR_ARCH etc can probably just be
# replaced with something like this everywhere.
def Tool(arch, tool):
  toolmap = {
    'x86-64': {
      'sel_ldr': SEL_LDR_X64,
      'bootstrap': BOOTSTRAP_X64,
      'irt': IRT_X64,
      'run_sel_ldr': RUN_SEL_LDR_X64,
    },
    'x86-32': {
      'sel_ldr': SEL_LDR_X32,
      'bootstrap': BOOTSTRAP_X32,
      'irt': IRT_X32,
      'run_sel_ldr': RUN_SEL_LDR_X32,
    },
    'arm': {
      'sel_ldr': SEL_LDR_ARM,
      'bootstrap': BOOTSTRAP_ARM,
      'irt': IRT_ARM,
      'run_sel_ldr': RUN_SEL_LDR_ARM,
    }
  }
  return toolmap[arch][tool]

for arch in ['x86-64', 'x86-32', 'arm']:
  TOOLCHAIN_CONFIGS['nacl_clang_%s_O0' % arch] = ToolchainConfig(
    desc='clang [%s O0]' % arch,
    attributes=[arch, 'O0'],
    commands=NaClClangCommands(arch),
    tools_needed=[NaClClang(arch), Tool(arch, 'bootstrap'),
                  Tool(arch, 'sel_ldr')],
    CC = NaClClang(arch),
    SEL_LDR = Tool(arch, 'run_sel_ldr'),
    IRT = Tool(arch, 'irt'),
    EMU = EMU_SCRIPT,
    CFLAGS = '-O0 ' + CLANG_CFLAGS + ' ' + GLOBAL_CFLAGS)

  # Unlike gcc (which doesn't care) and pnacl-clang (which figures out the
  # source language and explicitly tells the underlying clang), nacl-clang
  # doesn't like building C files using the C++ compiler. So different configs
  # are needed for C++ vs C tests.
  TOOLCHAIN_CONFIGS['nacl_clang++_%s_O0' % arch] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['nacl_clang_%s_O0' % arch],
    desc='clang++ [%s O0]' % arch,
    attributes=[arch, 'O3'],
    CC = NaClClang(arch, cpp=True))

  TOOLCHAIN_CONFIGS['nacl_clang_%s_O3' % arch] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['nacl_clang_%s_O0' % arch],
    desc='clang [%s O3]' % arch,
    attributes=[arch, 'O3'],
    CFLAGS = '-O3 ' + CLANG_CFLAGS + ' ' + GLOBAL_CFLAGS)

  TOOLCHAIN_CONFIGS['nacl_clang++_%s_O3' % arch] = ToolchainConfig(
    base=TOOLCHAIN_CONFIGS['nacl_clang_%s_O3' % arch],
    desc='clang++ [%s O3]' % arch,
    attributes=[arch, 'O3'],
    CC = NaClClang(arch, cpp=True))
