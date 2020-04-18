# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for Chromium JS resources.

See chrome/browser/PRESUBMIT.py
"""

import regex_check


class JSChecker(object):
  def __init__(self, input_api, output_api, file_filter=None):
    self.input_api = input_api
    self.output_api = output_api
    self.file_filter = file_filter

  def RegexCheck(self, line_number, line, regex, message):
    return regex_check.RegexCheck(
        self.input_api.re, line_number, line, regex, message)

  def ChromeSendCheck(self, i, line):
    """Checks for a particular misuse of 'chrome.send'."""
    return self.RegexCheck(i, line, r"chrome\.send\('[^']+'\s*(, \[\])\)",
        'Passing an empty array to chrome.send is unnecessary')

  def CommentIfAndIncludeCheck(self, line_number, line):
    return self.RegexCheck(line_number, line, r'(?<!\/\/ )(<if|<include) ',
        '<if> or <include> should be in a single line comment with a space ' +
        'after the slashes. Examples:\n' +
        '    // <include src="...">\n' +
        '    // <if expr="chromeos">\n' +
        '    // </if>\n')

  def EndJsDocCommentCheck(self, i, line):
    msg = 'End JSDoc comments with */ instead of **/'
    def _check(regex):
      return self.RegexCheck(i, line, regex, msg)
    return _check(r'^\s*(\*\*/)\s*$') or _check(r'/\*\* @[a-zA-Z]+.* (\*\*/)')

  def ExtraDotInGenericCheck(self, i, line):
    return self.RegexCheck(i, line, r"((?:Array|Object|Promise)\.<)",
        "Don't use a dot after generics (Object.<T> should be Object<T>).")

  def InheritDocCheck(self, i, line):
    """Checks for use of '@inheritDoc' instead of '@override'."""
    return self.RegexCheck(i, line, r"\* (@inheritDoc)",
        "@inheritDoc is deprecated, use @override instead")

  def PolymerLocalIdCheck(self, i, line):
    """Checks for use of element.$.localId."""
    return self.RegexCheck(i, line, r"(?<!this)(\.\$)[\[\.]",
        "Please only use this.$.localId, not element.$.localId")

  def RunEsLintChecks(self, affected_js_files, format='stylish'):
    """Runs lint checks using ESLint. The ESLint rules being applied are defined
       in the .eslintrc.js configuration file.
    """
    os_path = self.input_api.os_path

    try:
      # Import ESLint.
      _HERE_PATH = os_path.dirname(os_path.realpath(__file__))
      _SRC_PATH = os_path.normpath(os_path.join(_HERE_PATH, '..', '..'))
      import sys
      old_sys_path = sys.path[:]
      sys.path.append(os_path.join(_SRC_PATH, 'third_party', 'node'))
      import node, node_modules
    finally:
      sys.path = old_sys_path

    # Extract paths to be passed to ESLint.
    affected_js_files_paths = []
    presubmit_path = self.input_api.PresubmitLocalPath()
    for f in affected_js_files:
      affected_js_files_paths.append(
          os_path.relpath(f.AbsoluteLocalPath(), presubmit_path))

    output = node.RunNode([
        node_modules.PathToEsLint(),
        '--color',
        '--format', format,
        '--ignore-pattern \'!.eslintrc.js\'',
        ' '.join(affected_js_files_paths)])

    return [self.output_api.PresubmitError(output)] if output else []

  def VariableNameCheck(self, i, line):
    """See the style guide. http://goo.gl/eQiXVW"""
    return self.RegexCheck(i, line,
        r"(?:var|let|const) (?!g_\w+)(_?[a-z][a-zA-Z]*[_$][\w_$]*)(?<! \$)",
        "Please use variable namesLikeThis <https://goo.gl/eQiXVW>")

  def _GetErrorHighlight(self, start, length):
    """Takes a start position and a length, and produces a row of '^'s to
       highlight the corresponding part of a string.
    """
    return start * ' ' + length * '^'

  def RunChecks(self):
    """Check for violations of the Chromium JavaScript style guide. See
       https://chromium.googlesource.com/chromium/src/+/master/styleguide/web/web.md#JavaScript
    """
    results = []

    affected_files = self.input_api.AffectedFiles(file_filter=self.file_filter,
                                                  include_deletes=False)
    affected_js_files = filter(lambda f: f.LocalPath().endswith('.js'),
                               affected_files)

    if affected_js_files:
      results += self.RunEsLintChecks(affected_js_files)

    for f in affected_js_files:
      error_lines = []

      for i, line in enumerate(f.NewContents(), start=1):
        error_lines += filter(None, [
            self.ChromeSendCheck(i, line),
            self.CommentIfAndIncludeCheck(i, line),
            self.EndJsDocCommentCheck(i, line),
            self.ExtraDotInGenericCheck(i, line),
            self.InheritDocCheck(i, line),
            self.PolymerLocalIdCheck(i, line),
            self.VariableNameCheck(i, line),
        ])

      if error_lines:
        error_lines = [
            'Found JavaScript style violations in %s:' %
            f.LocalPath()] + error_lines
        results.append(self.output_api.PresubmitError('\n'.join(error_lines)))

    if results:
      results.append(self.output_api.PresubmitNotifyResult(
          'See the JavaScript style guide at '
          'https://chromium.googlesource.com/chromium/src/+/master/styleguide/web/web.md#JavaScript'))

    return results
