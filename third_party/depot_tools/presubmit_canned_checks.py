# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generic presubmit checks that can be reused by other presubmit checks."""

import os as _os
_HERE = _os.path.dirname(_os.path.abspath(__file__))

# Justifications for each filter:
#
# - build/include       : Too many; fix in the future.
# - build/include_order : Not happening; #ifdefed includes.
# - build/namespace     : I'm surprised by how often we violate this rule.
# - readability/casting : Mistakes a whole bunch of function pointer.
# - runtime/int         : Can be fixed long term; volume of errors too high
# - runtime/virtual     : Broken now, but can be fixed in the future?
# - whitespace/braces   : We have a lot of explicit scoping in chrome code.
DEFAULT_LINT_FILTERS = [
  '-build/include',
  '-build/include_order',
  '-build/namespace',
  '-readability/casting',
  '-runtime/int',
  '-runtime/virtual',
  '-whitespace/braces',
]

# These filters will always be removed, even if the caller specifies a filter
# set, as they are problematic or broken in some way.
#
# Justifications for each filter:
# - build/c++11         : Rvalue ref checks are unreliable (false positives),
#                         include file and feature blacklists are
#                         google3-specific.
BLACKLIST_LINT_FILTERS = [
  '-build/c++11',
]

### Description checks

def CheckChangeHasBugField(input_api, output_api):
  """Requires that the changelist have a Bug: field."""
  if input_api.change.BugsFromDescription():
    return []
  else:
    return [output_api.PresubmitNotifyResult(
        'If this change has an associated bug, add Bug: [bug number].')]


def CheckDoNotSubmitInDescription(input_api, output_api):
  """Checks that the user didn't add 'DO NOT ''SUBMIT' to the CL description.
  """
  keyword = 'DO NOT ''SUBMIT'
  if keyword in input_api.change.DescriptionText():
    return [output_api.PresubmitError(
        keyword + ' is present in the changelist description.')]
  else:
    return []


def CheckChangeHasDescription(input_api, output_api):
  """Checks the CL description is not empty."""
  text = input_api.change.DescriptionText()
  if text.strip() == '':
    if input_api.is_committing:
      return [output_api.PresubmitError('Add a description to the CL.')]
    else:
      return [output_api.PresubmitNotifyResult('Add a description to the CL.')]
  return []


def CheckChangeWasUploaded(input_api, output_api):
  """Checks that the issue was uploaded before committing."""
  if input_api.is_committing and not input_api.change.issue:
    return [output_api.PresubmitError(
      'Issue wasn\'t uploaded. Please upload first.')]
  return []


### Content checks

def CheckAuthorizedAuthor(input_api, output_api):
  """For non-googler/chromites committers, verify the author's email address is
  in AUTHORS.
  """
  if input_api.is_committing:
    error_type = output_api.PresubmitError
  else:
    error_type = output_api.PresubmitPromptWarning

  author = input_api.change.author_email
  if not author:
    input_api.logging.info('No author, skipping AUTHOR check')
    return []
  authors_path = input_api.os_path.join(
      input_api.PresubmitLocalPath(), 'AUTHORS')
  valid_authors = (
      input_api.re.match(r'[^#]+\s+\<(.+?)\>\s*$', line)
      for line in open(authors_path))
  valid_authors = [item.group(1).lower() for item in valid_authors if item]
  if not any(input_api.fnmatch.fnmatch(author.lower(), valid)
             for valid in valid_authors):
    input_api.logging.info('Valid authors are %s', ', '.join(valid_authors))
    return [error_type(
        ('%s is not in AUTHORS file. If you are a new contributor, please visit'
        '\n'
        'https://www.chromium.org/developers/contributing-code and read the '
        '"Legal" section\n'
        'If you are a chromite, verify the contributor signed the CLA.') %
        author)]
  return []


def CheckDoNotSubmitInFiles(input_api, output_api):
  """Checks that the user didn't add 'DO NOT ''SUBMIT' to any files."""
  # We want to check every text file, not just source files.
  file_filter = lambda x : x
  keyword = 'DO NOT ''SUBMIT'
  errors = _FindNewViolationsOfRule(lambda _, line : keyword not in line,
                                    input_api, file_filter)
  text = '\n'.join('Found %s in %s' % (keyword, loc) for loc in errors)
  if text:
    return [output_api.PresubmitError(text)]
  return []


def CheckChangeLintsClean(input_api, output_api, source_file_filter=None,
                          lint_filters=None, verbose_level=None):
  """Checks that all '.cc' and '.h' files pass cpplint.py."""
  _RE_IS_TEST = input_api.re.compile(r'.*tests?.(cc|h)$')
  result = []

  cpplint = input_api.cpplint
  # Access to a protected member _XX of a client class
  # pylint: disable=protected-access
  cpplint._cpplint_state.ResetErrorCounts()

  lint_filters = lint_filters or DEFAULT_LINT_FILTERS
  lint_filters.extend(BLACKLIST_LINT_FILTERS)
  cpplint._SetFilters(','.join(lint_filters))

  # We currently are more strict with normal code than unit tests; 4 and 5 are
  # the verbosity level that would normally be passed to cpplint.py through
  # --verbose=#. Hopefully, in the future, we can be more verbose.
  files = [f.AbsoluteLocalPath() for f in
           input_api.AffectedSourceFiles(source_file_filter)]
  for file_name in files:
    if _RE_IS_TEST.match(file_name):
      level = 5
    else:
      level = 4

    verbose_level = verbose_level or level
    cpplint.ProcessFile(file_name, verbose_level)

  if cpplint._cpplint_state.error_count > 0:
    if input_api.is_committing:
      res_type = output_api.PresubmitError
    else:
      res_type = output_api.PresubmitPromptWarning
    result = [res_type('Changelist failed cpplint.py check.')]

  return result


def CheckChangeHasNoCR(input_api, output_api, source_file_filter=None):
  """Checks no '\r' (CR) character is in any source files."""
  cr_files = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    if '\r' in input_api.ReadFile(f, 'rb'):
      cr_files.append(f.LocalPath())
  if cr_files:
    return [output_api.PresubmitPromptWarning(
        'Found a CR character in these files:', items=cr_files)]
  return []


def CheckChangeHasOnlyOneEol(input_api, output_api, source_file_filter=None):
  """Checks the files ends with one and only one \n (LF)."""
  eof_files = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    # Check that the file ends in one and only one newline character.
    if len(contents) > 1 and (contents[-1:] != '\n' or contents[-2:-1] == '\n'):
      eof_files.append(f.LocalPath())

  if eof_files:
    return [output_api.PresubmitPromptWarning(
      'These files should end in one (and only one) newline character:',
      items=eof_files)]
  return []


def CheckChangeHasNoCrAndHasOnlyOneEol(input_api, output_api,
                                       source_file_filter=None):
  """Runs both CheckChangeHasNoCR and CheckChangeHasOnlyOneEOL in one pass.

  It is faster because it is reading the file only once.
  """
  cr_files = []
  eof_files = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    if '\r' in contents:
      cr_files.append(f.LocalPath())
    # Check that the file ends in one and only one newline character.
    if len(contents) > 1 and (contents[-1:] != '\n' or contents[-2:-1] == '\n'):
      eof_files.append(f.LocalPath())
  outputs = []
  if cr_files:
    outputs.append(output_api.PresubmitPromptWarning(
        'Found a CR character in these files:', items=cr_files))
  if eof_files:
    outputs.append(output_api.PresubmitPromptWarning(
      'These files should end in one (and only one) newline character:',
      items=eof_files))
  return outputs

def CheckGenderNeutral(input_api, output_api, source_file_filter=None):
  """Checks that there are no gendered pronouns in any of the text files to be
  submitted.
  """
  gendered_re = input_api.re.compile(
      '(^|\s|\(|\[)([Hh]e|[Hh]is|[Hh]ers?|[Hh]im|[Ss]he|[Gg]uys?)\\b')

  errors = []
  for f in input_api.AffectedFiles(include_deletes=False,
                                   file_filter=source_file_filter):
    for line_num, line in f.ChangedContents():
      if gendered_re.search(line):
        errors.append('%s (%d): %s' % (f.LocalPath(), line_num, line))

  if len(errors):
    return [output_api.PresubmitPromptWarning('Found a gendered pronoun in:',
                                              long_text='\n'.join(errors))]
  return []



def _ReportErrorFileAndLine(filename, line_num, dummy_line):
  """Default error formatter for _FindNewViolationsOfRule."""
  return '%s:%s' % (filename, line_num)


def _FindNewViolationsOfRule(callable_rule, input_api, source_file_filter=None,
                             error_formatter=_ReportErrorFileAndLine):
  """Find all newly introduced violations of a per-line rule (a callable).

  Arguments:
    callable_rule: a callable taking a file extension and line of input and
      returning True if the rule is satisfied and False if there was a problem.
    input_api: object to enumerate the affected files.
    source_file_filter: a filter to be passed to the input api.
    error_formatter: a callable taking (filename, line_number, line) and
      returning a formatted error string.

  Returns:
    A list of the newly-introduced violations reported by the rule.
  """
  errors = []
  for f in input_api.AffectedFiles(include_deletes=False,
                                   file_filter=source_file_filter):
    # For speed, we do two passes, checking first the full file.  Shelling out
    # to the SCM to determine the changed region can be quite expensive on
    # Win32.  Assuming that most files will be kept problem-free, we can
    # skip the SCM operations most of the time.
    extension = str(f.LocalPath()).rsplit('.', 1)[-1]
    if all(callable_rule(extension, line) for line in f.NewContents()):
      continue  # No violation found in full text: can skip considering diff.

    for line_num, line in f.ChangedContents():
      if not callable_rule(extension, line):
        errors.append(error_formatter(f.LocalPath(), line_num, line))

  return errors


def CheckChangeHasNoTabs(input_api, output_api, source_file_filter=None):
  """Checks that there are no tab characters in any of the text files to be
  submitted.
  """
  # In addition to the filter, make sure that makefiles are blacklisted.
  if not source_file_filter:
    # It's the default filter.
    source_file_filter = input_api.FilterSourceFile
  def filter_more(affected_file):
    basename = input_api.os_path.basename(affected_file.LocalPath())
    return (not (basename in ('Makefile', 'makefile') or
                 basename.endswith('.mk')) and
            source_file_filter(affected_file))

  tabs = _FindNewViolationsOfRule(lambda _, line : '\t' not in line,
                                  input_api, filter_more)

  if tabs:
    return [output_api.PresubmitPromptWarning('Found a tab character in:',
                                              long_text='\n'.join(tabs))]
  return []


def CheckChangeTodoHasOwner(input_api, output_api, source_file_filter=None):
  """Checks that the user didn't add TODO(name) without an owner."""

  unowned_todo = input_api.re.compile('TO''DO[^(]')
  errors = _FindNewViolationsOfRule(lambda _, x : not unowned_todo.search(x),
                                    input_api, source_file_filter)
  errors = ['Found TO''DO with no owner in ' + x for x in errors]
  if errors:
    return [output_api.PresubmitPromptWarning('\n'.join(errors))]
  return []


def CheckChangeHasNoStrayWhitespace(input_api, output_api,
                                    source_file_filter=None):
  """Checks that there is no stray whitespace at source lines end."""
  errors = _FindNewViolationsOfRule(lambda _, line : line.rstrip() == line,
                                    input_api, source_file_filter)
  if errors:
    return [output_api.PresubmitPromptWarning(
        'Found line ending with white spaces in:',
        long_text='\n'.join(errors))]
  return []


def CheckLongLines(input_api, output_api, maxlen, source_file_filter=None):
  """Checks that there aren't any lines longer than maxlen characters in any of
  the text files to be submitted.
  """
  maxlens = {
      'java': 100,
      # This is specifically for Android's handwritten makefiles (Android.mk).
      'mk': 200,
      '': maxlen,
  }

  # Language specific exceptions to max line length.
  # '.h' is considered an obj-c file extension, since OBJC_EXCEPTIONS are a
  # superset of CPP_EXCEPTIONS.
  CPP_FILE_EXTS = ('c', 'cc')
  CPP_EXCEPTIONS = ('#define', '#endif', '#if', '#include', '#pragma')
  HTML_FILE_EXTS = ('html',)
  HTML_EXCEPTIONS = ('<g ', '<link ', '<path ',)
  JAVA_FILE_EXTS = ('java',)
  JAVA_EXCEPTIONS = ('import ', 'package ')
  JS_FILE_EXTS = ('js',)
  JS_EXCEPTIONS = ("GEN('#include",)
  OBJC_FILE_EXTS = ('h', 'm', 'mm')
  OBJC_EXCEPTIONS = ('#define', '#endif', '#if', '#import', '#include',
                     '#pragma')
  PY_FILE_EXTS = ('py',)
  PY_EXCEPTIONS = ('import', 'from')

  LANGUAGE_EXCEPTIONS = [
    (CPP_FILE_EXTS, CPP_EXCEPTIONS),
    (HTML_FILE_EXTS, HTML_EXCEPTIONS),
    (JAVA_FILE_EXTS, JAVA_EXCEPTIONS),
    (JS_FILE_EXTS, JS_EXCEPTIONS),
    (OBJC_FILE_EXTS, OBJC_EXCEPTIONS),
    (PY_FILE_EXTS, PY_EXCEPTIONS),
  ]

  def no_long_lines(file_extension, line):
    # Check for language specific exceptions.
    if any(file_extension in exts and line.lstrip().startswith(exceptions)
           for exts, exceptions in LANGUAGE_EXCEPTIONS):
      return True

    file_maxlen = maxlens.get(file_extension, maxlens[''])
    # Stupidly long symbols that needs to be worked around if takes 66% of line.
    long_symbol = file_maxlen * 2 / 3
    # Hard line length limit at 50% more.
    extra_maxlen = file_maxlen * 3 / 2

    line_len = len(line)
    if line_len <= file_maxlen:
      return True

    # Allow long URLs of any length.
    if any((url in line) for url in ('file://', 'http://', 'https://')):
      return True

    # If 'line-too-long' is explicitly suppressed for the line, any length is
    # acceptable.
    if 'pylint: disable=line-too-long' in line and file_extension == 'py':
      return True

    if line_len > extra_maxlen:
      return False

    if 'url(' in line and file_extension == 'css':
      return True

    if '<include' in line and file_extension in ('css', 'html', 'js'):
      return True

    return input_api.re.match(
        r'.*[A-Za-z][A-Za-z_0-9]{%d,}.*' % long_symbol, line)

  def format_error(filename, line_num, line):
    return '%s, line %s, %s chars' % (filename, line_num, len(line))

  errors = _FindNewViolationsOfRule(no_long_lines, input_api,
                                    source_file_filter,
                                    error_formatter=format_error)
  if errors:
    msg = 'Found lines longer than %s characters (first 5 shown).' % maxlen
    return [output_api.PresubmitPromptWarning(msg, items=errors[:5])]
  else:
    return []


def CheckLicense(input_api, output_api, license_re, source_file_filter=None,
    accept_empty_files=True):
  """Verifies the license header.
  """
  license_re = input_api.re.compile(license_re, input_api.re.MULTILINE)
  bad_files = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    if accept_empty_files and not contents:
      continue
    if not license_re.search(contents):
      bad_files.append(f.LocalPath())
  if bad_files:
    return [output_api.PresubmitPromptWarning(
        'License must match:\n%s\n' % license_re.pattern +
        'Found a bad license header in these files:', items=bad_files)]
  return []


### Other checks

def CheckDoNotSubmit(input_api, output_api):
  return (
      CheckDoNotSubmitInDescription(input_api, output_api) +
      CheckDoNotSubmitInFiles(input_api, output_api)
      )


def CheckTreeIsOpen(input_api, output_api,
                    url=None, closed=None, json_url=None):
  """Check whether to allow commit without prompt.

  Supports two styles:
    1. Checks that an url's content doesn't match a regexp that would mean that
       the tree is closed. (old)
    2. Check the json_url to decide whether to allow commit without prompt.
  Args:
    input_api: input related apis.
    output_api: output related apis.
    url: url to use for regex based tree status.
    closed: regex to match for closed status.
    json_url: url to download json style status.
  """
  if not input_api.is_committing:
    return []
  try:
    if json_url:
      connection = input_api.urllib2.urlopen(json_url)
      status = input_api.json.loads(connection.read())
      connection.close()
      if not status['can_commit_freely']:
        short_text = 'Tree state is: ' + status['general_state']
        long_text = status['message'] + '\n' + json_url
        return [output_api.PresubmitError(short_text, long_text=long_text)]
    else:
      # TODO(bradnelson): drop this once all users are gone.
      connection = input_api.urllib2.urlopen(url)
      status = connection.read()
      connection.close()
      if input_api.re.match(closed, status):
        long_text = status + '\n' + url
        return [output_api.PresubmitError('The tree is closed.',
                                          long_text=long_text)]
  except IOError as e:
    return [output_api.PresubmitError('Error fetching tree status.',
                                      long_text=str(e))]
  return []

def GetUnitTestsInDirectory(
    input_api, output_api, directory, whitelist=None, blacklist=None, env=None):
  """Lists all files in a directory and runs them. Doesn't recurse.

  It's mainly a wrapper for RunUnitTests. Use whitelist and blacklist to filter
  tests accordingly.
  """
  unit_tests = []
  test_path = input_api.os_path.abspath(
      input_api.os_path.join(input_api.PresubmitLocalPath(), directory))

  def check(filename, filters):
    return any(True for i in filters if input_api.re.match(i, filename))

  to_run = found = 0
  for filename in input_api.os_listdir(test_path):
    found += 1
    fullpath = input_api.os_path.join(test_path, filename)
    if not input_api.os_path.isfile(fullpath):
      continue
    if whitelist and not check(filename, whitelist):
      continue
    if blacklist and check(filename, blacklist):
      continue
    unit_tests.append(input_api.os_path.join(directory, filename))
    to_run += 1
  input_api.logging.debug('Found %d files, running %d unit tests'
                          % (found, to_run))
  if not to_run:
    return [
        output_api.PresubmitPromptWarning(
          'Out of %d files, found none that matched w=%r, b=%r in directory %s'
          % (found, whitelist, blacklist, directory))
    ]
  return GetUnitTests(input_api, output_api, unit_tests, env)


def GetUnitTests(input_api, output_api, unit_tests, env=None):
  """Runs all unit tests in a directory.

  On Windows, sys.executable is used for unit tests ending with ".py".
  """
  # We don't want to hinder users from uploading incomplete patches.
  if input_api.is_committing:
    message_type = output_api.PresubmitError
  else:
    message_type = output_api.PresubmitPromptWarning

  results = []
  for unit_test in unit_tests:
    cmd = [unit_test]
    if input_api.verbose:
      cmd.append('--verbose')
    kwargs = {'cwd': input_api.PresubmitLocalPath()}
    if env:
      kwargs['env'] = env
    results.append(input_api.Command(
        name=unit_test,
        cmd=cmd,
        kwargs=kwargs,
        message=message_type))
  return results


def GetUnitTestsRecursively(input_api, output_api, directory,
                            whitelist, blacklist):
  """Gets all files in the directory tree (git repo) that match the whitelist.

  Restricts itself to only find files within the Change's source repo, not
  dependencies.
  """
  def check(filename):
    return (any(input_api.re.match(f, filename) for f in whitelist) and
            not any(input_api.re.match(f, filename) for f in blacklist))

  tests = []

  to_run = found = 0
  for filepath in input_api.change.AllFiles(directory):
    found += 1
    if check(filepath):
      to_run += 1
      tests.append(filepath)
  input_api.logging.debug('Found %d files, running %d' % (found, to_run))
  if not to_run:
    return [
        output_api.PresubmitPromptWarning(
          'Out of %d files, found none that matched w=%r, b=%r in directory %s'
          % (found, whitelist, blacklist, directory))
    ]

  return GetUnitTests(input_api, output_api, tests)


def GetPythonUnitTests(input_api, output_api, unit_tests):
  """Run the unit tests out of process, capture the output and use the result
  code to determine success.

  DEPRECATED.
  """
  # We don't want to hinder users from uploading incomplete patches.
  if input_api.is_committing:
    message_type = output_api.PresubmitError
  else:
    message_type = output_api.PresubmitNotifyResult
  results = []
  for unit_test in unit_tests:
    # Run the unit tests out of process. This is because some unit tests
    # stub out base libraries and don't clean up their mess. It's too easy to
    # get subtle bugs.
    cwd = None
    env = None
    unit_test_name = unit_test
    # 'python -m test.unit_test' doesn't work. We need to change to the right
    # directory instead.
    if '.' in unit_test:
      # Tests imported in submodules (subdirectories) assume that the current
      # directory is in the PYTHONPATH. Manually fix that.
      unit_test = unit_test.replace('.', '/')
      cwd = input_api.os_path.dirname(unit_test)
      unit_test = input_api.os_path.basename(unit_test)
      env = input_api.environ.copy()
      # At least on Windows, it seems '.' must explicitly be in PYTHONPATH
      backpath = [
          '.', input_api.os_path.pathsep.join(['..'] * (cwd.count('/') + 1))
        ]
      if env.get('PYTHONPATH'):
        backpath.append(env.get('PYTHONPATH'))
      env['PYTHONPATH'] = input_api.os_path.pathsep.join((backpath))
      env.pop('VPYTHON_CLEAR_PYTHONPATH', None)
    cmd = [input_api.python_executable, '-m', '%s' % unit_test]
    results.append(input_api.Command(
        name=unit_test_name,
        cmd=cmd,
        kwargs={'env': env, 'cwd': cwd},
        message=message_type))
  return results


def RunUnitTestsInDirectory(input_api, *args, **kwargs):
  """Run tests in a directory serially.

  For better performance, use GetUnitTestsInDirectory and then
  pass to input_api.RunTests.
  """
  return input_api.RunTests(
      GetUnitTestsInDirectory(input_api, *args, **kwargs), False)


def RunUnitTests(input_api, *args, **kwargs):
  """Run tests serially.

  For better performance, use GetUnitTests and then pass to
  input_api.RunTests.
  """
  return input_api.RunTests(GetUnitTests(input_api, *args, **kwargs), False)


def RunPythonUnitTests(input_api, *args, **kwargs):
  """Run python tests in a directory serially.

  DEPRECATED
  """
  return input_api.RunTests(
      GetPythonUnitTests(input_api, *args, **kwargs), False)


def _FetchAllFiles(input_api, white_list, black_list):
  """Hack to fetch all files."""
  # We cannot use AffectedFiles here because we want to test every python
  # file on each single python change. It's because a change in a python file
  # can break another unmodified file.
  # Use code similar to InputApi.FilterSourceFile()
  def Find(filepath, filters):
    if input_api.platform == 'win32':
      filepath = filepath.replace('\\', '/')

    for item in filters:
      if input_api.re.match(item, filepath):
        return True
    return False

  files = []
  path_len = len(input_api.PresubmitLocalPath())
  for dirpath, dirnames, filenames in input_api.os_walk(
      input_api.PresubmitLocalPath()):
    # Passes dirnames in black list to speed up search.
    for item in dirnames[:]:
      filepath = input_api.os_path.join(dirpath, item)[path_len + 1:]
      if Find(filepath, black_list):
        dirnames.remove(item)
    for item in filenames:
      filepath = input_api.os_path.join(dirpath, item)[path_len + 1:]
      if Find(filepath, white_list) and not Find(filepath, black_list):
        files.append(filepath)
  return files


def GetPylint(input_api, output_api, white_list=None, black_list=None,
              disabled_warnings=None, extra_paths_list=None, pylintrc=None):
  """Run pylint on python files.

  The default white_list enforces looking only at *.py files.
  """
  white_list = tuple(white_list or ('.*\.py$',))
  black_list = tuple(black_list or input_api.DEFAULT_BLACK_LIST)
  extra_paths_list = extra_paths_list or []

  if input_api.is_committing:
    error_type = output_api.PresubmitError
  else:
    error_type = output_api.PresubmitPromptWarning

  # Only trigger if there is at least one python file affected.
  def rel_path(regex):
    """Modifies a regex for a subject to accept paths relative to root."""
    def samefile(a, b):
      # Default implementation for platforms lacking os.path.samefile
      # (like Windows).
      return input_api.os_path.abspath(a) == input_api.os_path.abspath(b)
    samefile = getattr(input_api.os_path, 'samefile', samefile)
    if samefile(input_api.PresubmitLocalPath(),
                input_api.change.RepositoryRoot()):
      return regex

    prefix = input_api.os_path.join(input_api.os_path.relpath(
        input_api.PresubmitLocalPath(), input_api.change.RepositoryRoot()), '')
    return input_api.re.escape(prefix) + regex
  src_filter = lambda x: input_api.FilterSourceFile(
      x, map(rel_path, white_list), map(rel_path, black_list))
  if not input_api.AffectedSourceFiles(src_filter):
    input_api.logging.info('Skipping pylint: no matching changes.')
    return []

  if pylintrc is not None:
    pylintrc = input_api.os_path.join(input_api.PresubmitLocalPath(), pylintrc)
  else:
    pylintrc = input_api.os_path.join(_HERE, 'pylintrc')
  extra_args = ['--rcfile=%s' % pylintrc]
  if disabled_warnings:
    extra_args.extend(['-d', ','.join(disabled_warnings)])

  files = _FetchAllFiles(input_api, white_list, black_list)
  if not files:
    return []
  files.sort()

  input_api.logging.info('Running pylint on %d files', len(files))
  input_api.logging.debug('Running pylint on: %s', files)
  env = input_api.environ.copy()
  env['PYTHONPATH'] = input_api.os_path.pathsep.join(
    extra_paths_list).encode('utf8')
  env.pop('VPYTHON_CLEAR_PYTHONPATH', None)
  input_api.logging.debug('  with extra PYTHONPATH: %r', extra_paths_list)

  def GetPylintCmd(flist, extra, parallel):
    # Windows needs help running python files so we explicitly specify
    # the interpreter to use. It also has limitations on the size of
    # the command-line, so we pass arguments via a pipe.
    cmd = [input_api.python_executable,
           input_api.os_path.join(_HERE, 'third_party', 'pylint.py'),
           '--args-on-stdin']
    if len(flist) == 1:
      description = flist[0]
    else:
      description = '%s files' % len(flist)

    args = extra_args[:]
    if extra:
      args.extend(extra)
      description += ' using %s' % (extra,)
    if parallel:
      args.append('--jobs=%s' % input_api.cpu_count)
      description += ' on %d cores' % input_api.cpu_count

    return input_api.Command(
        name='Pylint (%s)' % description,
        cmd=cmd,
        kwargs={'env': env, 'stdin': '\n'.join(args + flist)},
        message=error_type)

  # Always run pylint and pass it all the py files at once.
  # Passing py files one at time is slower and can produce
  # different results.  input_api.verbose used to be used
  # to enable this behaviour but differing behaviour in
  # verbose mode is not desirable.
  # Leave this unreachable code in here so users can make
  # a quick local edit to diagnose pylint issues more
  # easily.
  if True:
    # pylint's cycle detection doesn't work in parallel, so spawn a second,
    # single-threaded job for just that check.

    # Some PRESUBMITs explicitly mention cycle detection.
    if not any('R0401' in a or 'cyclic-import' in a for a in extra_args):
      return [
        GetPylintCmd(files, ["--disable=cyclic-import"], True),
        GetPylintCmd(files, ["--disable=all", "--enable=cyclic-import"], False)
      ]
    else:
      return [ GetPylintCmd(files, [], True) ]

  else:
    return map(lambda x: GetPylintCmd([x], [], 1), files)


def RunPylint(input_api, *args, **kwargs):
  """Legacy presubmit function.

  For better performance, get all tests and then pass to
  input_api.RunTests.
  """
  return input_api.RunTests(GetPylint(input_api, *args, **kwargs), False)


def CheckBuildbotPendingBuilds(input_api, output_api, url, max_pendings,
    ignored):
  try:
    connection = input_api.urllib2.urlopen(url)
    raw_data = connection.read()
    connection.close()
  except IOError:
    return [output_api.PresubmitNotifyResult('%s is not accessible' % url)]

  try:
    data = input_api.json.loads(raw_data)
  except ValueError:
    return [output_api.PresubmitNotifyResult('Received malformed json while '
                                             'looking up buildbot status')]

  out = []
  for (builder_name, builder) in data.iteritems():
    if builder_name in ignored:
      continue
    if builder.get('state', '') == 'offline':
      continue
    pending_builds_len = len(builder.get('pending_builds', []))
    if pending_builds_len > max_pendings:
      out.append('%s has %d build(s) pending' %
                  (builder_name, pending_builds_len))
  if out:
    return [output_api.PresubmitPromptWarning(
        'Build(s) pending. It is suggested to wait that no more than %d '
            'builds are pending.' % max_pendings,
        long_text='\n'.join(out))]
  return []


def CheckOwnersFormat(input_api, output_api):
  affected_files = set([
      f.LocalPath()
      for f in input_api.change.AffectedFiles()
      if 'OWNERS' in f.LocalPath() and f.Action() != 'D'
  ])
  if not affected_files:
    return []
  try:
    input_api.owners_db.load_data_needed_for(affected_files)
    return []
  except Exception as e:
    return [output_api.PresubmitError(
        'Error parsing OWNERS files:\n%s' % e)]


def CheckOwners(input_api, output_api, source_file_filter=None):
  affected_files = set([f.LocalPath() for f in
      input_api.change.AffectedFiles(file_filter=source_file_filter)])
  affects_owners = any('OWNERS' in name for name in affected_files)

  if input_api.is_committing:
    if input_api.tbr and not affects_owners:
      return [output_api.PresubmitNotifyResult(
          '--tbr was specified, skipping OWNERS check')]
    needed = 'LGTM from an OWNER'
    output_fn = output_api.PresubmitError
    if input_api.change.issue:
      if input_api.dry_run:
        output_fn = lambda text: output_api.PresubmitNotifyResult(
            'This is a dry run, but these failures would be reported on ' +
            'commit:\n' + text)
    else:
      return [output_api.PresubmitError(
          'OWNERS check failed: this CL has no Gerrit change number, '
          'so we can\'t check it for approvals.')]
  else:
    needed = 'OWNER reviewers'
    output_fn = output_api.PresubmitNotifyResult

  owners_db = input_api.owners_db
  owners_db.override_files = input_api.change.OriginalOwnersFiles()
  owner_email, reviewers = GetCodereviewOwnerAndReviewers(
      input_api,
      owners_db.email_regexp,
      approval_needed=input_api.is_committing)

  owner_email = owner_email or input_api.change.author_email

  finder = input_api.owners_finder(
      affected_files, input_api.change.RepositoryRoot(),
      owner_email, reviewers, fopen=file, os_path=input_api.os_path,
      email_postfix='', disable_color=True,
      override_files=input_api.change.OriginalOwnersFiles())
  missing_files = finder.unreviewed_files

  if missing_files:
    output_list = [
        output_fn('Missing %s for these files:\n    %s' %
                  (needed, '\n    '.join(sorted(missing_files))))]
    if input_api.tbr and affects_owners:
      output_list.append(output_fn('Note that TBR does not apply to changes '
                                   'that affect OWNERS files.'))
    if not input_api.is_committing:
      suggested_owners = owners_db.reviewers_for(missing_files, owner_email)
      owners_with_comments = []
      def RecordComments(text):
        owners_with_comments.append(finder.print_indent() + text)
      finder.writeln = RecordComments
      for owner in suggested_owners:
        finder.print_comments(owner)
      output_list.append(output_fn('Suggested OWNERS: ' +
          '(Use "git-cl owners" to interactively select owners.)\n    %s' %
          ('\n    '.join(owners_with_comments))))
    return output_list

  if input_api.is_committing and not reviewers:
    return [output_fn('Missing LGTM from someone other than %s' % owner_email)]
  return []


def GetCodereviewOwnerAndReviewers(input_api, email_regexp, approval_needed):
  """Return the owner and reviewers of a change, if any.

  If approval_needed is True, only reviewers who have approved the change
  will be returned.
  """
  issue = input_api.change.issue
  if not issue:
    return None, (set() if approval_needed else
                  _ReviewersFromChange(input_api.change))

  owner_email = input_api.gerrit.GetChangeOwner(issue)
  reviewers = set(
      r for r in input_api.gerrit.GetChangeReviewers(issue, approval_needed)
      if _match_reviewer_email(r, owner_email, email_regexp))
  input_api.logging.debug('owner: %s; approvals given by: %s',
                          owner_email, ', '.join(sorted(reviewers)))
  return owner_email, reviewers


def _ReviewersFromChange(change):
  """Return the reviewers specified in the |change|, if any."""
  reviewers = set()
  reviewers.update(change.ReviewersFromDescription())
  reviewers.update(change.TBRsFromDescription())

  # Drop reviewers that aren't specified in email address format.
  return set(reviewer for reviewer in reviewers if '@' in reviewer)


def _match_reviewer_email(r, owner_email, email_regexp):
  return email_regexp.match(r) and r != owner_email


def CheckSingletonInHeaders(input_api, output_api, source_file_filter=None):
  """Deprecated, must be removed."""
  return [
    output_api.PresubmitNotifyResult(
        'CheckSingletonInHeaders is deprecated, please remove it.')
  ]


def PanProjectChecks(input_api, output_api,
                     excluded_paths=None, text_files=None,
                     license_header=None, project_name=None,
                     owners_check=True, maxlen=80):
  """Checks that ALL chromium orbit projects should use.

  These are checks to be run on all Chromium orbit project, including:
    Chromium
    Native Client
    V8
  When you update this function, please take this broad scope into account.
  Args:
    input_api: Bag of input related interfaces.
    output_api: Bag of output related interfaces.
    excluded_paths: Don't include these paths in common checks.
    text_files: Which file are to be treated as documentation text files.
    license_header: What license header should be on files.
    project_name: What is the name of the project as it appears in the license.
  Returns:
    A list of warning or error objects.
  """
  excluded_paths = tuple(excluded_paths or [])
  text_files = tuple(text_files or (
      r'.+\.txt$',
      r'.+\.json$',
  ))
  project_name = project_name or 'Chromium'

  # Accept any year number from 2006 to the current year, or the special
  # 2006-20xx string used on the oldest files. 2006-20xx is deprecated, but
  # tolerated on old files.
  current_year = int(input_api.time.strftime('%Y'))
  allowed_years = (str(s) for s in reversed(xrange(2006, current_year + 1)))
  years_re = '(' + '|'.join(allowed_years) + '|2006-2008|2006-2009|2006-2010)'

  # The (c) is deprecated, but tolerate it until it's removed from all files.
  license_header = license_header or (
      r'.*? Copyright (\(c\) )?%(year)s The %(project)s Authors\. '
        r'All rights reserved\.\n'
      r'.*? Use of this source code is governed by a BSD-style license that '
        r'can be\n'
      r'.*? found in the LICENSE file\.(?: \*/)?\n'
  ) % {
      'year': years_re,
      'project': project_name,
  }

  results = []
  # This code loads the default black list (e.g. third_party, experimental, etc)
  # and add our black list (breakpad, skia and v8 are still not following
  # google style and are not really living this repository).
  # See presubmit_support.py InputApi.FilterSourceFile for the (simple) usage.
  black_list = input_api.DEFAULT_BLACK_LIST + excluded_paths
  white_list = input_api.DEFAULT_WHITE_LIST + text_files
  sources = lambda x: input_api.FilterSourceFile(x, black_list=black_list)
  text_files = lambda x: input_api.FilterSourceFile(
      x, black_list=black_list, white_list=white_list)

  snapshot_memory = []
  def snapshot(msg):
    """Measures & prints performance warning if a rule is running slow."""
    dt2 = input_api.time.clock()
    if snapshot_memory:
      delta_ms = int(1000*(dt2 - snapshot_memory[0]))
      if delta_ms > 500:
        print "  %s took a long time: %dms" % (snapshot_memory[1], delta_ms)
    snapshot_memory[:] = (dt2, msg)

  snapshot("checking owners files format")
  results.extend(input_api.canned_checks.CheckOwnersFormat(
      input_api, output_api))

  if owners_check:
    snapshot("checking owners")
    results.extend(input_api.canned_checks.CheckOwners(
        input_api, output_api, source_file_filter=None))

  snapshot("checking long lines")
  results.extend(input_api.canned_checks.CheckLongLines(
      input_api, output_api, maxlen, source_file_filter=sources))
  snapshot( "checking tabs")
  results.extend(input_api.canned_checks.CheckChangeHasNoTabs(
      input_api, output_api, source_file_filter=sources))
  snapshot( "checking stray whitespace")
  results.extend(input_api.canned_checks.CheckChangeHasNoStrayWhitespace(
      input_api, output_api, source_file_filter=sources))
  snapshot("checking license")
  results.extend(input_api.canned_checks.CheckLicense(
      input_api, output_api, license_header, source_file_filter=sources))

  if input_api.is_committing:
    snapshot("checking was uploaded")
    results.extend(input_api.canned_checks.CheckChangeWasUploaded(
        input_api, output_api))
    snapshot("checking description")
    results.extend(input_api.canned_checks.CheckChangeHasDescription(
        input_api, output_api))
    results.extend(input_api.canned_checks.CheckDoNotSubmitInDescription(
        input_api, output_api))
    snapshot("checking do not submit in files")
    results.extend(input_api.canned_checks.CheckDoNotSubmitInFiles(
        input_api, output_api))
  snapshot("done")
  return results


def CheckPatchFormatted(input_api, output_api, check_js=False):
  import git_cl
  cmd = ['-C', input_api.change.RepositoryRoot(),
         'cl', 'format', '--dry-run', '--presubmit']
  if check_js:
    cmd.append('--js')
  presubmit_subdir = input_api.os_path.relpath(
      input_api.PresubmitLocalPath(), input_api.change.RepositoryRoot())
  if presubmit_subdir.startswith('..') or presubmit_subdir == '.':
    presubmit_subdir = ''
  # If the PRESUBMIT.py is in a parent repository, then format the entire
  # subrepository. Otherwise, format only the code in the directory that
  # contains the PRESUBMIT.py.
  if presubmit_subdir:
    cmd.append(input_api.PresubmitLocalPath())
  code, _ = git_cl.RunGitWithCode(cmd, suppress_stderr=True)
  if code == 2:
    if presubmit_subdir:
      short_path = presubmit_subdir
    else:
      short_path = input_api.basename(input_api.change.RepositoryRoot())
    return [output_api.PresubmitPromptWarning(
      'The %s directory requires source formatting. '
      'Please run: git cl format %s%s' %
      (short_path, '--js ' if check_js else '', presubmit_subdir))]
  # As this is just a warning, ignore all other errors if the user
  # happens to have a broken clang-format, doesn't use git, etc etc.
  return []


def CheckGNFormatted(input_api, output_api):
  import gn
  affected_files = input_api.AffectedFiles(
      include_deletes=False,
      file_filter=lambda x: x.LocalPath().endswith('.gn') or
                            x.LocalPath().endswith('.gni') or
                            x.LocalPath().endswith('.typemap'))
  warnings = []
  for f in affected_files:
    cmd = ['gn', 'format', '--dry-run', f.AbsoluteLocalPath()]
    rc = gn.main(cmd)
    if rc == 2:
      warnings.append(output_api.PresubmitPromptWarning(
          '%s requires formatting. Please run:\n  gn format %s' % (
              f.AbsoluteLocalPath(), f.LocalPath())))
  # It's just a warning, so ignore other types of failures assuming they'll be
  # caught elsewhere.
  return warnings


def CheckCIPDManifest(input_api, output_api, path=None, content=None):
  """Verifies that a CIPD ensure file manifest is valid against all platforms.

  Exactly one of "path" or "content" must be provided. An assertion will occur
  if neither or both are provided.

  Args:
    path (str): If provided, the filesystem path to the manifest to verify.
    content (str): If provided, the raw content of the manifest to veirfy.
  """
  cipd_bin = 'cipd' if not input_api.is_windows else 'cipd.bat'
  cmd = [cipd_bin, 'ensure-file-verify']
  kwargs = {}

  if input_api.is_windows:
    # Needs to be able to resolve "cipd.bat".
    kwargs['shell'] = True

  if input_api.verbose:
    cmd += ['-log-level', 'debug']

  if path:
    assert content is None, 'Cannot provide both "path" and "content".'
    cmd += ['-ensure-file', path]
    name = 'Check CIPD manifest %r' % path
  elif content:
    assert path is None, 'Cannot provide both "path" and "content".'
    cmd += ['-ensure-file=-']
    kwargs['stdin'] = content
    # quick and dirty parser to extract checked packages.
    packages = [
      l.split()[0] for l in (ll.strip() for ll in content.splitlines())
      if ' ' in l and not l.startswith('$')
    ]
    name = 'Check CIPD packages from string: %r' % (packages,)
  else:
    raise Exception('Exactly one of "path" or "content" must be provided.')

  return input_api.Command(
      name,
      cmd,
      kwargs,
      output_api.PresubmitError)


def CheckCIPDPackages(input_api, output_api, platforms, packages):
  """Verifies that all named CIPD packages can be resolved against all supplied
  platforms.

  Args:
    platforms (list): List of CIPD platforms to verify.
    packages (dict): Mapping of package name to version.
  """
  manifest = []
  for p in platforms:
    manifest.append('$VerifiedPlatform %s' % (p,))
  for k, v in packages.iteritems():
    manifest.append('%s %s' % (k, v))
  return CheckCIPDManifest(input_api, output_api, content='\n'.join(manifest))


def CheckVPythonSpec(input_api, output_api, file_filter=None):
  """Validates any changed .vpython files with vpython verification tool.

  Args:
    input_api: Bag of input related interfaces.
    output_api: Bag of output related interfaces.
    file_filter: Custom function that takes a path (relative to client root) and
      returns boolean, which is used to filter files for which to apply the
      verification to. Defaults to any path ending with .vpython, which captures
      both global .vpython and <script>.vpython files.

  Returns:
    A list of input_api.Command objects containing verification commands.
  """
  file_filter = file_filter or (lambda f: f.LocalPath().endswith('.vpython'))
  affected_files = input_api.AffectedTestableFiles(file_filter=file_filter)
  affected_files = map(lambda f: f.AbsoluteLocalPath(), affected_files)

  commands = []
  for f in affected_files:
    commands.append(input_api.Command(
      'Verify %s' % f,
      ['vpython', '-vpython-spec', f, '-vpython-tool', 'verify'],
      {'stderr': input_api.subprocess.STDOUT},
      output_api.PresubmitError))

  return commands


def CheckChangedLUCIConfigs(input_api, output_api):
  import collections
  import base64
  import json
  import logging
  import urllib2

  import auth
  import git_cl

  LUCI_CONFIG_HOST_NAME = 'luci-config.appspot.com'

  cl = git_cl.Changelist()
  if input_api.change.issue and input_api.gerrit:
    remote_branch = input_api.gerrit.GetDestRef(input_api.change.issue)
  else:
    remote, remote_branch = cl.GetRemoteBranch()
    if remote_branch.startswith('refs/remotes/%s/' % remote):
      remote_branch = remote_branch.replace(
          'refs/remotes/%s/' % remote, 'refs/heads/', 1)
    if remote_branch.startswith('refs/remotes/branch-heads/'):
      remote_branch = remote_branch.replace(
          'refs/remotes/branch-heads/', 'refs/branch-heads/', 1)

  remote_host_url = cl.GetRemoteUrl()
  if not remote_host_url:
    return [output_api.PresubmitError(
        'Remote host url for git has not been defined')]
  remote_host_url = remote_host_url.rstrip('/')
  if remote_host_url.endswith('.git'):
    remote_host_url = remote_host_url[:-len('.git')]

  # authentication
  try:
    authenticator = auth.get_authenticator_for_host(
        LUCI_CONFIG_HOST_NAME, auth.make_auth_config())
    acc_tkn = authenticator.get_access_token()
  except auth.AuthenticationError as e:
    return [output_api.PresubmitError(
        'Error in authenticating user.', long_text=str(e))]

  def request(endpoint, body=None):
    api_url = ('https://%s/_ah/api/config/v1/%s'
               % (LUCI_CONFIG_HOST_NAME, endpoint))
    req = urllib2.Request(api_url)
    req.add_header('Authorization', 'Bearer %s' % acc_tkn.token)
    if body is not None:
      req.add_header('Content-Type', 'application/json')
      req.add_data(json.dumps(body))
    return json.load(urllib2.urlopen(req))

  try:
    config_sets = request('config-sets').get('config_sets')
  except urllib2.HTTPError as e:
    return [output_api.PresubmitError(
        'Config set request to luci-config failed', long_text=str(e))]
  if not config_sets:
    return [output_api.PresubmitWarning('No config_sets were returned')]
  loc_pref = '%s/+/%s/' % (remote_host_url, remote_branch)
  logging.debug('Derived location prefix: %s', loc_pref)
  dir_to_config_set = {
    '%s/' % cs['location'][len(loc_pref):].rstrip('/'): cs['config_set']
    for cs in config_sets
    if cs['location'].startswith(loc_pref) or
    ('%s/' % cs['location']) == loc_pref
  }
  cs_to_files = collections.defaultdict(list)
  for f in input_api.AffectedFiles():
    # windows
    file_path = f.LocalPath().replace(_os.sep, '/')
    logging.debug('Affected file path: %s', file_path)
    for dr, cs in dir_to_config_set.iteritems():
      if dr == '/' or file_path.startswith(dr):
        cs_to_files[cs].append({
          'path': file_path[len(dr):] if dr != '/' else file_path,
          'content': base64.b64encode(
              '\n'.join(f.NewContents()).encode('utf-8'))
        })
  outputs = []
  for cs, f in cs_to_files.iteritems():
    try:
      # TODO(myjang): parallelize
      res = request(
          'validate-config', body={'config_set': cs, 'files': f})
    except urllib2.HTTPError as e:
      return [output_api.PresubmitError(
          'Validation request to luci-config failed', long_text=str(e))]
    for msg in res.get('messages', []):
      sev = msg['severity']
      if sev == 'WARNING':
        out_f = output_api.PresubmitPromptWarning
      elif sev == 'ERROR' or sev == 'CRITICAL':
        out_f = output_api.PresubmitError
      else:
        out_f = output_api.PresubmitNotifyResult
      outputs.append(out_f('Config validation: %s' % msg['text']))
  return outputs
