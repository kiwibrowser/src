# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for subresource_filter component's core/common directory.

See https://www.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

def CheckIndexedRulesetVersion(input_api, output_api):
  """ Checks that IndexedRuleset format version is modified when necessary.

  Whenever any of the following files is changed:
   - components/subresource_filter/core/common/indexed_ruleset.cc
   - components/url_pattern_index/flat/*.fbs
   - components/url_pattern_index/url_pattern_index.cc
  and kIndexedFormatVersion constant stays intact, this check returns a
  presubmit warning to make sure the value should not be updated.

  Additionally, checks to ensure the format version in
  tools/perf/core/default_local_state.json stays up to date with
  kIndexedFormatVersion.
  """

  indexed_ruleset_changed = False
  indexed_ruleset_version_changed = False

  new_indexed_ruleset_version = None

  for affected_file in input_api.AffectedFiles():
    path = affected_file.LocalPath()
    if (not 'components/subresource_filter/core/common' in path and
        not 'components/url_pattern_index/flat' in path):
      continue
    basename = input_api.basename(path)

    if (basename == 'indexed_ruleset.cc' or basename == 'url_pattern_index.cc'
        or basename.endswith('.fbs')):
      indexed_ruleset_changed = True
    if basename == 'indexed_ruleset.cc':
      for (_, line) in affected_file.ChangedContents():
        if 'kIndexedFormatVersion =' in line:
          indexed_ruleset_version_changed = True
          new_indexed_ruleset_version = int(
              indexed_ruleset_line.split()[-1].replace(';',''))
          break

  # If the indexed ruleset version changed, ensure the perf benchmarks are using
  # the new format.
  out = []
  if indexed_ruleset_version_changed:
    assert new_indexed_ruleset_version is not None
    current_path = input_api.PresubmitLocalPath()
    local_state_path = input_api.os_path.join(
        current_path, '..', '..', '..', '..', 'tools', 'perf', 'core',
        'default_local_state.json')

    assert input_api.os_path.exists(local_state_path)
    with open(local_state_path, 'r') as f:
      json_state = input_api.json.load(f)
      version = json_state['subresource_filter']['ruleset_version']['format']
      if new_indexed_ruleset_version != version:
        out.append(output_api.PresubmitPromptWarning(
            'Please make sure that kIndexedFormatVersion (%d) and '
            'the format version in tools/perf/core/default_local_state.json '
            '(%d) are in sync' % (new_indexed_ruleset_version, version)))

  if indexed_ruleset_changed and not indexed_ruleset_version_changed:
    out.append(output_api.PresubmitPromptWarning(
        'Please make sure that UrlPatternIndex/IndexedRuleset modifications in '
        '*.fbs and url_pattern_index.cc/indexed_ruleset.cc do not require '
        'updating RulesetIndexer::kIndexedFormatVersion.'))

  return out

def CheckChangeOnUpload(input_api, output_api):
  return CheckIndexedRulesetVersion(input_api, output_api)
