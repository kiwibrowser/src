# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Conditions for scons to gn

Contains all conditions we iterate over (OS, CPU), as well as helper
convertion functions.
"""

FULLARCH = {
  'arm' : 'arm',
  'x86' : 'x86-32',
  'x64' : 'x86-64'
}

SUBARCH = {
  'arm' : '32',
  'x86' : '32',
  'x64' : '64'
}

class Conditions(object):
  def __init__(self, seta, setb):
    self._set_a = seta
    self._set_b = setb
    self._all = ['%s_%s' % (a, b) for a in seta for b in setb]
    self._active_condition = self._all[0]

  def get(self, key, default=False):
    os, arch = self._active_condition.split('_')
    if key in ["TARGET_FULLARCH", "TARGET_ARCHITECTURE"]:
      return FULLARCH[arch]
    if key == "TARGET_SUBARCH":
      return SUBARCH[arch]

  def All(self):
    return self._all

  def Bit(self, name):
    _, arch = self._active_condition.split('_')

    if name == 'coverage_enabled':
      return False

    if name == 'build_x86':
      return arch == 'x86' or arch == 'x64'

    if name == 'build_arm':
      return arch == 'arm'

    if name == 'build_x86_32':
      return arch == 'x86'

    if name == 'build_x86_64':
      return arch == 'x64'

    if name == 'build_mips32':
      return arch == 'mips32'

    if name == 'target_arm':
      return arch == 'arm'

    if name == 'target_x86':
      return arch == 'x86' or arch == 'x64'

    if name == 'target_x86_32':
      return arch == 'x86'

    if name == 'target_x86_64':
      return arch == 'x64'

    print 'Unknown bit: ' + name
    return False

  def SetA(self):
    return self._set_a

  def SetB(self):
    return self._set_b

  def ActiveCondition(self):
    return self._active_condition

  def SetActiveCondition(self, cond):
    if cond not in self._all:
      raise RuntimeError('Unknown condition: ' + cond)
    self._active_condition = cond

  def WriteImports(self, fileobj):
    if self.imports:
      fileobj.write("\n")
      for imp in self.imports:
        fileobj.write('import("%s")\n' % imp)
      fileobj.write("\n")


class TrustedConditions(Conditions):
  def __init__(self):
    OSES = ['AND', 'CHR', 'IOS',  'LIN', 'MAC', 'WIN']
    ARCH = ['arm', 'x86', 'x64']
    Conditions.__init__(self, OSES, ARCH)
    self.imports = []

  def Bit(self, name):
    os, arch = self._active_condition.split('_')
    osname = name[:3].upper()

    if osname in self.SetA():
      return osname == os

    return Conditions.Bit(self, name)


class UntrustedConditions(Conditions):
  def __init__(self):
    LIBS = ['newlib', 'glibc']
    ARCH = ['arm', 'x86', 'x64', 'pnacl']
    Conditions.__init__(self, LIBS, ARCH)
    self.imports = [
      "//native_client/build/toolchain/nacl/nacl_sdk.gni"
    ]

  def get(self, key, default=False):
    os, arch = self._active_condition.split('_')
    if key == "TARGET_FULLARCH":
      return FULLARCH[arch]
    return Conditions.get(self, key, default)

  def Bit(self, name):
    libc, arch = self._active_condition.split('_')

    if name == 'bitcode':
      return arch == 'pnacl'

    if name[:5] == 'nacl_':
      return name[5:] == libc

    return Conditions.Bit(self, name)


BOGUS = """
ALL = ['%s_%s' % (os, cpu) for os in OSES for cpu in CPUS]

CPU_TO_BIT_MAP = {}
BIT_TO_CPU_MAP = {}

for idx, cpu in enumerate(CPUS):
  CPU_TO_BIT_MAP[cpu] = 1 << idx
  BIT_TO_CPU_MAP[1 << idx] = cpu

def CPUsToBits(cpus):
  out = 0;
  for cpu in cpus:
    out += CPU_TO_BIT_MAP[cpu]
  return out


def BitsToCPUs(cpus):
  out = []
  for i in [1, 2, 4]:
    if cpus & i:
      out.append(BIT_TO_CPU_MAP[i])
  return out
"""
