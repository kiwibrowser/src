#!/usr/bin/env python
# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Makes sure OWNERS files have consistent TEAM and COMPONENT tags."""


import json
import logging
import optparse
import os
import sys
import urllib2

from collections import defaultdict

from owners_file_tags import parse


DEFAULT_MAPPING_URL = \
    'https://storage.googleapis.com/chromium-owners/component_map.json'


def rel_and_full_paths(root, owners_path):
  if root:
    full_path = os.path.join(root, owners_path)
    rel_path = owners_path
  else:
    full_path = os.path.abspath(owners_path)
    rel_path = os.path.relpath(owners_path)
  return rel_path, full_path


def validate_mappings(options, args):
  """Ensure team/component mapping remains consistent after patch.

  The main purpose of this check is to prevent new and edited OWNERS files
  introduce multiple teams for the same component.

  Args:
    options: Command line options from optparse
    args: List of paths to affected OWNERS files
  """
  mappings_file = json.load(urllib2.urlopen(options.current_mapping_url))

  # Convert dir -> component, component -> team to dir -> (team, component)
  current_mappings = {}
  for dir_name in mappings_file['dir-to-component'].keys():
    component = mappings_file['dir-to-component'].get(dir_name)
    if component:
      team = mappings_file['component-to-team'].get(component)
    else:
      team = None
    current_mappings[dir_name] = {'team': team, 'component': component}

  # Extract dir -> (team, component) for affected files
  affected = {}
  deleted = []
  for f in args:
    rel, full = rel_and_full_paths(options.root, f)
    if os.path.exists(full):
      affected[os.path.dirname(rel)] = parse(full)
    else:
      deleted.append(os.path.dirname(rel))
  for d in deleted:
    current_mappings.pop(d, None)
  current_mappings.update(affected)

  #Ensure internal consistency of modified mappings.
  new_dir_to_component = {}
  new_component_to_team = {}
  team_to_dir = defaultdict(list)
  errors = {}
  for dir_name, tags in current_mappings.iteritems():
    team = tags.get('team')
    component = tags.get('component')
    os_tag = tags.get('os')
    if os_tag:
      component = '%s(%s)' % (component, os)

    if component:
      new_dir_to_component[dir_name] = component
    if team:
      team_to_dir[team].append(dir_name)
    if component and team:
      if new_component_to_team.setdefault(component, team) != team:
        if component not in errors:
          errors[component] = set([new_component_to_team[component], team])
        else:
          errors[component].add(team)

  result = []
  for component, teams in errors.iteritems():
    error_message = 'The component "%s" has more than one team: ' % component
    team_details = []
    for team in teams:
      offending_dirs = [d for d in team_to_dir[team]
                        if new_dir_to_component.get(d) == component]
      team_details.append('%(team)s is used in %(paths)s' % {
          'team': team,
          'paths': ', '.join(offending_dirs),
      })
    error_message += '; '.join(team_details)
    result.append({
        'error': error_message,
        'full_path':
            ' '.join(['%s/OWNERS' % d
                      for d, c in new_dir_to_component.iteritems()
                      if c == component and d in affected.keys()])
    })
  return result


def check_owners(rel_path, full_path):
  """Component and Team check in OWNERS files. crbug.com/667954"""
  def result_dict(error):
    return {
      'error': error,
      'full_path': full_path,
      'rel_path': rel_path,
    }

  if not os.path.exists(full_path):
    return

  with open(full_path) as f:
    owners_file_lines = f.readlines()

  component_entries = [l for l in owners_file_lines if l.split()[:2] ==
                       ['#', 'COMPONENT:']]
  team_entries = [l for l in owners_file_lines if l.split()[:2] ==
                  ['#', 'TEAM:']]
  if len(component_entries) > 1:
    return result_dict('Contains more than one component per directory')
  if len(team_entries) > 1:
    return result_dict('Contains more than one team per directory')

  if not component_entries and not team_entries:
    return

  if component_entries:
    component = component_entries[0].split(':')[1]
    if not component:
      return result_dict('Has COMPONENT line but no component name')
    # Check for either of the following formats:
    #   component1, component2, ...
    #   component1,component2,...
    #   component1 component2 ...
    component_count = max(
        len(component.strip().split()),
        len(component.strip().split(',')))
    if component_count > 1:
      return result_dict('Has more than one component name')
    # TODO(robertocn): Check against a static list of valid components,
    # perhaps obtained from monorail at the beginning of presubmit.

  if team_entries:
    team_entry_parts = team_entries[0].split('@')
    if len(team_entry_parts) != 2:
      return result_dict('Has TEAM line, but not exactly 1 team email')
  # TODO(robertocn): Raise a warning if only one of (COMPONENT, TEAM) is
  # present.


def main():
  usage = """Usage: python %prog [--root <dir>] <owners_file1> <owners_file2>...
  owners_fileX  specifies the path to the file to check, these are expected
                to be relative to the root directory if --root is used.

Examples:
  python %prog --root /home/<user>/chromium/src/ tools/OWNERS v8/OWNERS
  python %prog /home/<user>/chromium/src/tools/OWNERS
  python %prog ./OWNERS
  """

  parser = optparse.OptionParser(usage=usage)
  parser.add_option(
      '--root', help='Specifies the repository root.')
  parser.add_option(
      '-v', '--verbose', action='count', default=0, help='Print debug logging')
  parser.add_option(
      '--bare',
      action='store_true',
      default=False,
      help='Prints the bare filename triggering the checks')
  parser.add_option(
      '--current_mapping_url', default=DEFAULT_MAPPING_URL,
      help='URL for existing dir/component and component/team mapping')
  parser.add_option('--json', help='Path to JSON output file')
  options, args = parser.parse_args()

  levels = [logging.ERROR, logging.INFO, logging.DEBUG]
  logging.basicConfig(level=levels[min(len(levels) - 1, options.verbose)])

  errors = filter(None, [check_owners(*rel_and_full_paths(options.root, f))
                         for f in args])

  if not errors:
    errors += validate_mappings(options, args) or []

  if options.json:
    with open(options.json, 'w') as f:
      json.dump(errors, f)

  if errors:
    if options.bare:
      print '\n'.join(e['full_path'] for e in errors)
    else:
      print '\nFAILED\n'
      print '\n'.join('%s: %s' % (e['full_path'], e['error']) for e in errors)
    return 1
  if not options.bare:
    print '\nSUCCESS\n'
  return 0


if '__main__' == __name__:
  sys.exit(main())
