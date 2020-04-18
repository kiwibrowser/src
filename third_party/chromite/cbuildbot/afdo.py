# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the various utilities to build Chrome with AFDO.

For a description of AFDO see gcc.gnu.org/wiki/AutoFDO.
"""

from __future__ import print_function

import bisect
import datetime
import glob
import os
import re

from chromite.lib import failures_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import timeout_util


# AFDO-specific constants.
# Chrome URL where AFDO data is stored.
_gsurls = {}
AFDO_CHROOT_ROOT = os.path.join('%(build_root)s', constants.DEFAULT_CHROOT_DIR)
AFDO_LOCAL_DIR = os.path.join('%(root)s', 'tmp')
AFDO_BUILDROOT_LOCAL = AFDO_LOCAL_DIR % {'root': AFDO_CHROOT_ROOT}
CHROME_ARCH_VERSION = '%(package)s-%(arch)s-%(version)s'
CHROME_PERF_AFDO_FILE = '%s.perf.data' % CHROME_ARCH_VERSION
CHROME_AFDO_FILE = '%s.afdo' % CHROME_ARCH_VERSION
CHROME_ARCH_RELEASE = '%(package)s-%(arch)s-%(release)s'
LATEST_CHROME_AFDO_FILE = 'latest-%s.afdo' % CHROME_ARCH_RELEASE
CHROME_DEBUG_BIN = os.path.join('%(root)s',
                                'build/%(board)s/usr/lib/debug',
                                'opt/google/chrome/chrome.debug')
# regex to find AFDO file for specific architecture within the ebuild file.
CHROME_EBUILD_AFDO_REGEX = (
    r'^(?P<bef>AFDO_FILE\["%s"\]=")(?P<name>.*)(?P<aft>")')
# and corresponding replacement string.
CHROME_EBUILD_AFDO_REPL = r'\g<bef>%s\g<aft>'

GSURL_BASE_BENCH = 'gs://chromeos-prebuilt/afdo-job/llvm'
GSURL_BASE_CWP = 'gs://chromeos-prebuilt/afdo-job/cwp/chrome'
GSURL_CHROME_PERF = os.path.join(GSURL_BASE_BENCH,
                                 CHROME_PERF_AFDO_FILE + '.bz2')
GSURL_CHROME_AFDO = os.path.join(GSURL_BASE_BENCH, CHROME_AFDO_FILE + '.bz2')
GSURL_LATEST_CHROME_AFDO = os.path.join(GSURL_BASE_BENCH,
                                        LATEST_CHROME_AFDO_FILE)
GSURL_CHROME_DEBUG_BIN = os.path.join(GSURL_BASE_BENCH,
                                      CHROME_ARCH_VERSION + '.debug.bz2')

AFDO_GENERATE_LLVM_PROF = '/usr/bin/create_llvm_prof'

# An AFDO data is considered stale when BOTH of the following two metrics don't
# meet. For example, if an AFDO data is generated 20 days ago but only 5 builds
# away, it is considered valid.

# How old can the AFDO data be? (in days).
AFDO_ALLOWED_STALE_DAYS = 14

# How old can the AFDO data be? (in difference of builds).
AFDO_ALLOWED_STALE_BUILDS = 7

# How old can the Kernel AFDO data be? (in days).
KERNEL_ALLOWED_STALE_DAYS = 28

# How old can the Kernel AFDO data be before sheriff got noticed? (in days).
KERNEL_WARN_STALE_DAYS = 14

# Set of boards that can generate the AFDO profile (can generate 'perf'
# data with LBR events). Currently, it needs to be a device that has
# at least 4GB of memory.
#
# This must be consistent with the definitions in autotest.
AFDO_DATA_GENERATORS_LLVM = ('chell', 'samus')

AFDO_ALERT_RECIPIENTS = [
    'chromeos-toolchain-sheriff@grotations.appspotmail.com']

KERNEL_PROFILE_URL = 'gs://chromeos-prebuilt/afdo-job/cwp/kernel/'
KERNEL_PROFILE_LS_PATTERN = '*/*.gcov.xz'
KERNEL_PROFILE_NAME_PATTERN = (
    r'([0-9]+\.[0-9]+)/R([0-9]+)-([0-9]+)\.([0-9]+)-([0-9]+)\.gcov\.xz')
KERNEL_PROFILE_MATCH_PATTERN = (
    r'^AFDO_PROFILE_VERSION="R[0-9]+-[0-9]+\.[0-9]+-[0-9]+"$')
KERNEL_PROFILE_WRITE_PATTERN = 'AFDO_PROFILE_VERSION="R%d-%d.%d-%d"'
KERNEL_EBUILD_ROOT = os.path.join(
    constants.SOURCE_ROOT,
    'src/third_party/chromiumos-overlay/sys-kernel'
)

GSURL_CWP_SUBDIR = {
    'silvermont': '',
    'airmont': 'airmont',
    'haswell': 'haswell',
    'broadwell': 'broadwell',
}


# Filename pattern of CWP profiles for Chrome
CWP_CHROME_PROFILE_NAME_PATTERN = r'R%s-%s.%s-%s.afdo.xz'


class MissingAFDOData(failures_lib.StepFailure):
  """Exception thrown when necessary AFDO data is missing."""


class MissingAFDOMarkers(failures_lib.StepFailure):
  """Exception thrown when necessary ebuild markers for AFDO are missing."""


class UnknownKernelVersion(failures_lib.StepFailure):
  """Exception thrown when the Kernel version can't be inferred."""


class NoValidProfileFound(failures_lib.StepFailure):
  """Exception thrown when there is no valid profile found."""


def CompressAFDOFile(to_compress, buildroot):
  """Compress file used by AFDO process.

  Args:
    to_compress: File to compress.
    buildroot: buildroot where to store the compressed data.

  Returns:
    Name of the compressed data file.
  """
  local_dir = AFDO_BUILDROOT_LOCAL % {'build_root': buildroot}
  dest = os.path.join(local_dir, os.path.basename(to_compress)) + '.bz2'
  cros_build_lib.CompressFile(to_compress, dest)
  return dest


def UncompressAFDOFile(to_decompress, buildroot):
  """Decompress file used by AFDO process.

  Args:
    to_decompress: File to decompress.
    buildroot: buildroot where to store the decompressed data.
  """
  local_dir = AFDO_BUILDROOT_LOCAL % {'build_root': buildroot}
  basename = os.path.basename(to_decompress)
  dest_basename = basename.rsplit('.', 1)[0]
  dest = os.path.join(local_dir, dest_basename)
  cros_build_lib.UncompressFile(to_decompress, dest)
  return dest


def GSUploadIfNotPresent(gs_context, src, dest):
  """Upload a file to GS only if the file does not exist.

  Will not generate an error if the file already exist in GS. It will
  only emit a warning.

  I could use GSContext.Copy(src,dest,version=0) here but it does not seem
  to work for large files. Using GSContext.Exists(dest) instead. See
  crbug.com/395858.

  Args:
    gs_context: GS context instance.
    src: File to copy.
    dest: Destination location.

  Returns:
    True if file was uploaded. False otherwise.
  """
  if gs_context.Exists(dest):
    logging.warning('File %s already in GS', dest)
    return False
  else:
    gs_context.Copy(src, dest, acl='public-read')
    return True


def GetAFDOPerfDataURL(cpv, arch):
  """Return the location URL for the AFDO per data file.

  Build the URL for the 'perf' data file given the release and architecture.

  Args:
    cpv: The portage_util.CPV object for chromeos-chrome.
    arch: architecture we're going to build Chrome for.

  Returns:
    URL of the location of the 'perf' data file.
  """

  # The file name of the perf data is based only in the chrome version.
  # The test case that produces it does not know anything about the
  # revision number.
  # TODO(llozano): perf data filename should include the revision number.
  version_number = cpv.version_no_rev.split('_')[0]
  chrome_spec = {'package': cpv.package,
                 'arch': arch,
                 'version': version_number}
  return GSURL_CHROME_PERF % chrome_spec


def CheckAFDOPerfData(cpv, arch, gs_context):
  """Check whether AFDO perf data exists for the given architecture.

  Check if 'perf' data file for this architecture and release is available
  in GS.

  Args:
    cpv: The portage_util.CPV object for chromeos-chrome.
    arch: architecture we're going to build Chrome for.
    gs_context: GS context to retrieve data.

  Returns:
    True if AFDO perf data is available. False otherwise.
  """
  url = GetAFDOPerfDataURL(cpv, arch)
  if not gs_context.Exists(url):
    logging.info('Could not find AFDO perf data at %s', url)
    return False

  logging.info('Found AFDO perf data at %s', url)
  return True


def WaitForAFDOPerfData(cpv, arch, buildroot, gs_context,
                        timeout=constants.AFDO_GENERATE_TIMEOUT):
  """Wait for AFDO perf data to show up (with an appropriate timeout).

  Wait for AFDO 'perf' data to show up in GS and copy it into a temp
  directory in the buildroot.

  Args:
    arch: architecture we're going to build Chrome for.
    cpv: CPV object for Chrome.
    buildroot: buildroot where AFDO data should be stored.
    gs_context: GS context to retrieve data.
    timeout: How long to wait total, in seconds.

  Returns:
    True if found the AFDO perf data before the timeout expired.
    False otherwise.
  """
  try:
    timeout_util.WaitForReturnTrue(
        CheckAFDOPerfData,
        func_args=(cpv, arch, gs_context),
        timeout=timeout, period=constants.SLEEP_TIMEOUT)
  except timeout_util.TimeoutError:
    logging.info('Could not find AFDO perf data before timeout')
    return False

  url = GetAFDOPerfDataURL(cpv, arch)
  dest_dir = AFDO_BUILDROOT_LOCAL % {'build_root': buildroot}
  dest_path = os.path.join(dest_dir, url.rsplit('/', 1)[1])
  gs_context.Copy(url, dest_path)

  UncompressAFDOFile(dest_path, buildroot)
  logging.info('Retrieved AFDO perf data to %s', dest_path)
  return True


def PatchChromeEbuildAFDOFile(ebuild_file, profiles):
  """Patch the Chrome ebuild with the dictionary of {arch: afdo_file} pairs.

  Args:
    ebuild_file: path of the ebuild file within the chroot.
    profiles: {source: afdo_file} pairs to put into the ebuild.
  """
  original_ebuild = path_util.FromChrootPath(ebuild_file)
  modified_ebuild = '%s.new' % original_ebuild

  patterns = {}
  repls = {}
  markers = {}
  for source in profiles.keys():
    patterns[source] = re.compile(CHROME_EBUILD_AFDO_REGEX % source)
    repls[source] = CHROME_EBUILD_AFDO_REPL % profiles[source]
    markers[source] = False

  with open(original_ebuild, 'r') as original:
    with open(modified_ebuild, 'w') as modified:
      for line in original:
        for source in profiles.keys():
          matched = patterns[source].match(line)
          if matched:
            markers[source] = True
            modified.write(patterns[source].sub(repls[source], line))
            break
        else: # line without markers, just copy it.
          modified.write(line)

  for source, found in markers.iteritems():
    if not found:
      raise MissingAFDOMarkers('Chrome ebuild file does not have appropriate '
                               'AFDO markers for source %s' % source)

  os.rename(modified_ebuild, original_ebuild)


def UpdateManifest(ebuild_file, ebuild_prog='ebuild'):
  """Regenerate the Manifest file.

  Args:
    ebuild_file: path to the ebuild file
    ebuild_prog: the ebuild command; can be board specific
  """
  gen_manifest_cmd = [ebuild_prog, ebuild_file, 'manifest', '--force']
  cros_build_lib.RunCommand(gen_manifest_cmd, enter_chroot=True, print_cmd=True)


def CommitIfChanged(ebuild_dir, message):
  """If there are changes to ebuild or Manifest, commit them.

  Args:
    ebuild_dir: the path to the directory of ebuild in the chroot
    message: commit message
  """
  # Check if anything changed compared to the previous version.
  modifications = git.RunGit(ebuild_dir,
                             ['status', '--porcelain', '-uno'],
                             capture_output=True, print_cmd=True).output
  if not modifications:
    logging.info('AFDO info for the ebuilds did not change. '
                 'Nothing to commit')
    return

  git.RunGit(ebuild_dir,
             ['commit', '-a', '-m', message],
             print_cmd=True)


def UpdateChromeEbuildAFDOFile(board, profiles):
  """Update chrome ebuild with the dictionary of {arch: afdo_file} pairs.

  Modifies the Chrome ebuild to set the appropriate AFDO file for each
  given architecture. Regenerates the associated Manifest file and
  commits the new ebuild and Manifest.

  Args:
    board: board we are building Chrome for.
    profiles: {arch: afdo_file} pairs to put into the ebuild.
              These are profiles from selected benchmarks.
  """
  # Find the Chrome ebuild file.
  equery_prog = 'equery'
  ebuild_prog = 'ebuild'
  if board:
    equery_prog += '-%s' % board
    ebuild_prog += '-%s' % board

  equery_cmd = [equery_prog, 'w', 'chromeos-chrome']
  ebuild_file = cros_build_lib.RunCommand(equery_cmd,
                                          enter_chroot=True,
                                          redirect_stdout=True).output.rstrip()

  # Patch the ebuild file with the names of the available afdo_files.
  PatchChromeEbuildAFDOFile(ebuild_file, profiles)

  # Also patch the 9999 ebuild. This is necessary because the uprev
  # process starts from the 9999 ebuild file and then compares to the
  # current version to see if the uprev is really necessary. We dont
  # want the names of the available afdo_files to show as differences.
  # It also allows developers to do USE=afdo_use when using the 9999
  # ebuild.
  ebuild_9999 = os.path.join(os.path.dirname(ebuild_file),
                             'chromeos-chrome-9999.ebuild')
  PatchChromeEbuildAFDOFile(ebuild_9999, profiles)

  UpdateManifest(ebuild_9999, ebuild_prog)

  ebuild_dir = path_util.FromChrootPath(os.path.dirname(ebuild_file))
  CommitIfChanged(ebuild_dir, 'Update profiles and manifests for Chrome.')


def VerifyLatestAFDOFile(afdo_release_spec, buildroot, gs_context):
  """Verify that the latest AFDO profile for a release is suitable.

  Find the latest AFDO profile file for a particular release and check
  that it is not too stale. The latest AFDO profile name for a release
  can be found in a file in GS under the name
  latest-chrome-<arch>-<release>.afdo.

  Args:
    afdo_release_spec: architecture and release to find the latest AFDO
        profile for.
    buildroot: buildroot where AFDO data should be stored.
    gs_context: GS context to retrieve data.

  Returns:
    The first return value is the name of the AFDO profile file found. None
    otherwise.
    The second return value indicates whether the profile found is expired or
    not. False when no profile is found.
  """
  latest_afdo_url = GSURL_LATEST_CHROME_AFDO % afdo_release_spec

  # Check if latest-chrome-<arch>-<release>.afdo exists.
  try:
    latest_detail = gs_context.List(latest_afdo_url, details=True)
  except gs.GSNoSuchKey:
    logging.info('Could not find latest AFDO info file %s', latest_afdo_url)
    return None, False

  # Then get the name of the latest valid AFDO profile file.
  local_dir = AFDO_BUILDROOT_LOCAL % {'build_root': buildroot}
  latest_afdo_file = LATEST_CHROME_AFDO_FILE % afdo_release_spec
  latest_afdo_path = os.path.join(local_dir, latest_afdo_file)
  gs_context.Copy(latest_afdo_url, latest_afdo_path)

  cand = osutils.ReadFile(latest_afdo_path).strip()
  cand_build = int(cand.split('.')[2])
  curr_build = int(afdo_release_spec['build'])

  # Verify the AFDO profile file is not too stale.
  mod_date = latest_detail[0].creation_time
  curr_date = datetime.datetime.now()
  allowed_stale_days = datetime.timedelta(days=AFDO_ALLOWED_STALE_DAYS)
  if ((curr_date - mod_date) > allowed_stale_days and
      (curr_build - cand_build) > AFDO_ALLOWED_STALE_BUILDS):
    logging.info('Found latest AFDO info file %s but it is too old',
                 latest_afdo_url)
    return cand, True

  return cand, False


def GetBenchmarkProfile(cpv, _source, buildroot, gs_context):
  """Try to find the latest suitable AFDO profile file.

  Try to find the latest AFDO profile generated for current release
  and architecture. If there is none, check the previous release (mostly
  in case we have just branched).

  Args:
    cpv: cpv object for Chrome.
    source: benchmark source for which we are looking
    buildroot: buildroot where AFDO data should be stored.
    gs_context: GS context to retrieve data.

  Returns:
    Name of latest suitable AFDO profile file if one is found.
    None otherwise.
  """

  # Currently, benchmark based profiles can only be generated on amd64.
  arch = 'amd64'
  version_numbers = cpv.version.split('.')
  current_release = version_numbers[0]
  current_build = version_numbers[2]
  afdo_release_spec = {'package': cpv.package,
                       'arch': arch,
                       'release': current_release,
                       'build': current_build}
  afdo_file, expired = VerifyLatestAFDOFile(afdo_release_spec, buildroot,
                                            gs_context)
  if afdo_file and not expired:
    return afdo_file

  # The profile found in current release is too old. This clearly is a sign of
  # problem. Therefore, don't try to find another one in previous branch.
  if expired:
    return None

  # Could not find suitable AFDO file for the current release.
  # Let's see if there is one from the previous release.
  previous_release = str(int(current_release) - 1)
  prev_release_spec = {'package': cpv.package,
                       'arch': arch,
                       'release': previous_release,
                       'build': current_build}
  afdo_file, expired = VerifyLatestAFDOFile(prev_release_spec, buildroot,
                                            gs_context)
  if expired:
    return None
  return afdo_file


def GenerateAFDOData(cpv, arch, board, buildroot, gs_context):
  """Generate AFDO profile data from 'perf' data.

  Given the 'perf' profile, generate an AFDO profile using create_llvm_prof.
  It also creates a latest-chrome-<arch>-<release>.afdo file pointing
  to the generated AFDO profile.
  Uploads the generated data to GS for retrieval by the chrome ebuild
  file when doing an 'afdo_use' build.
  It is possible the generated data has previously been uploaded to GS
  in which case this routine will not upload the data again. Uploading
  again may cause verication failures for the ebuild file referencing
  the previous contents of the data.

  Args:
    cpv: cpv object for Chrome.
    arch: architecture for which we are looking for AFDO profile.
    board: board we are building for.
    buildroot: buildroot where AFDO data should be stored.
    gs_context: GS context to retrieve/store data.

  Returns:
    Name of the AFDO profile file generated if successful.
  """
  CHROME_UNSTRIPPED_NAME = 'chrome.unstripped'

  version_number = cpv.version
  afdo_spec = {'package': cpv.package,
               'arch': arch,
               'version': version_number}
  chroot_root = AFDO_CHROOT_ROOT % {'build_root': buildroot}
  local_dir = AFDO_LOCAL_DIR % {'root': chroot_root}
  in_chroot_local_dir = AFDO_LOCAL_DIR % {'root': ''}

  # Upload compressed chrome debug binary to GS for triaging purposes.
  # TODO(llozano): This simplifies things in case of need of triaging
  # problems but is it really necessary?
  debug_bin = CHROME_DEBUG_BIN % {'root': chroot_root,
                                  'board': board}
  comp_debug_bin_path = CompressAFDOFile(debug_bin, buildroot)
  GSUploadIfNotPresent(gs_context, comp_debug_bin_path,
                       GSURL_CHROME_DEBUG_BIN % afdo_spec)

  # create_llvm_prof demands the name of the profiled binary exactly matches
  # the name of the unstripped binary or it is named 'chrome.unstripped'.
  # So create a symbolic link with the appropriate name.
  local_debug_sym = os.path.join(local_dir, CHROME_UNSTRIPPED_NAME)
  in_chroot_debug_bin = CHROME_DEBUG_BIN % {'root': '', 'board': board}
  osutils.SafeUnlink(local_debug_sym)
  os.symlink(in_chroot_debug_bin, local_debug_sym)

  # Call create_llvm_prof tool to generated AFDO profile from 'perf' profile
  # and upload it to GS. Need to call from within chroot since this tool
  # was built inside chroot.
  debug_sym = os.path.join(in_chroot_local_dir, CHROME_UNSTRIPPED_NAME)
  # The name of the 'perf' file is based only on the version of chrome. The
  # revision number is not included.
  afdo_spec_no_rev = {'package': cpv.package,
                      'arch': arch,
                      'version': cpv.version_no_rev.split('_')[0]}
  perf_afdo_file = CHROME_PERF_AFDO_FILE % afdo_spec_no_rev
  perf_afdo_path = os.path.join(in_chroot_local_dir, perf_afdo_file)
  afdo_file = CHROME_AFDO_FILE % afdo_spec
  afdo_path = os.path.join(in_chroot_local_dir, afdo_file)
  afdo_cmd = [AFDO_GENERATE_LLVM_PROF,
              '--binary=%s' % debug_sym,
              '--profile=%s' % perf_afdo_path,
              '--out=%s' % afdo_path]
  cros_build_lib.RunCommand(afdo_cmd, enter_chroot=True, capture_output=True,
                            print_cmd=True)

  afdo_local_path = os.path.join(local_dir, afdo_file)
  comp_afdo_path = CompressAFDOFile(afdo_local_path, buildroot)
  uploaded_afdo_file = GSUploadIfNotPresent(gs_context, comp_afdo_path,
                                            GSURL_CHROME_AFDO % afdo_spec)

  if uploaded_afdo_file:
    # Create latest-chrome-<arch>-<release>.afdo pointing to the name
    # of the AFDO profile file and upload to GS.
    current_release = version_number.split('.')[0]
    afdo_release_spec = {'package': cpv.package,
                         'arch': arch,
                         'release': current_release}
    latest_afdo_file = LATEST_CHROME_AFDO_FILE % afdo_release_spec
    latest_afdo_path = os.path.join(local_dir, latest_afdo_file)
    osutils.WriteFile(latest_afdo_path, afdo_file)
    gs_context.Copy(latest_afdo_path,
                    GSURL_LATEST_CHROME_AFDO % afdo_release_spec,
                    acl='public-read')

  return afdo_file


def CanGenerateAFDOData(board):
  """Does this board has the capability of generating its own AFDO data?."""
  return board in AFDO_DATA_GENERATORS_LLVM


def FindLatestProfile(target, versions):
  """Find latest profile that is usable by the target.

  Args:
    target: the target version
    versions: a list of versions

  Returns:
    latest profile that is older than the target
  """
  cand = bisect.bisect(versions, target) - 1
  if cand >= 0:
    return versions[cand]
  return None


def PatchKernelEbuild(filename, version):
  """Update the AFDO_PROFILE_VERSION string in the given kernel ebuild file.

  Args:
    filename: name of the ebuild
    version: e.g., [61, 9752, 0, 0]
  """
  contents = []
  for line in osutils.ReadFile(filename).splitlines():
    if re.match(KERNEL_PROFILE_MATCH_PATTERN, line):
      contents.append(KERNEL_PROFILE_WRITE_PATTERN % tuple(version) + '\n')
    else:
      contents.append(line + '\n')
  osutils.WriteFile(filename, contents, atomic=True)


def CWPProfileToVersionTuple(url):
  """Convert a CWP profile url to a version tuple

  Args:
    url: for example, gs://chromeos-prebuilt/afdo-job/cwp/chrome/
                      R65-3325.65-1519323840.afdo.xz

  Returns:
    A tuple of (milestone, major, minor, timestamp)
  """
  fn_mat = (CWP_CHROME_PROFILE_NAME_PATTERN %
            tuple(r'([0-9]+)' for _ in xrange(0, 4)))
  fn_mat.replace('.', '\\.')
  return map(int, re.match(fn_mat, os.path.basename(url)).groups())


def GetCWPProfile(cpv, source, _buildroot, gs_context):
  """Try to find the latest suitable AFDO profile file for cwp.

  Try to find the latest AFDO profile generated for current release
  and architecture.

  Args:
    cpv: cpv object for Chrome.
    source: profile source
    buildroot: buildroot where AFDO data should be stored.
    gs_context: GS context to retrieve data.

  Returns:
    Name of latest suitable AFDO profile file if one is found.
    None otherwise.
  """
  ver_mat = r'([0-9]+)\.[0-9]+\.([0-9]+)\.([0-9]+)_rc-r[0-9]+'
  target = map(int, re.match(ver_mat, cpv.version).groups())

  # Check 2 most recent milestones.
  #
  # When a branch just happens, the milestone of master increases by 1. There
  # will be no profile from that milestone until a dev release is pushed for a
  # short period of time. Therefore, a profile from previous branches must be
  # picked instead.
  #
  # Originally, we search toward root in the branch tree for a profile. Now we
  # prefer to look at the previous milestone if there's no profile from current
  # milestone, because:
  #
  # 1. dev channel has few samples. The profile quality is much better from
  #    beta, which is always in a branch.
  #
  # 2. Master is actually closer to the branch tip than to the branch point,
  #    assuming that most of the changes on a branch are cherry-picked from
  #    master.
  versions = []
  for milestone in (target[0], target[0] - 1):
    gs_ls_url = os.path.join(GSURL_BASE_CWP, GSURL_CWP_SUBDIR[source],
                             CWP_CHROME_PROFILE_NAME_PATTERN %
                             (milestone, '*', '*', '*'))
    try:
      res = gs_context.List(gs_ls_url)
      versions += map(CWPProfileToVersionTuple, [r.url for r in res])
    except gs.GSNoSuchKey:
      pass

  if not versions:
    logging.info('profile not found for: %s', cpv.version)
    return None

  versions.sort()
  cand = FindLatestProfile(target, versions)
  # reconstruct the filename and strip .xz
  return (CWP_CHROME_PROFILE_NAME_PATTERN % tuple(cand))[:-3]


def GetAvailableKernelProfiles():
  """Get available profiles on specified gsurl.

  Returns:
    a dictionary that maps kernel version, e.g. "4_4" to a list of
    [milestone, major, minor, timestamp]. E.g,
    [62, 9901, 21, 1506581147]
  """

  gs_context = gs.GSContext()
  gs_ls_url = os.path.join(KERNEL_PROFILE_URL, KERNEL_PROFILE_LS_PATTERN)
  gs_match_url = os.path.join(KERNEL_PROFILE_URL, KERNEL_PROFILE_NAME_PATTERN)
  try:
    res = gs_context.List(gs_ls_url)
  except gs.GSNoSuchKey:
    logging.info('gs files not found: %s', gs_ls_url)
    return {}

  matches = filter(None, [re.match(gs_match_url, p.url) for p in res])
  versions = {}
  for m in matches:
    versions.setdefault(m.group(1), []).append(map(int, m.groups()[1:]))
  for v in versions:
    versions[v].sort()
  return versions


def FindKernelEbuilds():
  """Find all ebuilds that specify AFDO_PROFILE_VERSION.

  The only assumption is that the ebuild files are named as the match pattern
  in kver(). If it fails to recognize the ebuild filename, an error will be
  thrown.

  equery is not used because that would require enumerating the boards, which
  is no easier than enumerating the kernel versions or ebuilds.

  Returns:
    a list of (ebuilds, kernel rev)
  """
  def kver(ebuild):
    matched = re.match(r'.*/chromeos-kernel-([0-9]+_[0-9]+)-.+\.ebuild$',
                       ebuild)
    if matched:
      return matched.group(1).replace('_', '.')
    raise UnknownKernelVersion(
        'Kernel version cannot be inferred from ebuild filename "%s".' % ebuild)

  for fn in glob.glob(os.path.join(KERNEL_EBUILD_ROOT, '*', '*.ebuild')):
    for line in osutils.ReadFile(fn).splitlines():
      if re.match(KERNEL_PROFILE_MATCH_PATTERN, line):
        yield (fn, kver(fn))
        break


def ProfileAge(profile_version):
  """Tell the age of profile_version in days.

  Args:
    profile_version: [chrome milestone, cros major, cros minor, timestamp]
                     e.g., [61, 9752, 0, 1500000000]

  Returns:
    Age of profile_version in days.
  """
  return (datetime.datetime.utcnow() -
          datetime.datetime.utcfromtimestamp(profile_version[3])).days


PROFILE_SOURCES = {
    'benchmark': GetBenchmarkProfile,
    'silvermont': GetCWPProfile,
    'airmont': GetCWPProfile,
    'haswell': GetCWPProfile,
    'broadwell': GetCWPProfile,
}
