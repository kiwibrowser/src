#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Splits a branch into smaller branches and uploads CLs."""

import collections
import os
import re
import subprocess2
import sys
import tempfile

import git_footers
import owners
import owners_finder

import git_common as git


def ReadFile(file_path):
  """Returns the content of |file_path|."""
  with open(file_path) as f:
    content = f.read()
  return content


def EnsureInGitRepository():
  """Throws an exception if the current directory is not a git repository."""
  git.run('rev-parse')


def CreateBranchForDirectory(prefix, directory, upstream):
  """Creates a branch named |prefix| + "_" + |directory| + "_split".

  Return false if the branch already exists. |upstream| is used as upstream for
  the created branch.
  """
  existing_branches = set(git.branches(use_limit = False))
  branch_name = prefix + '_' + directory + '_split'
  if branch_name in existing_branches:
    return False
  git.run('checkout', '-t', upstream, '-b', branch_name)
  return True


def FormatDescriptionOrComment(txt, directory):
  """Replaces $directory with |directory| in |txt|."""
  return txt.replace('$directory', '/' + directory)


def AddUploadedByGitClSplitToDescription(description):
  """Adds a 'This CL was uploaded by git cl split.' line to |description|.

  The line is added before footers, or at the end of |description| if it has no
  footers.
  """
  split_footers = git_footers.split_footers(description)
  lines = split_footers[0]
  if not lines[-1] or lines[-1].isspace():
    lines = lines + ['']
  lines = lines + ['This CL was uploaded by git cl split.']
  if split_footers[1]:
    lines += [''] + split_footers[1]
  return '\n'.join(lines)


def UploadCl(refactor_branch, refactor_branch_upstream, directory, files,
             description, comment, reviewers, changelist, cmd_upload):
  """Uploads a CL with all changes to |files| in |refactor_branch|.

  Args:
    refactor_branch: Name of the branch that contains the changes to upload.
    refactor_branch_upstream: Name of the upstream of |refactor_branch|.
    directory: Path to the directory that contains the OWNERS file for which
        to upload a CL.
    files: List of AffectedFile instances to include in the uploaded CL.
    description: Description of the uploaded CL.
    comment: Comment to post on the uploaded CL.
    reviewers: A set of reviewers for the CL.
    changelist: The Changelist class.
    cmd_upload: The function associated with the git cl upload command.
  """
  # Create a branch.
  if not CreateBranchForDirectory(
      refactor_branch, directory, refactor_branch_upstream):
    print 'Skipping ' + directory + ' for which a branch already exists.'
    return

  # Checkout all changes to files in |files|.
  deleted_files = [f.AbsoluteLocalPath() for f in files if f.Action() == 'D']
  if deleted_files:
    git.run(*['rm'] + deleted_files)
  modified_files = [f.AbsoluteLocalPath() for f in files if f.Action() != 'D']
  if modified_files:
    git.run(*['checkout', refactor_branch, '--'] + modified_files)

  # Commit changes. The temporary file is created with delete=False so that it
  # can be deleted manually after git has read it rather than automatically
  # when it is closed.
  with tempfile.NamedTemporaryFile(delete=False) as tmp_file:
    tmp_file.write(FormatDescriptionOrComment(description, directory))
    # Close the file to let git open it at the next line.
    tmp_file.close()
    git.run('commit', '-F', tmp_file.name)
    os.remove(tmp_file.name)

  # Upload a CL.
  upload_args = ['-f', '--cq-dry-run', '-r', ','.join(reviewers)]
  if not comment:
    upload_args.append('--send-mail')
  print 'Uploading CL for ' + directory + '.'
  cmd_upload(upload_args)
  if comment:
    changelist().AddComment(FormatDescriptionOrComment(comment, directory),
                            publish=True)


def GetFilesSplitByOwners(owners_database, files):
  """Returns a map of files split by OWNERS file.

  Returns:
    A map where keys are paths to directories containing an OWNERS file and
    values are lists of files sharing an OWNERS file.
  """
  files_split_by_owners = collections.defaultdict(list)
  for f in files:
    files_split_by_owners[owners_database.enclosing_dir_with_owners(
        f.LocalPath())].append(f)
  return files_split_by_owners


def PrintClInfo(cl_index, num_cls, directory, file_paths, description,
                reviewers):
  """Prints info about a CL.

  Args:
    cl_index: The index of this CL in the list of CLs to upload.
    num_cls: The total number of CLs that will be uploaded.
    directory: Path to the directory that contains the OWNERS file for which
        to upload a CL.
    file_paths: A list of files in this CL.
    description: The CL description.
    reviewers: A set of reviewers for this CL.
  """
  description_lines = FormatDescriptionOrComment(description,
                                                 directory).splitlines()
  indented_description = '\n'.join(['    ' + l for l in description_lines])

  print 'CL {}/{}'.format(cl_index, num_cls)
  print 'Path: {}'.format(directory)
  print 'Reviewers: {}'.format(', '.join(reviewers))
  print '\n' + indented_description + '\n'
  print '\n'.join(file_paths)
  print


def SplitCl(description_file, comment_file, changelist, cmd_upload, dry_run):
  """"Splits a branch into smaller branches and uploads CLs.

  Args:
    description_file: File containing the description of uploaded CLs.
    comment_file: File containing the comment of uploaded CLs.
    changelist: The Changelist class.
    cmd_upload: The function associated with the git cl upload command.
    dry_run: Whether this is a dry run (no branches or CLs created).

  Returns:
    0 in case of success. 1 in case of error.
  """
  description = AddUploadedByGitClSplitToDescription(ReadFile(description_file))
  comment = ReadFile(comment_file) if comment_file else None

  try:
    EnsureInGitRepository()

    cl = changelist()
    change = cl.GetChange(cl.GetCommonAncestorWithUpstream(), None)
    files = change.AffectedFiles()

    if not files:
      print 'Cannot split an empty CL.'
      return 1

    author = git.run('config', 'user.email').strip() or None
    refactor_branch = git.current_branch()
    assert refactor_branch, "Can't run from detached branch."
    refactor_branch_upstream = git.upstream(refactor_branch)
    assert refactor_branch_upstream, \
        "Branch %s must have an upstream." % refactor_branch

    owners_database = owners.Database(change.RepositoryRoot(), file, os.path)
    owners_database.load_data_needed_for([f.LocalPath() for f in files])

    files_split_by_owners = GetFilesSplitByOwners(owners_database, files)

    num_cls = len(files_split_by_owners)
    print('Will split current branch (' + refactor_branch + ') into ' +
          str(num_cls) + ' CLs.\n')

    for cl_index, (directory, files) in \
        enumerate(files_split_by_owners.iteritems(), 1):
      # Use '/' as a path separator in the branch name and the CL description
      # and comment.
      directory = directory.replace(os.path.sep, '/')
      file_paths = [f.LocalPath() for f in files]
      reviewers = owners_database.reviewers_for(file_paths, author)

      if dry_run:
        PrintClInfo(cl_index, num_cls, directory, file_paths, description,
                    reviewers)
      else:
        UploadCl(refactor_branch, refactor_branch_upstream, directory, files,
                 description, comment, reviewers, changelist, cmd_upload)

    # Go back to the original branch.
    git.run('checkout', refactor_branch)

  except subprocess2.CalledProcessError as cpe:
    sys.stderr.write(cpe.stderr)
    return 1
  return 0
