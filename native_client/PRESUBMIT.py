# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Documentation on PRESUBMIT.py can be found at:
# http://www.chromium.org/developers/how-tos/depottools/presubmit-scripts

import os
import subprocess
import sys

# List of directories to not apply presubmit project checks, relative
# to the NaCl top directory
EXCLUDE_PROJECT_CHECKS = [
    # The following contain test data (including automatically generated),
    # and do not follow our conventions.
    'src/trusted/validator_ragel/testdata/32/',
    'src/trusted/validator_ragel/testdata/64/',
    'src/trusted/validator_x86/testdata/32/',
    'src/trusted/validator_x86/testdata/64/',
    'src/trusted/validator/x86/decoder/generator/testdata/32/',
    'src/trusted/validator/x86/decoder/generator/testdata/64/',
    # The following directories contains automatically generated source,
    # which may not follow our conventions.
    'src/trusted/validator_x86/gen/',
    'src/trusted/validator/x86/decoder/gen/',
    'src/trusted/validator/x86/decoder/generator/gen/',
    'src/trusted/validator/x86/ncval_seg_sfi/gen/',
    'src/trusted/validator_arm/gen/',
    'src/trusted/validator_ragel/gen/',
    # The following files contain code from outside native_client (e.g. newlib)
    # or are known the contain style violations.
    'src/trusted/service_runtime/include/sys/',
    'src/include/win/port_win.h',
    'src/trusted/service_runtime/include/machine/_types.h'
    ]

NACL_TOP_DIR = os.getcwd()
while not os.path.isfile(os.path.join(NACL_TOP_DIR, 'PRESUBMIT.py')):
  NACL_TOP_DIR = os.path.dirname(NACL_TOP_DIR)
  assert len(NACL_TOP_DIR) >= 3, "Could not find NaClTopDir"


def CheckGitBranch():
  p = subprocess.Popen("git branch -vv", shell=True,
                       stdout=subprocess.PIPE)
  output, _ = p.communicate()
  lines = output.split('\n')
  for line in lines:
    # output format for checked-out branch should be
    # * branchname hash [TrackedBranchName ...
    toks = line.split()
    if '*' not in toks[0]:
      continue
    if not ('origin/master' in toks[3] or
            'origin/refs/heads/master' in toks[3]):
      warning = 'Warning: your current branch:\n' + line
      warning += '\nis not tracking origin/master. git cl push may silently '
      warning += 'fail to push your change. To fix this, do\n'
      warning += 'git branch -u origin/master'
      return warning
    return None
  print 'Warning: presubmit check could not determine local git branch'
  return None


def _CommonChecks(input_api, output_api):
  """Checks for both upload and commit."""
  results = []

  results.extend(input_api.canned_checks.PanProjectChecks(
      input_api, output_api, project_name='Native Client',
      excluded_paths=tuple(EXCLUDE_PROJECT_CHECKS)))

  # The commit queue assumes PRESUBMIT.py is standalone.
  # TODO(bradnelson): Migrate code_hygiene to a common location so that
  # it can be used by the commit queue.
  old_sys_path = list(sys.path)
  try:
    sys.path.append(os.path.join(NACL_TOP_DIR, 'tools'))
    sys.path.append(os.path.join(NACL_TOP_DIR, 'build'))
    import code_hygiene
  finally:
    sys.path = old_sys_path
    del old_sys_path

  affected_files = input_api.AffectedFiles(include_deletes=False)
  exclude_dirs = [ NACL_TOP_DIR + '/' + x for x in EXCLUDE_PROJECT_CHECKS ]
  for filename in affected_files:
    filename = filename.AbsoluteLocalPath()
    if filename in exclude_dirs:
      continue
    if not IsFileInDirectories(filename, exclude_dirs):
      errors, warnings = code_hygiene.CheckFile(filename, False)
      for e in errors:
        results.append(output_api.PresubmitError(e, items=errors[e]))
      for w in warnings:
        results.append(output_api.PresubmitPromptWarning(w, items=warnings[w]))

  return results


def IsFileInDirectories(f, dirs):
  """ Returns true if f is in list of directories"""
  for d in dirs:
    if d is os.path.commonprefix([f, d]):
      return True
  return False


def CheckChangeOnUpload(input_api, output_api):
  """Verifies all changes in all files.
  Args:
    input_api: the limited set of input modules allowed in presubmit.
    output_api: the limited set of output modules allowed in presubmit.
  """
  report = []
  report.extend(_CommonChecks(input_api, output_api))

  branch_warning = CheckGitBranch()
  if branch_warning:
    report.append(output_api.PresubmitPromptWarning(branch_warning))

  return report


def CheckChangeOnCommit(input_api, output_api):
  """Verifies all changes in all files and verifies that the
  tree is open and can accept a commit.
  Args:
    input_api: the limited set of input modules allowed in presubmit.
    output_api: the limited set of output modules allowed in presubmit.
  """
  report = []
  report.extend(_CommonChecks(input_api, output_api))
  report.extend(input_api.canned_checks.CheckTreeIsOpen(
      input_api, output_api,
      json_url='http://nativeclient-status.appspot.com/current?format=json'))
  return report


# Note that this list is duplicated in the Commit Queue.  If you change
# this list, you should also update the CQ's list in infra/config/cq.cfg
# (see https://crbug.com/399059).
DEFAULT_TRYBOTS = [
    'nacl-precise32_newlib_dbg',
    'nacl-precise32_newlib_opt',
    'nacl-precise32_glibc_opt',
    'nacl-precise64_newlib_dbg',
    'nacl-precise64_newlib_opt',
    'nacl-precise64_glibc_opt',
    'nacl-mac_newlib_dbg',
    'nacl-mac_newlib_opt',
    'nacl-mac_glibc_dbg',
    'nacl-mac_glibc_opt',
    'nacl-win32_newlib_opt',
    'nacl-win32_glibc_opt',
    'nacl-win64_newlib_dbg',
    'nacl-win64_newlib_opt',
    'nacl-win64_glibc_opt',
    'nacl-win8-64_newlib_dbg',
    'nacl-win8-64_newlib_opt',
    'nacl-arm_opt_panda',
    # arm-nacl-gcc bots
    'nacl-win7_64_arm_newlib_opt',
    'nacl-mac_arm_newlib_opt',
    'nacl-precise64_arm_newlib_opt',
    'nacl-precise64_arm_glibc_opt',
    # pnacl scons bots
    'nacl-precise_64-newlib-arm_qemu-pnacl',
    'nacl-precise_64-newlib-x86_32-pnacl',
    'nacl-precise_64-newlib-x86_64-pnacl',
    'nacl-mac_newlib_opt_pnacl',
    'nacl-win7_64_newlib_opt_pnacl',
    # pnacl spec2k bots
    'nacl-arm_perf_panda',
    'nacl-precise_64-newlib-x86_32-pnacl-spec',
    'nacl-precise_64-newlib-x86_64-pnacl-spec',
    ]

PNACL_TOOLCHAIN_TRYBOTS = [
    'nacl-toolchain-linux-pnacl-x86_64',
    'nacl-toolchain-linux-pnacl-x86_32',
    'nacl-toolchain-mac-pnacl-x86_32',
    'nacl-toolchain-win7-pnacl-x86_64',
    ]

TOOLCHAIN_BUILD_TRYBOTS = [
    'nacl-toolchain-precise64-newlib-arm',
    'nacl-toolchain-mac-newlib-arm',
    ] + PNACL_TOOLCHAIN_TRYBOTS


def GetPreferredTryMasters(_, change):

  has_pnacl = False
  has_toolchain_build = False
  has_others = False

  for file in change.AffectedFiles():
    if IsFileInDirectories(file.AbsoluteLocalPath(),
                           [os.path.join(NACL_TOP_DIR, 'build'),
                            os.path.join(NACL_TOP_DIR, 'buildbot'),
                            os.path.join(NACL_TOP_DIR, 'pynacl')]):
      # Buildbot and infrastructure changes should trigger all the try bots.
      has_pnacl = True
      has_toolchain_build = True
      has_others = True
      break
    elif IsFileInDirectories(file.AbsoluteLocalPath(),
                           [os.path.join(NACL_TOP_DIR, 'pnacl')]):
      has_pnacl = True
    elif IsFileInDirectories(file.AbsoluteLocalPath(),
                             [os.path.join(NACL_TOP_DIR, 'toolchain_build')]):
      has_toolchain_build = True
    else:
      has_others = True

  trybots = []
  if has_pnacl:
    trybots += PNACL_TOOLCHAIN_TRYBOTS
  if has_toolchain_build:
    trybots += TOOLCHAIN_BUILD_TRYBOTS
  if has_others:
    trybots += DEFAULT_TRYBOTS

  return {
    'tryserver.nacl': { t: set(['defaulttests']) for t in set(trybots) },
  }
