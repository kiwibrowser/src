# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cros pinchrome: Pin chrome to an earlier version."""

from __future__ import print_function

import fnmatch
import glob
import os
import re
import shutil
import tempfile

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.cbuildbot import repository
from chromite.cli import command
from chromite.lib import cros_build_lib
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import portage_util
from chromite.scripts import cros_mark_as_stable


site_config = config_lib.GetConfig()


class UprevNotFound(Exception):
  """Exception to throw when no Chrome Uprev CL is found."""


# git utility functions.


def CloneWorkingRepo(dest, url, reference, branch):
  """Clone a git repository with an existing local copy as a reference.

  Also copy the hooks into the new repository.

  Args:
    dest: The directory to clone int.
    url: The URL of the repository to clone.
    reference: Local checkout to draw objects from.
    branch: The branch to clone.
  """
  repository.CloneGitRepo(dest, url, reference=reference,
                          single_branch=True, branch=branch)
  for name in glob.glob(os.path.join(reference, '.git', 'hooks', '*')):
    newname = os.path.join(dest, '.git', 'hooks', os.path.basename(name))
    shutil.copyfile(name, newname)
    shutil.copystat(name, newname)


# Portage utilities.

def UpdateManifest(ebuild):
  """Update the manifest for an ebuild.

  Args:
    ebuild: Path to the ebuild to update the manifest for.
  """
  ebuild = path_util.ToChrootPath(os.path.realpath(ebuild))
  cros_build_lib.RunCommand(['ebuild', ebuild, 'manifest'], quiet=True,
                            enter_chroot=True)


def SplitPVPath(path):
  """Utility function to run both SplitEbuildPath and SplitPV.

  Args:
    path: Ebuild path to run those functions on.

  Returns:
    The output of SplitPV.
  """
  return portage_util.SplitPV(portage_util.SplitEbuildPath(path)[2])


def RevertStableEBuild(dirname, rev):
  """Revert the stable ebuilds for a package back to a particular revision.

  Also add/remove the files in git.

  Args:
    dirname: Path to the ebuild directory.
    rev: Revision to revert back to.

  Returns:
    The name of the ebuild reverted to.
  """
  package = os.path.basename(dirname.rstrip(os.sep))
  pattern = '%s-*.ebuild' % package

  # Get rid of existing stable ebuilds.
  ebuilds = glob.glob(os.path.join(dirname, pattern))
  for ebuild in ebuilds:
    parts = SplitPVPath(ebuild)
    if parts.version != '9999':
      git.RmPath(ebuild)

  # Bring back the old stable ebuild.
  names = git.GetObjectAtRev(dirname, './', rev).split()
  names = fnmatch.filter(names, pattern)
  names = [name for name in names
           if SplitPVPath(os.path.join(dirname, name)).version != '9999']
  if not names:
    return None
  assert len(names) == 1
  name = names[0]
  git.RevertPath(dirname, name, rev)

  # Update the manifest.
  UpdateManifest(os.path.join(dirname, name))
  manifest_path = os.path.join(dirname, 'Manifest')
  if os.path.exists(manifest_path):
    git.AddPath(manifest_path)
  return os.path.join(dirname, name)


def RevertBinhostConf(overlay, conf_files, rev):
  """Revert binhost config files back to a particular revision.

  Args:
    overlay: The overlay holding the binhost config files.
    conf_files: A list of config file names.
    rev: The revision to revert back to.
  """
  binhost_dir = os.path.join(overlay, 'chromeos', 'binhost')
  for conf_file in conf_files:
    try:
      git.RevertPath(os.path.join(binhost_dir, 'target'), conf_file, rev)
    except Exception as e1:
      try:
        git.RevertPath(os.path.join(binhost_dir, 'host'), conf_file, rev)
      except Exception as e2:
        raise Exception(str(e1) + '\n' + str(e2))


def MaskNewerPackages(overlay, ebuilds):
  """Mask ebuild versions newer than the ones passed in.

  This creates a new mask file called chromepin which masks ebuilds newer than
  the ones passed in. To undo the masking, just delete that file. The
  mask file is added with git.

  Args:
    overlay: The overlay that will hold the mask file.
    ebuilds: List of ebuilds to set up masks for.
  """
  content = '# Pin chrome by masking more recent versions.\n'
  for ebuild in ebuilds:
    parts = portage_util.SplitEbuildPath(ebuild)
    content += '>%s\n' % os.path.join(parts[0], parts[2])
  mask_file = os.path.join(overlay, MASK_FILE)
  osutils.WriteFile(mask_file, content)
  git.AddPath(mask_file)


# Tools to pick the point right before an uprev to pin chrome to and get
# information about it.

CONF_RE = re.compile(
    r'^\s*(?P<conf>[^:\n]+): updating LATEST_RELEASE_CHROME_BINHOST',
    flags=re.MULTILINE)


# Interesting paths.
OVERLAY = os.path.join(constants.SOURCE_ROOT,
                       constants.CHROMIUMOS_OVERLAY_DIR)
OVERLAY_URL = (site_config.params.EXTERNAL_GOB_URL +
               '/chromiumos/overlays/chromiumos-overlay')
PRIV_OVERLAY = os.path.join(constants.SOURCE_ROOT, 'src',
                            'private-overlays',
                            'chromeos-partner-overlay')
PRIV_OVERLAY_URL = (site_config.params.INTERNAL_GOB_URL +
                    '/chromeos/overlays/chromeos-partner-overlay')
MASK_FILE = os.path.join('profiles', 'default', 'linux',
                         'package.mask', 'chromepin')


class ChromeUprev(object):
  """A class to represent Chrome uprev CLs in the public overlay."""

  def __init__(self, ebuild_dir, before=None):
    """Construct a Chrome uprev object

    Args:
      ebuild_dir: Path to the directory with the chrome ebuild in it.
      before: CL to work backwards from.
    """
    # Format includes the hash, commit body including subject, and author date.
    cmd = ['log', '-n', '1', '--author', 'chrome-bot', '--grep',
           cros_mark_as_stable.GIT_COMMIT_SUBJECT,
           '--format=format:%H%n%aD%n%B']
    if before:
      cmd.append(str(before) + '~')
    cmd.append('.')
    log = git.RunGit(ebuild_dir, cmd).output
    if not log.strip():
      raise UprevNotFound('No uprev CL was found')

    self.sha, _, log = log.partition('\n')
    self.date, _, message = log.partition('\n')
    self.conf_files = [m.group('conf') for m in CONF_RE.finditer(message)]

    entries = git.RawDiff(ebuild_dir, '%s^!' % self.sha)
    for entry in entries:
      if entry.status != 'R':
        continue

      from_path = entry.src_file
      to_path = entry.dst_file

      if (os.path.splitext(from_path)[1] != '.ebuild' or
          os.path.splitext(to_path)[1] != '.ebuild'):
        continue

      self.from_parts = SplitPVPath(from_path)
      self.to_parts = SplitPVPath(to_path)
      if (self.from_parts.package != 'chromeos-chrome' or
          self.to_parts.package != 'chromeos-chrome'):
        continue

      break
    else:
      raise Exception('Failed to find chromeos-chrome uprev in CL %s' %
                      self.sha)


class UprevList(object):
  """A generator which returns chrome uprev CLs in a particular repository.

  It also keeps track of what CLs have been presented so the one the user
  chose can be retrieved.
  """

  def __init__(self, chrome_path):
    """Initialize the class.

    Args:
      chrome_path: The path to the repository to search.
    """
    self.uprevs = []
    self.last = None
    self.chrome_path = chrome_path

  def __iter__(self):
    return self

  def __next__(self):
    return self.next()

  def next(self):
    before = self.last.sha if self.last else None
    try:
      self.last = ChromeUprev(self.chrome_path, before=before)
    except UprevNotFound:
      raise StopIteration()
    ver = self.last.from_parts.version + ' (%s)' % self.last.date
    self.uprevs.append(self.last)
    return ver


# Tools to find the binhost updates in the private overlay which go with the
# ones in the public overlay.

class BinHostUprev(object):
  """Class which represents an uprev CL for the private binhost configs."""

  def __init__(self, sha, log):
    self.sha = sha
    self.conf_files = [m.group('conf') for m in CONF_RE.finditer(log)]


def FindPrivateConfCL(overlay, pkg_dir):
  """Find the private binhost uprev CL which goes with the public one.

  Args:
    overlay: Path to the private overlay.
    pkg_dir: What the package directory should contain to be considered a
             match.

  Returns:
    A BinHostUprev object representing the CL.
  """
  binhost_dir = os.path.join(overlay, 'chromeos', 'binhost')
  before = None

  plus_package_re = re.compile(r'^\+.*%s' % re.escape(pkg_dir),
                               flags=re.MULTILINE)

  while True:
    cmd = ['log', '-n', '1', '--grep', 'LATEST_RELEASE_CHROME_BINHOST',
           '--format=format:%H']
    if before:
      cmd.append('%s~' % before)
    cmd.append('.')
    sha = git.RunGit(binhost_dir, cmd).output.strip()
    if not sha:
      return None

    cl = git.RunGit(overlay, ['show', '-M', sha]).output

    if plus_package_re.search(cl):
      return BinHostUprev(sha, cl)

    before = sha


# The main attraction.

@command.CommandDecorator('pinchrome')
class PinchromeCommand(command.CliCommand):
  # pylint: disable=docstring-too-many-newlines
  """Pin chrome to an earlier revision.


  Pinning procedure:

  When pinning chrome, this script first looks through the history of the
  public overlay repository looking for changes which upreved chrome. It shows
  the user what versions chrome has been at recently and when the uprevs
  happened, and lets the user pick a point in that history to go back to.

  Once an old version has been selected, the script creates a change which
  overwrites the chrome ebuild(s) and binhost config files to what they were
  at that version in the public overlay. It also adds entries to the portage
  mask files to prevent newer versions from being installed.

  Next, the script looks for a version of the binhost config file in the
  private overlay directory which corresponds to the one in the public overlay.
  It creates a change which overwrites the binhost config similar to above.

  For safety, these two changes have CQ-DEPEND added to them and refer to each
  other. The script uploads them, expecting the user to go to their review
  pages and send them on their way.


  Unpinning procedure:

  To unpin, this script simply deletes the entries in the portage mask files
  added above. After that, the Chrome PFQ can uprev chrome normally,
  overwriting the ebuilds and binhost configs.
  """

  def __init__(self, options):
    super(PinchromeCommand, self).__init__(options)

    # Make up a branch name which is unlikely to collide.
    self.branch_name = 'chrome_pin_' + cros_build_lib.GetRandomString()

  @classmethod
  def AddParser(cls, parser):
    super(cls, PinchromeCommand).AddParser(parser)
    parser.add_argument('--unpin', help='Unpin chrome.', default=False,
                        action='store_true')
    parser.add_argument('--bug', help='Used in the "BUG" field of CLs.',
                        required=True)
    parser.add_argument('--branch', default='master',
                        help='The branch to pin chrome on (default master).')
    parser.add_argument('--nowipe', help='Preserve the working directory',
                        default=True, dest='wipe', action='store_false')
    parser.add_argument('--dryrun', action='store_true',
                        help='Prepare pinning CLs but don\'t upload them')

  def CommitMessage(self, subject, cq_depend=None, change_id=None):
    """Generate a commit message

    Args:
      subject: The subject of the message.
      cq_depend: An optional CQ-DEPEND target.
      change_id: An optional change ID.

    Returns:
      The commit message.
    """
    message = [
        '%s' % subject,
        '',
        'DO NOT REVERT THIS CL.',
        'In general, reverting chrome (un)pin CLs does not do what you expect.',
        'Instead, use `cros pinchrome` to generate new CLs.',
        '',
        'BUG=%s' % self.options.bug,
        'TEST=None',
    ]
    if cq_depend:
      message += ['CQ-DEPEND=%s' % cq_depend]
    if change_id:
      message += [
          '',
          'Change-Id: %s' % change_id,
      ]

    return '\n'.join(message)

  def unpin(self, work_dir):
    """Unpin chrome."""

    overlay = os.path.join(work_dir, 'overlay')
    print('Setting up working directory...')
    CloneWorkingRepo(overlay, OVERLAY_URL, OVERLAY, self.options.branch)
    print('Done')

    mask_file = os.path.join(overlay, MASK_FILE)
    if not os.path.exists(mask_file):
      raise Exception('Mask file not found. Is Chrome pinned?')

    git.CreateBranch(overlay, self.branch_name, track=True,
                     branch_point='origin/%s' % self.options.branch)

    git.RmPath(mask_file)
    git.Commit(overlay, self.CommitMessage('Chrome: Unpin chrome'))
    git.UploadCL(overlay, OVERLAY_URL, self.options.branch,
                 skip=self.options.dryrun)

  def pin(self, work_dir):
    """Pin chrome."""

    overlay = os.path.join(work_dir, 'overlay')
    priv_overlay = os.path.join(work_dir, 'priv_overlay')
    print('Setting up working directory...')
    CloneWorkingRepo(overlay, OVERLAY_URL, OVERLAY, self.options.branch)
    CloneWorkingRepo(priv_overlay, PRIV_OVERLAY_URL, PRIV_OVERLAY,
                     self.options.branch)
    print('Done')

    # Interesting paths.
    chrome_dir = os.path.join(overlay, constants.CHROME_CP)
    other_dirs = [os.path.join(overlay, pkg) for pkg in
                  constants.OTHER_CHROME_PACKAGES]

    # Let the user pick what version to pin chrome to.
    uprev_list = UprevList(chrome_dir)
    choice = cros_build_lib.GetChoice('Versions of chrome to pin to:',
                                      uprev_list, group_size=5)
    pin_version = uprev_list.uprevs[choice]
    commit_subject = ('Chrome: Pin to version %s' %
                      pin_version.from_parts.version)

    # Public branch.
    git.CreateBranch(overlay, self.branch_name, track=True,
                     branch_point='origin/%s' % self.options.branch)

    target_sha = pin_version.sha + '~'
    ebs = [RevertStableEBuild(chrome_dir, target_sha)]
    for pkg_dir in other_dirs:
      ebs.append(RevertStableEBuild(pkg_dir, target_sha))
    RevertBinhostConf(overlay, pin_version.conf_files, target_sha)
    git.RevertPath(os.path.join(overlay, 'chromeos', 'binhost'),
                   'chromium.json', target_sha)
    MaskNewerPackages(overlay, (eb for eb in ebs if eb))

    pub_cid = git.Commit(overlay, 'Public overlay commit')
    if not pub_cid:
      raise Exception('Don\'t know the commit ID of the public overlay CL.')

    # Find out what package directory the binhost configs should point to.
    binhost_dir = os.path.join(overlay, 'chromeos', 'binhost')
    target_file = os.path.join(binhost_dir, 'target', pin_version.conf_files[0])
    host_file = os.path.join(binhost_dir, 'host', pin_version.conf_files[0])
    conf_file = target_file if os.path.exists(target_file) else host_file
    conf_content = osutils.ReadFile(conf_file)
    match = re.search('/(?P<package_dir>[^/\n]*)/packages', conf_content)
    if not match:
      raise Exception('Failed to parse binhost conf %s' % conf_content.strip())
    pkg_dir = match.group('package_dir')

    # Private branch.
    git.CreateBranch(priv_overlay, self.branch_name, track=True,
                     branch_point='origin/%s' % self.options.branch)

    binhost_uprev = FindPrivateConfCL(priv_overlay, pkg_dir)
    if not binhost_uprev:
      raise Exception('Failed to find private binhost uprev.')
    target_sha = binhost_uprev.sha
    RevertBinhostConf(priv_overlay, binhost_uprev.conf_files, target_sha)
    git.RevertPath(os.path.join(priv_overlay, 'chromeos', 'binhost'),
                   'chrome.json', target_sha)

    commit_message = self.CommitMessage(commit_subject, pub_cid)
    priv_cid = git.Commit(priv_overlay, commit_message)
    if not priv_cid:
      raise Exception('Don\'t know the commit ID of the private overlay CL.')

    # Update the commit message on the public overlay CL.
    commit_message = self.CommitMessage(commit_subject, '*' + priv_cid, pub_cid)
    git.Commit(overlay, commit_message, amend=True)

    # Upload the CLs.
    external_push = git.UploadCL(overlay, OVERLAY_URL, self.options.branch,
                                 skip=self.options.dryrun)
    print(external_push.output)
    internal_push = git.UploadCL(priv_overlay, PRIV_OVERLAY_URL,
                                 self.options.branch, skip=self.options.dryrun)
    print(internal_push.output)

    print('\n** Both of the changes above need to be submitted for chrome '
          'to be pinned. **\n')

  def Run(self):
    """Run cros pinchrome."""
    self.options.Freeze()
    chroot_tmp = os.path.join(constants.SOURCE_ROOT,
                              constants.DEFAULT_CHROOT_DIR, 'tmp')
    tmp_override = None if cros_build_lib.IsInsideChroot() else chroot_tmp
    work_dir = tempfile.mkdtemp(prefix='pinchrome_', dir=tmp_override)
    try:
      if self.options.unpin:
        self.unpin(work_dir)
      else:
        self.pin(work_dir)
    finally:
      if self.options.wipe:
        osutils.RmDir(work_dir)
      else:
        print('Leaving working directory at %s.' % work_dir)
