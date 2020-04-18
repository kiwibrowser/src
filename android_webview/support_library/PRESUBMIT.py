# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Presubmit tests for android_webview/support_library/

Runs various style checks before upload.
"""

def CheckChangeOnUpload(input_api, output_api):
  results = []
  results.extend(_CheckAnnotatedInvocationHandlers(input_api, output_api))
  return results

def _CheckAnnotatedInvocationHandlers(input_api, output_api):
  """Checks that all references to InvocationHandlers are annotated with a
  comment describing the class the InvocationHandler represents. This does not
  check .../support_lib_boundary/util/, because this has legitimate reasons to
  refer to InvocationHandlers without them standing for a specific type.
  """

  invocation_handler_str = r'\bInvocationHandler\b'
  annotation_str = r'/\* \w+ \*/\s+'
  invocation_handler_import_pattern = input_api.re.compile(
      r'^import.*' + invocation_handler_str + ';$')
  possibly_annotated_handler_pattern = input_api.re.compile(
      r'(' + annotation_str + r')?(' + invocation_handler_str + r')')

  errors = []

  sources = lambda affected_file: input_api.FilterSourceFile(
      affected_file,
      black_list=(input_api.DEFAULT_BLACK_LIST +
                  (r'.*support_lib_boundary[\\\/]util[\\\/].*',)),
      white_list=(r'.*\.java$',))

  for f in input_api.AffectedSourceFiles(sources):
    for line_num, line in f.ChangedContents():
      if not invocation_handler_import_pattern.search(line):
        for match in possibly_annotated_handler_pattern.findall(line):
          annotation = match[0]
          if not annotation:
            # Note: we intentionally double-count lines which have multiple
            # mistakes, since we require each mention of 'InvocationHandler' to
            # be annotated.
            errors.append("%s:%d" % (f.LocalPath(), line_num))

  results = []

  if errors:
    results.append(output_api.PresubmitPromptWarning("""
All references to InvocationHandlers should be annotated with the type they
represent using a comment, e.g.:

/* RetType */ InvocationHandler method(/* ParamType */ InvocationHandler param);
""",
        errors))

  return results
