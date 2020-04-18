#!/usr/bin/env python
# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Rolls a git-svn dependency.

It takes the path to a dep and a git commit hash or svn revision, and updates
the parent repo's DEPS file with the corresponding git commit hash.

Sample invocation:

[chromium/src]$ roll-dep-svn third_party/WebKit 12345

After the script completes, the DEPS file will be dirty with the new revision.
The user can then:

$ git add DEPS
$ git commit
"""

import ast
import optparse
import os
import re
import sys

from itertools import izip
from subprocess import check_output, Popen, PIPE
from textwrap import dedent


SHA1_RE = re.compile('^[a-fA-F0-9]{40}$')
GIT_SVN_ID_RE = re.compile('^git-svn-id: .*@([0-9]+) .*$')
ROLL_DESCRIPTION_STR = (
'''Roll %(dep_path)s %(before_rev)s:%(after_rev)s%(svn_range)s

Summary of changes available at:
%(revlog_url)s
''')


def shorten_dep_path(dep):
  """Shorten the given dep path if necessary."""
  while len(dep) > 31:
    dep = '.../' + dep.lstrip('./').partition('/')[2]
  return dep


def posix_path(path):
  """Convert a possibly-Windows path to a posix-style path."""
  (_, path) = os.path.splitdrive(path)
  return path.replace(os.sep, '/')


def platform_path(path):
  """Convert a path to the native path format of the host OS."""
  return path.replace('/', os.sep)


def find_gclient_root():
  """Find the directory containing the .gclient file."""
  cwd = posix_path(os.getcwd())
  result = ''
  for _ in xrange(len(cwd.split('/'))):
    if os.path.exists(os.path.join(result, '.gclient')):
      return result
    result = os.path.join(result, os.pardir)
  assert False, 'Could not find root of your gclient checkout.'


def get_solution(gclient_root, dep_path):
  """Find the solution in .gclient containing the dep being rolled."""
  dep_path = os.path.relpath(dep_path, gclient_root)
  cwd = os.getcwd().rstrip(os.sep) + os.sep
  gclient_root = os.path.realpath(gclient_root)
  gclient_path = os.path.join(gclient_root, '.gclient')
  gclient_locals = {}
  execfile(gclient_path, {}, gclient_locals)
  for soln in gclient_locals['solutions']:
    soln_relpath = platform_path(soln['name'].rstrip('/')) + os.sep
    if (dep_path.startswith(soln_relpath) or
        cwd.startswith(os.path.join(gclient_root, soln_relpath))):
      return soln
  assert False, 'Could not determine the parent project for %s' % dep_path


def is_git_hash(revision):
  """Determines if a given revision is a git hash."""
  return SHA1_RE.match(revision)


def verify_git_revision(dep_path, revision):
  """Verify that a git revision exists in a repository."""
  p = Popen(['git', 'rev-list', '-n', '1', revision],
            cwd=dep_path, stdout=PIPE, stderr=PIPE)
  result = p.communicate()[0].strip()
  if p.returncode != 0 or not is_git_hash(result):
    result = None
  return result


def get_svn_revision(dep_path, git_revision):
  """Given a git revision, return the corresponding svn revision."""
  p = Popen(['git', 'log', '-n', '1', '--pretty=format:%B', git_revision],
            stdout=PIPE, cwd=dep_path)
  (log, _) = p.communicate()
  assert p.returncode == 0, 'git log %s failed.' % git_revision
  for line in reversed(log.splitlines()):
    m = GIT_SVN_ID_RE.match(line.strip())
    if m:
      return m.group(1)
  return None


def convert_svn_revision(dep_path, revision):
  """Find the git revision corresponding to an svn revision."""
  err_msg = 'Unknown error'
  revision = int(revision)
  latest_svn_rev = None
  with open(os.devnull, 'w') as devnull:
    for ref in ('HEAD', 'origin/master'):
      try:
        log_p = Popen(['git', 'log', ref],
                      cwd=dep_path, stdout=PIPE, stderr=devnull)
        grep_p = Popen(['grep', '-e', '^commit ', '-e', '^ *git-svn-id: '],
                       stdin=log_p.stdout, stdout=PIPE, stderr=devnull)
        git_rev = None
        prev_svn_rev = None
        for line in grep_p.stdout:
          if line.startswith('commit '):
            git_rev = line.split()[1]
            continue
          try:
            svn_rev = int(line.split()[1].partition('@')[2])
          except (IndexError, ValueError):
            print >> sys.stderr, (
                'WARNING: Could not parse svn revision out of "%s"' % line)
            continue
          if not latest_svn_rev or int(svn_rev) > int(latest_svn_rev):
            latest_svn_rev = svn_rev
          if svn_rev == revision:
            return git_rev
          if svn_rev > revision:
            prev_svn_rev = svn_rev
            continue
          if prev_svn_rev:
            err_msg = 'git history skips from revision %d to revision %d.' % (
                svn_rev, prev_svn_rev)
          else:
            err_msg = (
                'latest available revision is %d; you may need to '
                '"git fetch origin" to get the latest commits.' %
                latest_svn_rev)
      finally:
        log_p.terminate()
        grep_p.terminate()
  raise RuntimeError('No match for revision %d; %s' % (revision, err_msg))


def get_git_revision(dep_path, revision):
  """Convert the revision argument passed to the script to a git revision."""
  svn_revision = None
  if revision.startswith('r'):
    git_revision = convert_svn_revision(dep_path, revision[1:])
    svn_revision = revision[1:]
  elif re.search('[a-fA-F]', revision):
    git_revision = verify_git_revision(dep_path, revision)
    if not git_revision:
      raise RuntimeError('Please \'git fetch origin\' in %s' % dep_path)
    svn_revision = get_svn_revision(dep_path, git_revision)
  elif len(revision) > 6:
    git_revision = verify_git_revision(dep_path, revision)
    if git_revision:
      svn_revision = get_svn_revision(dep_path, git_revision)
    else:
      git_revision = convert_svn_revision(dep_path, revision)
      svn_revision = revision
  else:
    try:
      git_revision = convert_svn_revision(dep_path, revision)
      svn_revision = revision
    except RuntimeError:
      git_revision = verify_git_revision(dep_path, revision)
      if not git_revision:
        raise
      svn_revision = get_svn_revision(dep_path, git_revision)
  return git_revision, svn_revision


def ast_err_msg(node):
  return 'ERROR: Undexpected DEPS file AST structure at line %d column %d' % (
      node.lineno, node.col_offset)


def find_deps_section(deps_ast, section):
  """Find a top-level section of the DEPS file in the AST."""
  try:
    result = [n.value for n in deps_ast.body if
              n.__class__ is ast.Assign and
              n.targets[0].__class__ is ast.Name and
              n.targets[0].id == section][0]
    return result
  except IndexError:
    return None


def find_dict_index(dict_node, key):
  """Given a key, find the index of the corresponding dict entry."""
  assert dict_node.__class__ is ast.Dict, ast_err_msg(dict_node)
  indices = [i for i, n in enumerate(dict_node.keys) if
             n.__class__ is ast.Str and n.s == key]
  assert len(indices) < 2, (
      'Found redundant dict entries for key "%s"' % key)
  return indices[0] if indices else None


def update_node(deps_lines, deps_ast, node, git_revision):
  """Update an AST node with the new git revision."""
  if node.__class__ is ast.Str:
    return update_string(deps_lines, node, git_revision)
  elif node.__class__ is ast.BinOp:
    return update_binop(deps_lines, deps_ast, node, git_revision)
  elif node.__class__ is ast.Call:
    return update_call(deps_lines, deps_ast, node, git_revision)
  elif node.__class__ is ast.Dict:
    return update_dict(deps_lines, deps_ast, node, git_revision)
  else:
    assert False, ast_err_msg(node)


def update_string(deps_lines, string_node, git_revision):
  """Update a string node in the AST with the new git revision."""
  line_idx = string_node.lineno - 1
  start_idx = string_node.col_offset - 1
  line = deps_lines[line_idx]
  (prefix, sep, old_rev) = string_node.s.partition('@')
  if sep:
    start_idx = line.find(prefix + sep, start_idx) + len(prefix + sep)
    tail_idx = start_idx + len(old_rev)
  else:
    start_idx = line.find(prefix, start_idx)
    tail_idx = start_idx + len(prefix)
    old_rev = prefix
  deps_lines[line_idx] = line[:start_idx] + git_revision + line[tail_idx:]
  return line_idx


def update_binop(deps_lines, deps_ast, binop_node, git_revision):
  """Update a binary operation node in the AST with the new git revision."""
  # Since the revision part is always last, assume that it's the right-hand
  # operand that needs to be updated.
  return update_node(deps_lines, deps_ast, binop_node.right, git_revision)


def update_call(deps_lines, deps_ast, call_node, git_revision):
  """Update a function call node in the AST with the new git revision."""
  # The only call we know how to handle is Var()
  assert call_node.func.id == 'Var', ast_err_msg(call_node)
  assert call_node.args and call_node.args[0].__class__ is ast.Str, (
      ast_err_msg(call_node))
  return update_var(deps_lines, deps_ast, call_node.args[0].s, git_revision)


def update_dict(deps_lines, deps_ast, dict_node, git_revision):
  """Update a dict node in the AST with the new git revision."""
  for key, value in zip(dict_node.keys, dict_node.values):
    if key.__class__ is ast.Str and key.s == 'url':
      return update_node(deps_lines, deps_ast, value, git_revision)


def update_var(deps_lines, deps_ast, var_name, git_revision):
  """Update an entry in the vars section of the DEPS file with the new
  git revision."""
  vars_node = find_deps_section(deps_ast, 'vars')
  assert vars_node, 'Could not find "vars" section of DEPS file.'
  var_idx = find_dict_index(vars_node, var_name)
  assert var_idx is not None, (
      'Could not find definition of "%s" var in DEPS file.' % var_name)
  val_node = vars_node.values[var_idx]
  return update_node(deps_lines, deps_ast, val_node, git_revision)


def short_rev(rev, dep_path):
  return check_output(['git', 'rev-parse', '--short', rev],
                      cwd=dep_path).rstrip()


def generate_commit_message(deps_section, dep_path, dep_name, new_rev):
  dep_url = deps_section[dep_name]
  if isinstance(dep_url, dict):
    dep_url = dep_url['url']

  (url, _, old_rev) = dep_url.partition('@')
  if url.endswith('.git'):
    url = url[:-4]
  old_rev_short = short_rev(old_rev, dep_path)
  new_rev_short = short_rev(new_rev, dep_path)
  url += '/+log/%s..%s' % (old_rev_short, new_rev_short)
  try:
    old_svn_rev = get_svn_revision(dep_path, old_rev)
    new_svn_rev = get_svn_revision(dep_path, new_rev)
  except Exception:
    # Ignore failures that might arise from the repo not being checked out.
    old_svn_rev = new_svn_rev = None
  svn_range_str = ''
  if old_svn_rev and new_svn_rev:
    svn_range_str = ' (svn %s:%s)' % (old_svn_rev, new_svn_rev)
  return dedent(ROLL_DESCRIPTION_STR % {
    'dep_path': shorten_dep_path(dep_name),
    'before_rev': old_rev_short,
    'after_rev': new_rev_short,
    'svn_range': svn_range_str,
    'revlog_url': url,
  })


def update_deps_entry(deps_lines, deps_ast, value_node, new_rev, comment):
  line_idx = update_node(deps_lines, deps_ast, value_node, new_rev)
  (content, _, _) = deps_lines[line_idx].partition('#')
  if comment:
    deps_lines[line_idx] = '%s # %s' % (content.rstrip(), comment)
  else:
    deps_lines[line_idx] = content.rstrip()


def update_deps(deps_file, dep_path, dep_name, new_rev, comment):
  """Update the DEPS file with the new git revision."""
  commit_msg = ''
  with open(deps_file) as fh:
    deps_content = fh.read()
  deps_locals = {}
  def _Var(key):
    return deps_locals['vars'][key]
  deps_locals['Var'] = _Var
  exec deps_content in {}, deps_locals
  deps_lines = deps_content.splitlines()
  deps_ast = ast.parse(deps_content, deps_file)
  deps_node = find_deps_section(deps_ast, 'deps')
  assert deps_node, 'Could not find "deps" section of DEPS file'
  dep_idx = find_dict_index(deps_node, dep_name)
  if dep_idx is not None:
    value_node = deps_node.values[dep_idx]
    update_deps_entry(deps_lines, deps_ast, value_node, new_rev, comment)
    commit_msg = generate_commit_message(deps_locals['deps'], dep_path,
                                         dep_name, new_rev)
  deps_os_node = find_deps_section(deps_ast, 'deps_os')
  if deps_os_node:
    for (os_name, os_node) in izip(deps_os_node.keys, deps_os_node.values):
      dep_idx = find_dict_index(os_node, dep_name)
      if dep_idx is not None:
        value_node = os_node.values[dep_idx]
        if value_node.__class__ is ast.Name and value_node.id == 'None':
          pass
        else:
          update_deps_entry(deps_lines, deps_ast, value_node, new_rev, comment)
          commit_msg = generate_commit_message(
              deps_locals['deps_os'][os_name.s], dep_path, dep_name, new_rev)
  if not commit_msg:
    print 'Could not find an entry in %s to update.' % deps_file
    return 1

  print 'Pinning %s' % dep_name
  print 'to revision %s' % new_rev
  print 'in %s' % deps_file
  with open(deps_file, 'w') as fh:
    for line in deps_lines:
      print >> fh, line
  deps_file_dir = os.path.normpath(os.path.dirname(deps_file))
  deps_file_root = Popen(
      ['git', 'rev-parse', '--show-toplevel'],
      cwd=deps_file_dir, stdout=PIPE).communicate()[0].strip()
  with open(os.path.join(deps_file_root, '.git', 'MERGE_MSG'), 'w') as fh:
    fh.write(commit_msg)
  return 0


def main(argv):
  usage = 'Usage: roll-dep-svn [options] <dep path> <rev> [ <DEPS file> ]'
  parser = optparse.OptionParser(usage=usage, description=__doc__)
  parser.add_option('--no-verify-revision',
                    help='Don\'t verify the revision passed in. This '
                         'also skips adding an svn revision comment '
                         'for git dependencies and requires the passed '
                         'revision to be a git hash.',
                    default=False, action='store_true')
  options, args = parser.parse_args(argv)
  if len(args) not in (2, 3):
    parser.error('Expected either 2 or 3 positional parameters.')
  arg_dep_path, revision = args[:2]
  gclient_root = find_gclient_root()
  dep_path = platform_path(arg_dep_path)
  if not os.path.exists(dep_path):
    dep_path = os.path.join(gclient_root, dep_path)
  if not options.no_verify_revision:
    # Only require the path to exist if the revision should be verified. A path
    # to e.g. os deps might not be checked out.
    if not os.path.isdir(dep_path):
      print >> sys.stderr, 'No such directory: %s' % arg_dep_path
      return 1
  if len(args) > 2:
    deps_file = args[2]
  else:
    soln = get_solution(gclient_root, dep_path)
    soln_path = os.path.relpath(os.path.join(gclient_root, soln['name']))
    deps_file = os.path.join(soln_path, 'DEPS')
  dep_name = posix_path(os.path.relpath(dep_path, gclient_root))
  if options.no_verify_revision:
    if not is_git_hash(revision):
      print >> sys.stderr, (
          'The passed revision %s must be a git hash when skipping revision '
          'verification.' % revision)
      return 1
    git_rev = revision
    comment = None
  else:
    git_rev, svn_rev = get_git_revision(dep_path, revision)
    comment = ('from svn revision %s' % svn_rev) if svn_rev else None
    if not git_rev:
      print >> sys.stderr, 'Could not find git revision matching %s.' % revision
      return 1
  return update_deps(deps_file, dep_path, dep_name, git_rev, comment)


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
