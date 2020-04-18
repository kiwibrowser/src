#
# Copyright 2015 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Custom script to run PyLint on apitools codebase.

"Inspired" by the similar script in gcloud-python.

This runs pylint as a script via subprocess in two different
subprocesses. The first lints the production/library code
using the default rc file (PRODUCTION_RC). The second lints the
demo/test code using an rc file (TEST_RC) which allows more style
violations (hence it has a reduced number of style checks).
"""

import ConfigParser
import copy
import os
import subprocess
import sys


IGNORED_DIRECTORIES = [
    'apitools/gen/testdata',
    'samples/bigquery_sample/bigquery_v2',
    'samples/dns_sample/dns_v1',
    'samples/fusiontables_sample/fusiontables_v1',
    'samples/iam_sample/iam_v1',
    'samples/servicemanagement_sample/servicemanagement_v1',
    'samples/storage_sample/storage_v1',
    'venv',
]
IGNORED_FILES = [
    'ez_setup.py',
    'run_pylint.py',
    'setup.py',
    'apitools/base/py/gzip.py',
    'apitools/base/py/gzip_test.py',
]
PRODUCTION_RC = 'default.pylintrc'
TEST_RC = 'reduced.pylintrc'
TEST_DISABLED_MESSAGES = [
    'exec-used',
    'invalid-name',
    'missing-docstring',
    'protected-access',
]
TEST_RC_ADDITIONS = {
    'MESSAGES CONTROL': {
        'disable': ',\n'.join(TEST_DISABLED_MESSAGES),
    },
}


def read_config(filename):
    """Reads pylintrc config onto native ConfigParser object."""
    config = ConfigParser.ConfigParser()
    with open(filename, 'r') as file_obj:
        config.readfp(file_obj)
    return config


def make_test_rc(base_rc_filename, additions_dict, target_filename):
    """Combines a base rc and test additions into single file."""
    main_cfg = read_config(base_rc_filename)

    # Create fresh config for test, which must extend production.
    test_cfg = ConfigParser.ConfigParser()
    test_cfg._sections = copy.deepcopy(main_cfg._sections)

    for section, opts in additions_dict.items():
        curr_section = test_cfg._sections.setdefault(
            section, test_cfg._dict())
        for opt, opt_val in opts.items():
            curr_val = curr_section.get(opt)
            if curr_val is None:
                raise KeyError('Expected to be adding to existing option.')
            curr_section[opt] = '%s\n%s' % (curr_val, opt_val)

    with open(target_filename, 'w') as file_obj:
        test_cfg.write(file_obj)


def valid_filename(filename):
    """Checks if a file is a Python file and is not ignored."""
    for directory in IGNORED_DIRECTORIES:
        if filename.startswith(directory):
            return False
    return (filename.endswith('.py') and
            filename not in IGNORED_FILES)


def is_production_filename(filename):
    """Checks if the file contains production code.

    :rtype: boolean
    :returns: Boolean indicating production status.
    """
    return not ('demo' in filename or 'test' in filename or
                filename.startswith('regression'))


def get_files_for_linting(allow_limited=True, diff_base=None):
    """Gets a list of files in the repository.

    By default, returns all files via ``git ls-files``. However, in some cases
    uses a specific commit or branch (a so-called diff base) to compare
    against for changed files. (This requires ``allow_limited=True``.)

    To speed up linting on Travis pull requests against master, we manually
    set the diff base to origin/master. We don't do this on non-pull requests
    since origin/master will be equivalent to the currently checked out code.
    One could potentially use ${TRAVIS_COMMIT_RANGE} to find a diff base but
    this value is not dependable.

    :type allow_limited: boolean
    :param allow_limited: Boolean indicating if a reduced set of files can
                          be used.

    :rtype: pair
    :returns: Tuple of the diff base using the the list of filenames to be
              linted.
    """
    if os.getenv('TRAVIS') == 'true':
        # In travis, don't default to master.
        diff_base = None

    if (os.getenv('TRAVIS_BRANCH') == 'master' and
            os.getenv('TRAVIS_PULL_REQUEST') != 'false'):
        # In the case of a pull request into master, we want to
        # diff against HEAD in master.
        diff_base = 'origin/master'

    if diff_base is not None and allow_limited:
        result = subprocess.check_output(['git', 'diff', '--name-only',
                                          diff_base])
        print 'Using files changed relative to %s:' % (diff_base,)
        print '-' * 60
        print result.rstrip('\n')  # Don't print trailing newlines.
        print '-' * 60
    else:
        print 'Diff base not specified, listing all files in repository.'
        result = subprocess.check_output(['git', 'ls-files'])

    return result.rstrip('\n').split('\n'), diff_base


def get_python_files(all_files=None, diff_base=None):
    """Gets a list of all Python files in the repository that need linting.

    Relies on :func:`get_files_for_linting()` to determine which files should
    be considered.

    NOTE: This requires ``git`` to be installed and requires that this
          is run within the ``git`` repository.

    :type all_files: list or ``NoneType``
    :param all_files: Optional list of files to be linted.

    :rtype: tuple
    :returns: A tuple containing two lists and a boolean. The first list
              contains all production files, the next all test/demo files and
              the boolean indicates if a restricted fileset was used.
    """
    using_restricted = False
    if all_files is None:
        all_files, diff_base = get_files_for_linting(diff_base=diff_base)
        using_restricted = diff_base is not None

    library_files = []
    non_library_files = []
    for filename in all_files:
        if valid_filename(filename):
            if is_production_filename(filename):
                library_files.append(filename)
            else:
                non_library_files.append(filename)

    return library_files, non_library_files, using_restricted


def lint_fileset(filenames, rcfile, description):
    """Lints a group of files using a given rcfile."""
    # Only lint filenames that exist. For example, 'git diff --name-only'
    # could spit out deleted / renamed files. Another alternative could
    # be to use 'git diff --name-status' and filter out files with a
    # status of 'D'.
    filenames = [filename for filename in filenames
                 if os.path.exists(filename)]
    if filenames:
        rc_flag = '--rcfile=%s' % (rcfile,)
        pylint_shell_command = ['pylint', rc_flag] + filenames
        status_code = subprocess.call(pylint_shell_command)
        if status_code != 0:
            error_message = ('Pylint failed on %s with '
                             'status %d.' % (description, status_code))
            print >> sys.stderr, error_message
            sys.exit(status_code)
    else:
        print 'Skipping %s, no files to lint.' % (description,)


def main(argv):
    """Script entry point. Lints both sets of files."""
    diff_base = argv[1] if len(argv) > 1 else None
    make_test_rc(PRODUCTION_RC, TEST_RC_ADDITIONS, TEST_RC)
    library_files, non_library_files, using_restricted = get_python_files(
        diff_base=diff_base)
    try:
        lint_fileset(library_files, PRODUCTION_RC, 'library code')
        lint_fileset(non_library_files, TEST_RC, 'test and demo code')
    except SystemExit:
        if not using_restricted:
            raise

        message = 'Restricted lint failed, expanding to full fileset.'
        print >> sys.stderr, message
        all_files, _ = get_files_for_linting(allow_limited=False)
        library_files, non_library_files, _ = get_python_files(
            all_files=all_files)
        lint_fileset(library_files, PRODUCTION_RC, 'library code')
        lint_fileset(non_library_files, TEST_RC, 'test and demo code')


if __name__ == '__main__':
    main(sys.argv)
