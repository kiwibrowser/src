# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re

from collections import defaultdict


def parse(filename):
  """Searches the file for lines that start with `# TEAM:` or `# COMPONENT:`.

  Args:
    filename (str): path to the file to parse.
  Returns:
    a dict with the following format, with any subset of the listed keys:
    {
        'component': 'component>name',
        'team': 'team@email.here',
        'os': 'Linux|Windows|Mac|Android|Chrome|Fuchsia'
    }
 """
  team_regex = re.compile('\s*#\s*TEAM\s*:\s*(\S+)')
  component_regex = re.compile('\s*#\s*COMPONENT\s*:\s*(\S+)')
  os_regex = re.compile('\s*#\s*OS\s*:\s*(\S+)')
  result = {}
  with open(filename) as f:
    for line in f:
      team_matches = team_regex.match(line)
      if team_matches:
        result['team'] = team_matches.group(1)
      component_matches = component_regex.match(line)
      if component_matches:
        result['component'] = component_matches.group(1)
      os_matches = os_regex.match(line)
      if os_matches:
        result['os'] = os_matches.group(1)
  return result


def aggregate_components_from_owners(all_owners_data, root,
                                     include_subdirs=False):
  """Converts the team/component/os tags parsed from OWNERS into mappings.

  Args:
    all_owners_data (dict): A mapping from relative path to a dir to a dict
        mapping the tag names to their values. See docstring for scrape_owners.
    root (str): the path to the src directory.
    include_subdirs (bool): Deprecated, whether to generate the additional
        dir-to-team mapping. This mapping is being replaced by the result of
        scrape_owners below.

  Returns:
    A tuple (data, warnings, errors, stats) where data is a dict of the form
      {'component-to-team': {'Component1': 'team1@chr...', ...},
       'dir-to-component': {'/path/to/1': 'Component1', ...}}
      , warnings is a list of strings, stats is a dict of form
      {'OWNERS-count': total number of OWNERS files,
       'OWNERS-with-component-only-count': number of OWNERS have # COMPONENT,
       'OWNERS-with-team-and-component-count': number of
                          OWNERS have TEAM and COMPONENT,
       'OWNERS-count-by-depth': {directory depth: number of OWNERS},
       'OWNERS-with-component-only-count-by-depth': {directory depth: number
                          of OWNERS have COMPONENT at this depth},
       'OWNERS-with-team-and-component-count-by-depth':{directory depth: ...}}
  """
  stats = {}
  num_total = 0
  num_with_component = 0
  num_with_team_component = 0
  num_total_by_depth = defaultdict(int)
  num_with_component_by_depth = defaultdict(int)
  num_with_team_component_by_depth = defaultdict(int)
  warnings = []
  component_to_team = defaultdict(set)
  dir_to_component = {}
  dir_missing_info_by_depth = defaultdict(list)
  # TODO(sergiyb): Remove this mapping. Please do not use it as it is going to
  # be removed in the future. See http://crbug.com/702202.
  dir_to_team = {}
  for rel_dirname, owners_data in all_owners_data.iteritems():
    # We apply relpath to remove any possible `.` and `..` chunks and make
    # counting separators work correctly as a means of obtaining the file_depth.
    rel_path = os.path.relpath(rel_dirname, root)
    file_depth = 0 if rel_path == '.' else rel_path.count(os.path.sep) + 1
    num_total += 1
    num_total_by_depth[file_depth] += 1
    component = owners_data.get('component')
    team = owners_data.get('team')
    os_tag = owners_data.get('os')
    if os_tag and component:
      component = '%s(%s)' % (component, os_tag)
    if component:
      num_with_component += 1
      num_with_component_by_depth[file_depth] += 1
      dir_to_component[rel_dirname] = component
      if team:
        num_with_team_component += 1
        num_with_team_component_by_depth[file_depth] += 1
        component_to_team[component].add(team)
    else:
      rel_owners_path = os.path.join(rel_dirname, 'OWNERS')
      warnings.append('%s has no COMPONENT tag' % rel_owners_path)
      if not team and not os_tag:
        dir_missing_info_by_depth[file_depth].append(rel_owners_path)

    # TODO(robertocn): Remove the dir-to-team mapping once the raw owners data
    # is being exported in its own file and being used by sergiyb's scripts.
    # Add dir-to-team mapping unless there is also dir-to-component mapping.
    if (include_subdirs and team and not component and
        rel_dirname.startswith('third_party/WebKit/LayoutTests')):
      dir_to_team[rel_dirname] = team

    if include_subdirs and rel_dirname not in dir_to_component:
      rel_parent_dirname = os.path.relpath(rel_dirname, root)
      if rel_parent_dirname in dir_to_component:
        dir_to_component[rel_dirname] = dir_to_component[rel_parent_dirname]
      if rel_parent_dirname in dir_to_team:
        dir_to_team[rel_dirname] = dir_to_team[rel_parent_dirname]

  mappings = {'component-to-team': component_to_team,
              'dir-to-component': dir_to_component}
  if include_subdirs:
    mappings['dir-to-team'] = dir_to_team
  errors = validate_one_team_per_component(mappings)
  stats = {'OWNERS-count': num_total,
           'OWNERS-with-component-only-count': num_with_component,
           'OWNERS-with-team-and-component-count': num_with_team_component,
           'OWNERS-count-by-depth': num_total_by_depth,
           'OWNERS-with-component-only-count-by-depth':
           num_with_component_by_depth,
           'OWNERS-with-team-and-component-count-by-depth':
           num_with_team_component_by_depth,
           'OWNERS-missing-info-by-depth':
           dir_missing_info_by_depth}
  return unwrap(mappings), warnings, errors, stats


def validate_one_team_per_component(m):
  """Validates that each component is associated with at most 1 team."""
  errors = []
  # TODO(robertocn): Validate the component names: crbug.com/679540
  component_to_team = m['component-to-team']
  for c in component_to_team:
    if len(component_to_team[c]) > 1:
      errors.append('Component %s has more than one team assigned to it: %s' % (
          c, ', '.join(list(component_to_team[c]))))
  return errors


def unwrap(mappings):
  """Remove the set() wrapper around values in component-to-team mapping."""
  for c in mappings['component-to-team']:
    mappings['component-to-team'][c] = mappings['component-to-team'][c].pop()
  return mappings

def scrape_owners(root, include_subdirs):
  """Recursively parse OWNERS files for tags.

  Args:
    root (str): The directory where to start parsing.
    include_subdirs (bool): Whether to generate entries for subdirs with no
        own OWNERS files based on the parent dir's tags.

  Returns a dict in the form below.
  {
      '/path/to/dir': {
          'component': 'component>name',
          'team': 'team@email.here',
          'os': 'Linux|Windows|Mac|Android|Chrome|Fuchsia'
      },
      '/path/to/dir/inside/dir': {
          'component': ...
      }
  }
  """
  data = {}
  for dirname, _, files in os.walk(root):
    # Proofing against windows casing oddities.
    owners_file_names = [f for f in files if f.upper() == 'OWNERS']
    rel_dirname = os.path.relpath(dirname, root)
    if owners_file_names:
      owners_full_path = os.path.join(dirname, owners_file_names[0])
      data[rel_dirname] = parse(owners_full_path)
    if include_subdirs and not data.get(rel_dirname):
      parent_dirname = os.path.dirname(dirname)
      # In the case where the root doesn't have an OWNERS file, don't try to
      # check its parent.
      if parent_dirname:
        rel_parent_dirname = os.path.relpath(parent_dirname , root)
        if rel_parent_dirname in data:
          data[rel_dirname] = data[rel_parent_dirname]
  return data
