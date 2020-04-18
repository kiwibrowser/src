#!/usr/bin/env python
#
# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Identifies address adjustments required for native crash dumps."""

import glob
import os.path
import re
import subprocess


_BASE_APK = 'base.apk'
_LIBCHROME_SO = 'libchrome.so'

_BUILD_FINGERPRINT_RE = re.compile('.*Build fingerprint: (.*)$')


def GetTargetAndroidVersionNumber(lines):
  """Return the Android major version number from the build fingerprint.

  Args:
    lines: Lines read from the tombstone file, before preprocessing.
  Returns:
    5, 6, etc, or None if not determinable (developer build?)
  """
  # For example, "Build fingerprint: 'Android/aosp_flo/flo:5.1.1/...'" is 5.
  for line in lines:
    m = _BUILD_FINGERPRINT_RE.match(line)
    if not m:
      continue
    fingerprint = m.group(1)
    try:
      version = fingerprint.split('/')[2].split(':')[1].split('.')[0]
      return int(version)
    except Exception:
      pass
  return None


def _HasElfHeader(path):
  """Return True if the file at the given path has an ELF magic header.

  Minimal check only, for 'ELF' in bytes 1 to 3 of the file. Filters out
  the zero-byte false-positives such as libchromeview.so returned by glob.

  Args:
    path: Path to file to check.
  Returns:
    True or False
  """
  with open(path) as stream:
    elf_header = stream.read(4)
    return len(elf_header) == 4 and elf_header[1:4] == 'ELF'


def _ReadElfProgramHeaders(lib):
  """Return an iterable of program headers, from 'readelf -l ...'.

  Uses the platform readelf in all cases. This is somewhat lazy, but suffices
  in practice because program headers in ELF files are architecture-agnostic.

  Args:
    lib: Library file to read.
  Returns:
    [readelf -l output line, ...]
  """
  string = subprocess.check_output(['readelf', '-l', lib])
  return string.split('\n')


def _FindMinLoadVaddr(lib):
  """Return the minimum VirtAddr field of all library LOAD segments.

  Args:
    lib: Library file to read.
  Returns:
    Min VirtAddr field for all LOAD segments, or 0 if none found.
  """
  vaddrs = []
  # Locate LOAD lines and find the smallest VirtAddr field, eg:
  #   Type       Offset    VirtAddr   PhysAddr   FileSiz   MemSiz    Flg Align
  #   LOAD       0x000000  0x001d6000 0x001d6000 0x20f63fc 0x20f63fc R E 0x1000
  #   LOAD       0x20f6970 0x022cd970 0x022cd970 0x182df8  0x1b4490  RW  0x1000
  # would return 0x1d6000. Ignores all non-LOAD lines.
  for line in _ReadElfProgramHeaders(lib):
    elements = line.split()
    if elements and elements[0] == 'LOAD':
      vaddrs.append(int(elements[2], 16))
  if vaddrs:
    return min(vaddrs)
  return 0


def GetLoadVaddrs(stripped_libs=None, stripped_libs_dir=None):
  """Return a dict of minimum VirtAddr for libraries in the given directory.

  The dictionary returned may be passed to stack_core.ConvertTrace(). In
  pre-M Android releases the addresses printed by debuggerd into tombstones
  do not take account of non-zero vaddrs. Here we collect this information,
  so that we can use it later to correct such debuggerd tombstones.

  Args:
    stripped_libs_dir: Path to directory containing apk's stripped libraries.
  Returns:
    {'libchrome.so': 12345, ...}
  """
  if not stripped_libs:
    stripped_libs = []
  if stripped_libs_dir:
    stripped_libs.extend(glob.glob(os.path.join(stripped_libs_dir, '*.so')))
  libs = [l for l in stripped_libs if _HasElfHeader(l)]

  load_vaddrs = {}
  for lib in libs:
    min_vaddr = _FindMinLoadVaddr(lib)
    if min_vaddr:
      # Store with the library basename as the key. This is because once on
      # the device its path may not fully match its place in the APK staging
      # directory
      load_vaddrs[os.path.basename(lib)] = min_vaddr

  # Direct load from APK causes debuggerd to tag trace lines as if from the
  # file .../base.apk. So if we encounter a libchrome.so with packed
  # relocations, replicate this as base.apk so that later adjustment code
  # finds the appropriate adjustment.
  if _LIBCHROME_SO in load_vaddrs:
    load_vaddrs[_BASE_APK] = load_vaddrs[_LIBCHROME_SO]

  return load_vaddrs
