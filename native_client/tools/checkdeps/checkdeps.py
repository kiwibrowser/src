#!/usr/bin/env python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Makes sure that files include headers from allowed directories.

Checks DEPS files in the source tree for rules, and applies those rules to
"#include" commands in source files. Any source file including something not
permitted by the DEPS files will fail.

The format of the deps file:

First you have the normal module-level deps. These are the ones used by
gclient. An example would be:

  deps = {
    "base":"http://foo.bar/trunk/base"
  }

DEPS files not in the top-level of a module won't need this. Then you have
any additional include rules. You can add (using "+") or subtract (using "-")
from the previously specified rules (including module-level deps).

  include_rules = {
    # Code should be able to use base (it's specified in the module-level
    # deps above), but nothing in "base/evil" because it's evil.
    "-base/evil",

    # But this one subdirectory of evil is OK.
    "+base/evil/not",

    # And it can include files from this other directory even though there is
    # no deps rule for it.
    "+tools/crime_fighter"
  }

DEPS files may be placed anywhere in the tree. Each one applies to all
subdirectories, where there may be more DEPS files that provide additions or
subtractions for their own sub-trees.

There is an implicit rule for the current directory (where the DEPS file lives)
and all of its subdirectories. This prevents you from having to explicitly
allow the current directory everywhere.  This implicit rule is applied first,
so you can modify or remove it using the normal include rules.

The rules are processed in order. This means you can explicitly allow a higher
directory and then take away permissions from sub-parts, or the reverse.

Note that all directory separators must be slashes (Unix-style) and not
backslashes. All directories should be relative to the source root and use
only lowercase.
"""

import os
import optparse
import pipes
import re
import sys
import copy

# Variable name used in the DEPS file to add or subtract include files from
# the module-level deps.
INCLUDE_RULES_VAR_NAME = "include_rules"

# Optionally present in the DEPS file to list subdirectories which should not
# be checked. This allows us to skip third party code, for example.
SKIP_SUBDIRS_VAR_NAME = "skip_child_includes"

# The maximum number of non-include lines we can see before giving up.
MAX_UNINTERESTING_LINES = 50

# The maximum line length, this is to be efficient in the case of very long
# lines (which can't be #includes).
MAX_LINE_LENGTH = 128

# Set to true for more output. This is set by the command line options.
VERBOSE = False

# This regular expression will be used to extract filenames from include
# statements.
EXTRACT_INCLUDE_PATH = re.compile('[ \t]*#[ \t]*(?:include|import)[ \t]+"(.*)"')

# In lowercase, using forward slashes as directory separators, ending in a
# forward slash. Set by the command line options.
BASE_DIRECTORY = ""

# The directories which contain the sources managed by git.
GIT_SOURCE_DIRECTORY = set()


# Specifies a single rule for an include, which can be either allow or disallow.
class Rule(object):
  def __init__(self, allow, dir, source):
    self._allow = allow
    self._dir = dir
    self._source = source

  def __str__(self):
    if (self._allow):
      return '"+%s" from %s.' % (self._dir, self._source)
    return '"-%s" from %s.' % (self._dir, self._source)

  def ParentOrMatch(self, other):
    """Returns true if the input string is an exact match or is a parent
    of the current rule. For example, the input "foo" would match "foo/bar"."""
    return self._dir == other or self._dir.startswith(other + "/")

  def ChildOrMatch(self, other):
    """Returns true if the input string would be covered by this rule. For
    example, the input "foo/bar" would match the rule "foo"."""
    return self._dir == other or other.startswith(self._dir + "/")


def ParseRuleString(rule_string, source):
  """Returns a tuple of a boolean indicating whether the directory is an allow
  rule, and a string holding the directory name.
  """
  if len(rule_string) < 1:
    raise Exception('The rule string "%s" is too short\nin %s' %
                    (rule_string, source))

  if rule_string[0] == "+":
    return (True, rule_string[1:])
  if rule_string[0] == "-":
    return (False, rule_string[1:])
  raise Exception('The rule string "%s" does not begin with a "+" or a "-"' %
                  rule_string)


class Rules:
  def __init__(self):
    """Initializes the current rules with an empty rule list."""
    self._rules = []

  def __str__(self):
    ret = "Rules = [\n"
    ret += "\n".join([" %s" % x for x in self._rules])
    ret += "]\n"
    return ret

  def AddRule(self, rule_string, source):
    """Adds a rule for the given rule string.

    Args:
      rule_string: The include_rule string read from the DEPS file to apply.
      source: A string representing the location of that string (filename, etc.)
              so that we can give meaningful errors.
    """
    (add_rule, rule_dir) = ParseRuleString(rule_string, source)
    # Remove any existing rules or sub-rules that apply. For example, if we're
    # passed "foo", we should remove "foo", "foo/bar", but not "foobar".
    self._rules = [x for x in self._rules if not x.ParentOrMatch(rule_dir)]
    self._rules.insert(0, Rule(add_rule, rule_dir, source))

  def DirAllowed(self, allowed_dir):
    """Returns a tuple (success, message), where success indicates if the given
    directory is allowed given the current set of rules, and the message tells
    why if the comparison failed."""
    for rule in self._rules:
      if rule.ChildOrMatch(allowed_dir):
        # This rule applies.
        if rule._allow:
          return (True, "")
        return (False, rule.__str__())
    # No rules apply, fail.
    return (False, "no rule applying")


def ApplyRules(existing_rules, includes, cur_dir):
  """Applies the given include rules, returning the new rules.

  Args:
    existing_rules: A set of existing rules that will be combined.
    include: The list of rules from the "include_rules" section of DEPS.
    cur_dir: The current directory. We will create an implicit rule that
             allows inclusion from this directory.

  Returns: A new set of rules combining the existing_rules with the other
           arguments.
  """
  rules = copy.copy(existing_rules)

  # First apply the implicit "allow" rule for the current directory.
  if cur_dir.lower().startswith(BASE_DIRECTORY):
    relative_dir = cur_dir[len(BASE_DIRECTORY) + 1:]
    # Normalize path separators to slashes.
    relative_dir = relative_dir.replace("\\", "/")
    source = relative_dir
    if len(source) == 0:
      source = "top level"  # Make the help string a little more meaningful.
    rules.AddRule("+" + relative_dir, "Default rule for " + source)
  else:
    raise Exception("Internal error: base directory is not at the beginning" +
                    " for\n  %s and base dir\n  %s" %
                    (cur_dir, BASE_DIRECTORY))

  # Last, apply the additional explicit rules.
  for (index, rule_str) in enumerate(includes):
    if not len(relative_dir):
      rule_description = "the top level include_rules"
    else:
      rule_description = relative_dir + "'s include_rules"
    rules.AddRule(rule_str, rule_description)

  return rules


def ApplyDirectoryRules(existing_rules, dir_name):
  """Combines rules from the existing rules and the new directory.

  Any directory can contain a DEPS file. Toplevel DEPS files can contain
  module dependencies which are used by gclient. We use these, along with
  additional include rules and implicit rules for the given directory, to
  come up with a combined set of rules to apply for the directory.

  Args:
    existing_rules: The rules for the parent directory. We'll add-on to these.
    dir_name: The directory name that the deps file may live in (if it exists).
              This will also be used to generate the implicit rules.

  Returns: A tuple containing: (1) the combined set of rules to apply to the
           sub-tree, and (2) a list of all subdirectories that should NOT be
           checked, as specified in the DEPS file (if any).
  """
  # Check for a .svn directory in this directory or check this directory is
  # contained in git source direcotries. This will tell us if it's a source
  # directory and should be checked.
  if not (os.path.exists(os.path.join(dir_name, ".svn")) or
          (dir_name.lower() in GIT_SOURCE_DIRECTORY)):
    return (None, [])

  # Check the DEPS file in this directory.
  if VERBOSE:
    print "Applying rules from", dir_name
  def FromImpl(unused, unused2):
    pass  # NOP function so "From" doesn't fail.

  def FileImpl(unused):
    pass  # NOP function so "File" doesn't fail.

  class _VarImpl:
    def __init__(self, local_scope):
      self._local_scope = local_scope

    def Lookup(self, var_name):
      """Implements the Var syntax."""
      if var_name in self._local_scope.get("vars", {}):
        return self._local_scope["vars"][var_name]
      raise Error("Var is not defined: %s" % var_name)

  local_scope = {}
  global_scope = {
      "File": FileImpl,
      "From": FromImpl,
      "Var": _VarImpl(local_scope).Lookup,
      }
  deps_file = os.path.join(dir_name, "DEPS")

  if os.path.isfile(deps_file):
    execfile(deps_file, global_scope, local_scope)
  elif VERBOSE:
    print "  No deps file found in", dir_name

  # Even if a DEPS file does not exist we still invoke ApplyRules
  # to apply the implicit "allow" rule for the current directory
  include_rules = local_scope.get(INCLUDE_RULES_VAR_NAME, [])
  skip_subdirs = local_scope.get(SKIP_SUBDIRS_VAR_NAME, [])

  return (ApplyRules(existing_rules, include_rules, dir_name), skip_subdirs)


def ShouldCheckFile(file_name):
  """Returns True if the given file is a type we want to check."""
  checked_extensions = [
      '.c',
      '.cc',
      '.h',
      '.m',
      '.mm',
      # These are not the preferred extension in our codebase,
      # but including them for good measure.
      # (They do appear in the newlib toolchain + third_party libraries).
      '.cpp',
      '.hpp',
  ]
  basename, extension = os.path.splitext(file_name)
  return extension in checked_extensions


def CheckLine(rules, line):
  """Checks the given file with the given rule set.
  Returns a tuple (is_include, illegal_description).
  If the line is an #include directive the first value will be True.
  If it is also an illegal include, the second value will be a string describing
  the error.  Otherwise, it will be None."""
  found_item = EXTRACT_INCLUDE_PATH.match(line)
  if not found_item:
    return False, None  # Not a match

  include_path = found_item.group(1)

  # Fix up backslashes in case somebody accidentally used them.
  include_path.replace("\\", "/")

  if include_path.find("/") < 0:
    # Don't fail when no directory is specified. We may want to be more
    # strict about this in the future.
    if VERBOSE:
      print " WARNING: directory specified with no path: " + include_path
    return True, None

  (allowed, why_failed) = rules.DirAllowed(include_path)
  if not allowed:
    if VERBOSE:
      retval = "\nFor " + rules.__str__()
    else:
      retval = ""
    return True, retval + ('Illegal include: "%s"\n    Because of %s' %
        (include_path, why_failed))

  return True, None


def CheckFile(rules, file_name):
  """Checks the given file with the given rule set.

  Args:
    rules: The set of rules that apply to files in this directory.
    file_name: The source file to check.

  Returns: Either a string describing the error if there was one, or None if
           the file checked out OK.
  """
  if VERBOSE:
    print "Checking: " + file_name

  ret_val = ""  # We'll collect the error messages in here
  last_include = 0
  try:
    cur_file = open(file_name, "r")
    in_if0 = 0
    for line_num in xrange(sys.maxint):
      if line_num - last_include > MAX_UNINTERESTING_LINES:
        break

      cur_line = cur_file.readline(MAX_LINE_LENGTH)
      if cur_line == "":
        break
      cur_line = cur_line.strip()

      # Check to see if we're at / inside a #if 0 block
      if cur_line == '#if 0':
        in_if0 += 1
        continue
      if in_if0 > 0:
        if cur_line.startswith('#if'):
          in_if0 += 1
        elif cur_line == '#endif':
          in_if0 -= 1
        continue

      is_include, line_status = CheckLine(rules, cur_line)
      if is_include:
        last_include = line_num
      if line_status is not None:
        if len(line_status) > 0:  # Add newline to separate messages.
          line_status += "\n"
        ret_val += line_status
    cur_file.close()

  except IOError:
    if VERBOSE:
      print "Unable to open file: " + file_name
    cur_file.close()

  # Map empty string to None for easier checking.
  if len(ret_val) == 0:
    return None
  return ret_val


def CheckDirectory(parent_rules, dir_name):
  (rules, skip_subdirs) = ApplyDirectoryRules(parent_rules, dir_name)
  if rules == None:
    return True

  # Collect a list of all files and directories to check.
  files_to_check = []
  dirs_to_check = []
  success = True
  contents = os.listdir(dir_name)
  for cur in contents:
    if cur in skip_subdirs:
      continue  # Don't check children that DEPS has asked us to skip.
    full_name = os.path.join(dir_name, cur)
    if os.path.isdir(full_name):
      dirs_to_check.append(full_name)
    elif ShouldCheckFile(full_name):
      files_to_check.append(full_name)

  # First check all files in this directory.
  for cur in files_to_check:
    file_status = CheckFile(rules, cur)
    if file_status != None:
      print "ERROR in " + cur + "\n" + file_status
      success = False

  # Next recurse into the subdirectories.
  for cur in dirs_to_check:
    if not CheckDirectory(rules, cur):
      success = False

  return success


def GetGitSourceDirectory(root):
  """Returns a set of the directories to be checked.

  Args:
    root: The repository root where .git directory exists.

  Returns:
    A set of directories which contain sources managed by git.
  """
  git_source_directory = set()
  popen_out = os.popen("cd %s && git ls-files --full-name ." %
                       pipes.quote(root))
  for line in popen_out.readlines():
    dir_name = os.path.join(root, os.path.dirname(line))
    # Add the directory as well as all the parent directories.
    while dir_name != root:
      git_source_directory.add(dir_name)
      dir_name = os.path.dirname(dir_name)
  git_source_directory.add(root)
  return git_source_directory


def PrintUsage():
  print """Usage: python checkdeps.py [--root <root>] [tocheck]
  --root   Specifies the repository root. This defaults to "../../.." relative
           to the script file. This will be correct given the normal location
           of the script in "<root>/tools/checkdeps".

  tocheck  Specifies the directory, relative to root, to check. This defaults
           to "." so it checks everything. Only one level deep is currently
           supported, so you can say "chrome" but not "chrome/browser".

Examples:
  python checkdeps.py
  python checkdeps.py --root c:\\source chrome"""


def checkdeps(options, args):
  global VERBOSE
  if options.verbose:
    VERBOSE = True

  # Optional base directory of the repository.
  global BASE_DIRECTORY
  if not options.base_directory:
    BASE_DIRECTORY = os.path.abspath(
        os.path.join(os.path.abspath(os.path.dirname(__file__)), "../.."))
  else:
    BASE_DIRECTORY = os.path.abspath(options.base_directory)

  # Figure out which directory we have to check.
  if len(args) == 0:
    # No directory to check specified, use the repository root.
    start_dir = BASE_DIRECTORY
  elif len(args) == 1:
    # Directory specified. Start here. It's supposed to be relative to the
    # base directory.
    start_dir = os.path.abspath(os.path.join(BASE_DIRECTORY, args[0]))
  else:
    # More than one argument, we don't handle this.
    PrintUsage()
    return 1

  print "Using base directory:", BASE_DIRECTORY
  print "Checking:", start_dir

  base_rules = Rules()

  # The base directory should be lower case from here on since it will be used
  # for substring matching on the includes, and we compile on case-insensitive
  # systems. Plus, we always use slashes here since the include parsing code
  # will also normalize to slashes.
  BASE_DIRECTORY = BASE_DIRECTORY.lower()
  BASE_DIRECTORY = BASE_DIRECTORY.replace("\\", "/")
  start_dir = start_dir.replace("\\", "/")

  if os.path.exists(os.path.join(BASE_DIRECTORY, ".git")):
    global GIT_SOURCE_DIRECTORY
    GIT_SOURCE_DIRECTORY = GetGitSourceDirectory(BASE_DIRECTORY)

  success = CheckDirectory(base_rules, start_dir)
  if not success:
    print "\nFAILED\n"
    return 1
  print "\nSUCCESS\n"
  return 0


def main():
  option_parser = optparse.OptionParser()
  option_parser.add_option("", "--root", default="", dest="base_directory",
                           help='Specifies the repository root. This defaults '
                           'to "../../.." relative to the script file, which '
                           'will normally be the repository root.')
  option_parser.add_option("-v", "--verbose", action="store_true",
                           default=False, help="Print debug logging")
  options, args = option_parser.parse_args()
  return checkdeps(options, args)


if '__main__' == __name__:
  sys.exit(main())
