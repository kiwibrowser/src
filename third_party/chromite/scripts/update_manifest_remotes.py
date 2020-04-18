# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A tool that updates remotes in all historical manifests to point to GoB.

It clones manifest-versions repository, scans through all manifests there and
replaces known old gerrit/gerrit-int URLs with Gerrit on Borg ones.

It doesn't commit or push any changes, just updates files in a working
directory.
"""

from __future__ import print_function

import collections
import os

from xml.etree import ElementTree

from chromite.lib import config_lib
from chromite.cbuildbot import manifest_version
from chromite.lib import commandline
from chromite.lib import cros_logging as logging
from chromite.lib import osutils


site_config = config_lib.GetConfig()


GOB_EXTERNAL = 'https://chromium.googlesource.com'
GOB_INTERNAL = 'https://chrome-internal.googlesource.com'


GERRIT_EXTERNAL = 'https://chromium-review.googlesource.com'
GERRIT_INTERNAL = 'https://chrome-internal-review.googlesource.com'


# Old fetch URL -> new fetch URL.
# Old fetch urls are found by grepping through manifest-versions repo.
FETCH_URLS = {
    'http://git.chromium.org': GOB_EXTERNAL,
    'http://git.chromium.org/git': GOB_EXTERNAL,
    'https://git.chromium.org/git': GOB_EXTERNAL,
    'ssh://gerrit.chromium.org:29418': GOB_EXTERNAL,
    'ssh://git@gitrw.chromium.org:9222': GOB_EXTERNAL,
    'ssh://gerrit-int.chromium.org:29419': GOB_INTERNAL,
}


# Old review URL -> new review URL.
REVIEW_URLS = {
    'gerrit.chromium.org/gerrit': GERRIT_EXTERNAL,
    'gerrit-int.chromium.org': GERRIT_INTERNAL,
}


# Single remote entry in a manifest.
Remote = collections.namedtuple('Remote', ['name', 'fetch', 'review'])


def EnumerateManifests(directory):
  """Yields paths to manifest files inside a directory."""
  for path, directories, files in os.walk(directory):
    # Find regular (not a symlink) xml files.
    for name in files:
      if not name.endswith('.xml'):
        continue
      full_path = os.path.join(path, name)
      if os.path.isfile(full_path) and not os.path.islink(full_path):
        yield full_path
    # Skip 'hidden' directories.
    for hidden in [name for name in directories if name.startswith('.')]:
      directories.remove(hidden)


def UpdateRemotes(manifest):
  """Updates remotes in manifest to use Gerrit on Borg URLs.

  Args:
    manifest: Path to manifest file to modify in place.

  Returns:
    True if file was modified.
  """
  # Read manifest file as str.
  body = osutils.ReadFile(manifest)
  original = body

  # Update fetch="..." entries.
  for old, new in FETCH_URLS.iteritems():
    body = body.replace('fetch="%s"' % old, 'fetch="%s"' % new)

  # Update review="..." entries.
  for old, new in REVIEW_URLS.iteritems():
    body = body.replace('review="%s"' % old, 'review="%s"' % new)

  # Write back only if modified.
  if original != body:
    osutils.WriteFile(manifest, body)
    return True

  return False


def GetRemotes(manifest):
  """Returns list of remotes referenced in manifest.

  Args:
    manifest: Path to manifest file to scan for remotes.

  Returns:
    List of Remote tuples.
  """
  doc = ElementTree.parse(manifest)
  root = doc.getroot()
  return [Remote(
      remote.attrib['name'], remote.attrib['fetch'],
      remote.attrib.get('review'))
          for remote in root.findall('remote')]


def GetParser():
  """Creates the argparse parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument(
      '--skip-update', action='store_true', default=False,
      help='Do not revert versions manifest checkout to original state')
  parser.add_argument(
      '--remotes-summary', action='store_true', default=False,
      help='Scan all manifests and print all various remotes found in them')
  parser.add_argument(
      'manifest_versions_dir', type='path',
      help='Directory to checkout manifest versions repository into')
  return parser


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)

  # Clone manifest-versions repository.
  manifest_repo_url = site_config.params.MANIFEST_VERSIONS_INT_GOB_URL
  if not options.skip_update:
    manifest_version.RefreshManifestCheckout(
        options.manifest_versions_dir, manifest_repo_url)

  if options.remotes_summary:
    # Find all unique remotes.
    logging.info('Scanning manifests for remotes...')
    remotes = set()
    for manifest in EnumerateManifests(options.manifest_versions_dir):
      remotes.update(GetRemotes(manifest))
    # Pretty print a table.
    print('Remotes found:')
    row_formatter = lambda a, b, c: ''.join(
        [a, ' ' * (16 - len(a)), b, ' ' * (45 - len(b)), c])
    print(row_formatter('Name', 'Remote', 'Review'))
    print('-' * 80)
    for remote in sorted(remotes):
      print(row_formatter(remote.name, remote.fetch, remote.review or ''))
    return 0

  logging.info('Updating manifests...')
  up_to_date = True
  for manifest in EnumerateManifests(options.manifest_versions_dir):
    if UpdateRemotes(manifest):
      up_to_date = False
      logging.info('Updated manifest: %s', manifest)

  if up_to_date:
    logging.info('All manifests are up to date')
  else:
    logging.info('Done')

  return 0
