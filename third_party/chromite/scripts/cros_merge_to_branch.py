# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Developer helper tool for merging CLs from ToT to branches.

This simple program takes changes from gerrit/gerrit-int and creates new
changes for them on the desired branch using your gerrit/ssh credentials. To
specify a change on gerrit-int, you must prefix the change with a *.

Note that this script is best used from within an existing checkout of
Chromium OS that already has the changes you want merged to the branch in it
i.e. if you want to push changes to crosutils.git, you must have src/scripts
checked out. If this isn't true e.g. you are running this script from a
minilayout or trying to upload an internal change from a non internal checkout,
you must specify some extra options: use the --nomirror option and use -e to
specify your email address. This tool will then checkout the git repo fresh
using the credentials for the -e/email you specified and upload the change. Note
you can always use this method but it's slower than the "mirrored" method and
requires more typing :(.

Examples:
  cros_merge_to_branch 32027 32030 32031 release-R22.2723.B

This will create changes for 32027, 32030 and 32031 on the R22 branch. To look
up the name of a branch, go into a git sub-dir and type 'git branch -a' and the
find the branch you want to merge to. If you want to upload internal changes
from gerrit-int, you must prefix the gerrit change number with a * e.g.

  cros_merge_to_branch *26108 release-R22.2723.B

For more information on how to do this yourself you can go here:
http://dev.chromium.org/chromium-os/how-tos-and-troubleshooting/working-on-a-br\
anch
"""

from __future__ import print_function

import errno
import os
import re
import shutil
import sys
import tempfile

from chromite.lib import constants
from chromite.cbuildbot import repository
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gerrit
from chromite.lib import git
from chromite.lib import patch as cros_patch


def _GetParser():
  """Returns the parser to use for this module."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('-d', '--draft', default=False, action='store_true',
                      help='upload a draft to Gerrit rather than a change')
  parser.add_argument('-n', '--dry-run', default=False, action='store_true',
                      dest='dryrun',
                      help='apply changes locally but do not upload them')
  parser.add_argument('-e', '--email',
                      help='use this email instead of the email you would '
                           'upload changes as; required w/--nomirror')
  parser.add_argument('--nomirror', default=True, dest='mirror',
                      action='store_false',
                      help='checkout git repo directly; requires --email')
  parser.add_argument('--nowipe', default=True, dest='wipe',
                      action='store_false',
                      help='do not wipe the work directory after finishing')
  parser.add_argument('change', nargs='+', help='CLs to merge')
  parser.add_argument('branch', help='the branch to merge to')
  return parser


def _UploadChangeToBranch(work_dir, patch, branch, draft, dryrun):
  """Creates a new change from GerritPatch |patch| to |branch| from |work_dir|.

  Args:
    patch: Instance of GerritPatch to upload.
    branch: Branch to upload to.
    work_dir: Local directory where repository is checked out in.
    draft: If True, upload to refs/draft/|branch| rather than refs/for/|branch|.
    dryrun: Don't actually upload a change but go through all the steps up to
      and including git push --dry-run.

  Returns:
    A list of all the gerrit URLs found.
  """
  upload_type = 'drafts' if draft else 'for'
  # Download & setup the patch if need be.
  patch.Fetch(work_dir)
  # Apply the actual change.
  patch.CherryPick(work_dir, inflight=True, leave_dirty=True)

  # Get the new sha1 after apply.
  new_sha1 = git.GetGitRepoRevision(work_dir)
  reviewers = set()

  # Filter out tags that are added by gerrit and chromite.
  filter_re = re.compile(
      r'((Commit|Trybot)-Ready|Commit-Queue|(Reviewed|Submitted|Tested)-by): ')

  # Rewrite the commit message all the time.  Latest gerrit doesn't seem
  # to like it when you use the same ChangeId on different branches.
  msg = []
  for line in patch.commit_message.splitlines():
    if line.startswith('Reviewed-on: '):
      line = 'Previous-' + line
    elif filter_re.match(line):
      # If the tag is malformed, or the person lacks a name,
      # then that's just too bad -- throw it away.
      ele = re.split(r'[<>@]+', line)
      if len(ele) == 4:
        reviewers.add('@'.join(ele[-3:-1]))
      continue
    msg.append(line)
  msg += ['(cherry picked from commit %s)' % patch.sha1]
  git.RunGit(work_dir, ['commit', '--amend', '-F', '-'],
             input='\n'.join(msg).encode('utf8'))

  # Get the new sha1 after rewriting the commit message.
  new_sha1 = git.GetGitRepoRevision(work_dir)

  # Create and use a LocalPatch to Upload the change to Gerrit.
  local_patch = cros_patch.LocalPatch(
      work_dir, patch.project_url, constants.PATCH_BRANCH,
      patch.tracking_branch, patch.remote, new_sha1)
  for reviewers in (reviewers, ()):
    try:
      return local_patch.Upload(
          patch.project_url, 'refs/%s/%s' % (upload_type, branch),
          carbon_copy=False, dryrun=dryrun, reviewers=reviewers)
    except cros_build_lib.RunCommandError as e:
      if (e.result.returncode == 128 and
          re.search(r'fatal: user ".*?" not found', e.result.error)):
        logging.warning('Some reviewers were not found (%s); '
                        'dropping them & retrying upload', ' '.join(reviewers))
        continue
      raise


def _SetupWorkDirectoryForPatch(work_dir, patch, branch, manifest, email):
  """Set up local dir for uploading changes to the given patch's project."""
  logging.notice('Setting up dir %s for uploading changes to %s', work_dir,
                 patch.project_url)

  # Clone the git repo from reference if we have a pointer to a
  # ManifestCheckout object.
  reference = None
  if manifest:
    # Get the path to the first checkout associated with this change. Since
    # all of the checkouts share git objects, it doesn't matter which checkout
    # we pick.
    path = manifest.FindCheckouts(patch.project)[0]['path']

    reference = os.path.join(constants.SOURCE_ROOT, path)
    if not os.path.isdir(reference):
      logging.error('Unable to locate git checkout: %s', reference)
      logging.error('Did you mean to use --nomirror?')
      # This will do an "raise OSError" with the right values.
      os.open(reference, os.O_DIRECTORY)
    # Use the email if email wasn't specified.
    if not email:
      email = git.GetProjectUserEmail(reference)

  repository.CloneGitRepo(work_dir, patch.project_url, reference=reference)

  # Set the git committer.
  git.RunGit(work_dir, ['config', '--replace-all', 'user.email', email])

  mbranch = git.MatchSingleBranchName(
      work_dir, branch, namespace='refs/remotes/origin/')
  if branch != mbranch:
    logging.notice('Auto resolved branch name "%s" to "%s"', branch, mbranch)
  branch = mbranch

  # Finally, create a local branch for uploading changes to the given remote
  # branch.
  git.CreatePushBranch(
      constants.PATCH_BRANCH, work_dir, sync=False,
      remote_push_branch=git.RemoteRef('ignore', 'origin/%s' % branch))

  return branch


def _ManifestContainsAllPatches(manifest, patches):
  """Returns true if the given manifest contains all the patches.

  Args:
    manifest: an instance of git.Manifest
    patches: a collection of GerritPatch objects.
  """
  for patch in patches:
    if not manifest.FindCheckouts(patch.project):
      logging.error('Your manifest does not have the repository %s for '
                    'change %s. Please re-run with --nomirror and '
                    '--email set', patch.project, patch.gerrit_number)
      return False

    return True


def main(argv):
  parser = _GetParser()
  options = parser.parse_args(argv)
  changes = options.change
  branch = options.branch

  try:
    patches = gerrit.GetGerritPatchInfo(changes)
  except ValueError as e:
    logging.error('Invalid patch: %s', e)
    cros_build_lib.Die('Did you swap the branch/gerrit number?')

  # Suppress all logging info output unless we're running debug.
  if not options.debug:
    logging.getLogger().setLevel(logging.NOTICE)

  # Get a pointer to your repo checkout to look up the local project paths for
  # both email addresses and for using your checkout as a git mirror.
  manifest = None
  if options.mirror:
    try:
      manifest = git.ManifestCheckout.Cached(constants.SOURCE_ROOT)
    except OSError as e:
      if e.errno == errno.ENOENT:
        logging.error('Unable to locate ChromiumOS checkout: %s',
                      constants.SOURCE_ROOT)
        logging.error('Did you mean to use --nomirror?')
        return 1
      raise
    if not _ManifestContainsAllPatches(manifest, patches):
      return 1
  else:
    if not options.email:
      chromium_email = '%s@chromium.org' % os.environ['USER']
      logging.notice('--nomirror set without email, using %s', chromium_email)
      options.email = chromium_email

  index = 0
  work_dir = None
  root_work_dir = tempfile.mkdtemp(prefix='cros_merge_to_branch')
  try:
    for index, (change, patch) in enumerate(zip(changes, patches)):
      # We only clone the project and set the committer the first time.
      work_dir = os.path.join(root_work_dir, patch.project)
      if not os.path.isdir(work_dir):
        branch = _SetupWorkDirectoryForPatch(work_dir, patch, branch, manifest,
                                             options.email)

      # Now that we have the project checked out, let's apply our change and
      # create a new change on Gerrit.
      logging.notice('Uploading change %s to branch %s', change, branch)
      urls = _UploadChangeToBranch(work_dir, patch, branch, options.draft,
                                   options.dryrun)
      logging.notice('Successfully uploaded %s to %s', change, branch)
      for url in urls:
        if url.endswith('\x1b[K'):
          # Git will often times emit these escape sequences.
          url = url[0:-3]
        logging.notice('  URL: %s', url)

  except (cros_build_lib.RunCommandError, cros_patch.ApplyPatchException,
          git.AmbiguousBranchName, OSError) as e:
    # Tell the user how far we got.
    good_changes = changes[:index]
    bad_changes = changes[index:]

    logging.warning('############## SOME CHANGES FAILED TO UPLOAD ############')

    if good_changes:
      logging.notice(
          'Successfully uploaded change(s) %s', ' '.join(good_changes))

    # Printing out the error here so that we can see exactly what failed. This
    # is especially useful to debug without using --debug.
    logging.error('Upload failed with %s', str(e).strip())
    if not options.wipe:
      logging.error('Not wiping the directory. You can inspect the failed '
                    'change at %s; After fixing the change (if trivial) you can'
                    ' try to upload the change by running:\n'
                    'git commit -a -c CHERRY_PICK_HEAD\n'
                    'git push %s HEAD:refs/for/%s', work_dir, patch.project_url,
                    branch)
    else:
      logging.error('--nowipe not set thus deleting the work directory. If you '
                    'wish to debug this, re-run the script with change(s) '
                    '%s and --nowipe by running:\n  %s %s %s --nowipe',
                    ' '.join(bad_changes), sys.argv[0], ' '.join(bad_changes),
                    branch)

    # Suppress the stack trace if we're not debugging.
    if options.debug:
      raise
    else:
      return 1

  finally:
    if options.wipe:
      shutil.rmtree(root_work_dir)

  if options.dryrun:
    logging.notice('Success! To actually upload changes, re-run without '
                   '--dry-run.')
  else:
    logging.notice('Successfully uploaded all changes requested.')

  return 0
