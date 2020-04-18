# Copyright 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Makes sure that injected JavaScript is gjslint clean."""

# Regular expression patterns for JavaScript source files.
INCLUDE_JS_FILES_ONLY = (
  r'.*\.js$',
)

# List of standard locations where gjslint may be installed.
GJSLINT_PATHS = (
  '/usr/local/bin/gjslint',
  '/usr/bin/gjslint',
)

def CheckChangeJsLintsClean(input_api, output_api):
  """Checks for JavaScript lint errors."""
  # Gets the list of modified files.
  source_file_filter = lambda x: input_api.FilterSourceFile(
      x, white_list=INCLUDE_JS_FILES_ONLY)
  files = [f.AbsoluteLocalPath()
      for f in input_api.AffectedSourceFiles(source_file_filter)]
  results = []

  # Run gjslint only if there are JavaScript files changed.
  if files:
    # Finds out where gjslint is installed, and warns user
    # gently about this missing utility. Then treats it as
    # if there are no JavaScript lint errors.
    gjslint_path = None
    for path in GJSLINT_PATHS:
      if input_api.os_path.exists(path):
        gjslint_path = path
        break

    if not gjslint_path:
      # Early return if gjslint is not available.
      return [ output_api.PresubmitNotifyResult(
          '** WARNING ** gjslint is not installed in your environment.',
          GJSLINT_PATHS,
          ('Please see http://code.google.com/'
           'closure/utilities/docs/linter_howto.html '
           'for installation instructions.\n'
           'You are strongly encouraged to install gjslint '
           'because a kitten dies every time you submit code '
           'without gjslint :)')) ]

    # Found gjslint, run it over modified JavaScript files.
    for file_name in files:
      lint_error = RunJsLint(input_api, output_api, gjslint_path, file_name)
      if lint_error is not None:
        results.append(lint_error)

  return results

def RunJsLint(input_api, output_api, gjslint_path, file_name):
  """Runs gjslint on a file.

  Returns: None if there are no lint errors, or an object of type
  output_api.Presubmit{Error,PromptWarning} if lint errors are found.
  """
  # Do not hinder users from uploading incomplete patches.
  if input_api.is_committing:
    message_type = output_api.PresubmitError
  else:
    message_type = output_api.PresubmitPromptWarning

  result = None
  cmd = [ gjslint_path, file_name ];
  try:
    if input_api.verbose:
      input_api.subprocess.check_call(cmd, cwd=input_api.PresubmitLocalPath())
    else:
      input_api.subprocess.check_output(
          cmd,
          stderr=input_api.subprocess.STDOUT,
          cwd=input_api.PresubmitLocalPath())
  except (OSError, input_api.subprocess.CalledProcessError), e:
    result = message_type('%s failed.\n%s' % (' '.join(cmd), e))
  return result

def CheckChangeOnUpload(input_api, output_api):
  """Special Top level function called by git_cl."""
  return CheckChangeJsLintsClean(input_api, output_api)

