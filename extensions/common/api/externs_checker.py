# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

class ExternsChecker(object):
  _UPDATE_MESSAGE = """To update the externs, run:
 src/ $ python tools/json_schema_compiler/compiler.py\
 %s --root=. --generator=externs > %s"""

  def __init__(self, input_api, output_api, api_pairs):
    self._input_api = input_api
    self._output_api = output_api
    self._api_pairs = api_pairs

    for path in api_pairs.keys() + api_pairs.values():
      if not input_api.os_path.exists(path):
        raise OSError('Path Not Found: %s' % path)

  def RunChecks(self):
    bad_files = []
    affected = [f.AbsoluteLocalPath() for f in
                   self._input_api.change.AffectedFiles()]
    for path in affected:
      pair = self._api_pairs.get(path)
      if pair != None and pair not in affected:
        bad_files.append({'source': path, 'extern': pair})
    results = []
    if bad_files:
      replacements = (('<source_file>', '<output_file>') if len(bad_files) > 1
          else (bad_files[0]['source'], bad_files[0]['extern']))
      long_text = self._UPDATE_MESSAGE % replacements
      results.append(self._output_api.PresubmitPromptWarning(
          str('Found updated extension api files without updated extern files. '
              'Please update the extern files.'),
          [f['source'] for f in bad_files],
          long_text))
    return results
