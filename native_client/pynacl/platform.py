#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys

import lib

# Define a list of supported operating systems here. The OS is based on the
# "sys.platform" variable. The list and variable names need to be kept in sync,
# it is written so there will be a syntax error if an item was added to 1 list
# but not the other, although there is no check in place to be sure the order
# is kept.
OS_LIST = [
    'win',
    'mac',
    'linux',
]

(
    OS_WIN,
    OS_MAC,
    OS_LINUX,
) = OS_LIST

OS_MAP = {
    OS_WIN: ('win', 'win32', 'cygwin'),
    OS_MAC: ('mac', 'darwin'),
    OS_LINUX: ('linux', 'linux2', 'linux3', 'netbsd7'),
}

OS_DICT = dict([(platform, os_name)
                for os_name, platforms in OS_MAP.iteritems()
                for platform in platforms])

def GetOS(platform=None):
  if platform is None:
    platform = sys.platform

  platform = platform.lower()
  assert platform in OS_DICT, "Unrecognized OS platform: %s" % platform
  return OS_DICT[platform]


def IsCygWin(platform=None):
  if platform is None:
    platform = sys.platform

  platform = platform.lower()
  return platform == 'cygwin'

# Define a list of supported architectures here. Our architecture definition
# comes in 2 flavors, ones that care about 32/64 bit differences, and ones that
# do not (a flattened list). The architecture is based on the
# "lib.platform.machine()" variable which can return many variations even on
# the same architecture (see variations of x86-32). The list and variable names
# needs to be kept in sync, it is written so there will be a syntax error if
# an item was added to 1 list but not the other, although there is no check
# in place to be sure the order is kept.
ARCH3264_LIST = [
    'x86-32',
    'x86-32-nonsfi',
    'x86-64',
    'arm',
    'arm-nonsfi',
    'mips32',
]

(
    ARCH3264_X86_32,
    ARCH3264_X86_32_NONSFI,
    ARCH3264_X86_64,
    ARCH3264_ARM,
    ARCH3264_ARM_NONSFI,
    ARCH3264_MIPS32,
) = ARCH3264_LIST

ARCH64_SET = set([ARCH3264_X86_64])

ARCH3264_MAP = {
    ARCH3264_X86_32: ('x86', 'x86-32', 'x86_32', 'x8632', 'i386',
                      'i686', 'ia32', '32'),
    ARCH3264_X86_32_NONSFI: ('x86-32-nonsfi',),
    ARCH3264_X86_64: ('x86-64', 'amd64', 'x86_64', 'x8664', '64'),
    ARCH3264_ARM: ('arm', 'armv7', 'armv7l'),
    ARCH3264_ARM_NONSFI: ('arm-nonsfi',),
    ARCH3264_MIPS32: ('mips32', 'mips', 'mipsel'),
}

ARCH3264_DICT = dict([(machine, arch_name)
                      for arch_name, machinelist in ARCH3264_MAP.iteritems()
                      for machine in machinelist])

ARCH_LIST = [
    'x86',
    'arm',
    'mips'
]

(
    ARCH_X86,
    ARCH_ARM,
    ARCH_MIPS,
) = ARCH_LIST

# ARCH_MAP should be a strict flattening of ARCH3264_MAP, use that list to be
# sure both of them contain all the variations of machine names.
ARCH_MAP = {
    ARCH_X86: (ARCH3264_X86_32, ARCH3264_X86_64),
    ARCH_ARM: (ARCH3264_ARM,),
    ARCH_MIPS: (ARCH3264_MIPS32,),
}

ARCH_DICT = dict([(arch3264_name, arch_name)
                  for arch_name, arch3264_list in ARCH_MAP.iteritems()
                  for arch3264_name in arch3264_list])

def GetArch3264(machine=None):
  if machine is None:
    machine = lib.platform.machine()
    # platform.machine is based on running kernel. It's possible to use 64-bit
    # kernel with 32-bit userland, e.g. to give linker slightly more memory.
    # Distinguish between different userland bitness by querying
    # the python binary.
    if (ARCH3264_DICT.get(machine) == ARCH3264_X86_64 and
            lib.platform.architecture()[0] == '32bit'):
      machine = 'ia32'

  machine = machine.lower()
  assert machine in ARCH3264_DICT, "Unrecognized arch machine: %s" % machine
  return ARCH3264_DICT[machine]

def IsArch64Bit(machine=None):
  return GetArch3264(machine) in ARCH64_SET

def GetArch(machine=None):
  arch3264 = GetArch3264(machine)
  return ARCH_DICT[arch3264]

# Here are some helper function for common checks, these should be based
# on the generic functions above.
def IsWindows(platform=None):
  return GetOS(platform) == OS_WIN

def IsMac(platform=None):
  return GetOS(platform) == OS_MAC

def IsLinux(platform=None):
  return GetOS(platform) == OS_LINUX

def IsLinux64(platform=None, machine=None):
  return IsLinux(platform) and IsArch64Bit(machine)

# If we are on cygwin convert a (possibly) Windows path to one we can use
# with Python APIs
def CygPath(path):
  if IsCygWin():
    return subprocess.check_output(['cygpath', path]).strip()
  return path

# Some of our tools utilize a unique platform string which is used to
# distinguish between platform and architectures.
def PlatformTriple(platform=None, machine=None):
  os = GetOS(platform)
  arch3264 = GetArch3264(machine)

  if os == OS_WIN:
    if IsCygWin(platform):
      return 'i686-pc-cygwin'
    else:
      return 'i686-w64-mingw32'
  elif os == OS_MAC:
    return 'x86_64-apple-darwin'
  elif os == OS_LINUX:
    if arch3264 == ARCH3264_ARM:
      # TODO(mcgrathr): How to distinguish gnueabi vs gnueabihf?
      return 'arm-linux-gnueabihf'
    elif arch3264 == ARCH3264_X86_32:
      return 'i686-linux'
    elif arch3264 == ARCH3264_X86_64:
      return 'x86_64-linux'

  raise Exception('Unknown platform and machine')


def KillSubprocessAndChildren(proc):
  """Kill a subprocess and all children.

  While this is trivial on Posix platforms, on Windows this requires some
  method for walking the process tree. Relying on this functionality in
  the taskkill.exe utility for now.

  Args:
    proc: A subprocess.Popen process.
  """
  if IsWindows():
    # Do subprocess call as the process may terminate before we manage
    # to invoke taskkill.
    subprocess.call(
        [os.path.join(os.environ['SYSTEMROOT'], 'System32', 'taskkill.exe'),
        '/F', '/T', '/PID', str(proc.pid)])
  else:
    proc.kill()
