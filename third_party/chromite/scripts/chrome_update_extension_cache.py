# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generate and upload tarballs for default apps cache.

Run inside the 'files' dir containing 'external_extensions.json' file:
$ chromite/bin/chrome_update_extension_cache --create --upload \\
    chromeos-default-apps-1.0.0

Always increment the version when you update an existing package.
If no new files are added, increment the third version number.
  e.g. 1.0.0 -> 1.0.1
If you change list of default extensions, increment the second version number.
  e.g. 1.0.0 -> 1.1.0

Also you need to regenerate the Manifest with the new tarball digest.
Run inside the chroot:
$ ebuild chromeos-default-apps-1.0.0.ebuild manifest --force
"""

from __future__ import print_function

import json
import os
import urllib
import xml.dom.minidom

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils


UPLOAD_URL_BASE = 'gs://chromeos-localmirror-private/distfiles'


def DownloadCrx(ext, extension, crxdir):
  """Download .crx file from WebStore and update entry."""
  logging.info('Extension "%s"(%s)...', extension['name'], ext)

  update_url = ('%s?x=prodversion%%3D35.1.1.1%%26id%%3D%s%%26uc' %
                (extension['external_update_url'], ext))
  response = urllib.urlopen(update_url)
  if response.getcode() != 200:
    logging.error('Cannot get update response, URL: %s, error: %d', update_url,
                  response.getcode())
    return False

  dom = xml.dom.minidom.parse(response)
  status = dom.getElementsByTagName('app')[0].getAttribute('status')
  if status != 'ok':
    logging.error('Cannot fetch extension, status: %s', status)
    return False

  node = dom.getElementsByTagName('updatecheck')[0]
  url = node.getAttribute('codebase')
  version = node.getAttribute('version')
  filename = '%s-%s.crx' % (ext, version)
  response = urllib.urlopen(url)
  if response.getcode() != 200:
    logging.error('Cannot download extension, URL: %s, error: %d', url,
                  response.getcode())
    return False

  osutils.WriteFile(os.path.join(crxdir, 'extensions', filename),
                    response.read())

  # Keep external_update_url in json file, ExternalCache will take care about
  # replacing it with proper external_crx path and version.

  logging.info('Downloaded, current version %s', version)
  return True


def CreateValidationFiles(validationdir, crxdir, identifier):
  """Create validationfiles for all extensions in |crxdir|."""

  verified_files = []

  # Discover all extensions to be validated (but not JSON files).
  for directory, _, filenames in os.walk(os.path.join(crxdir, 'extensions')):

    # Make directory relative to output dir by removing crxdir and /.
    for filename in filenames:
      verified_files.append(os.path.join(directory[len(crxdir) + 1:],
                                         filename))

  validation_file = os.path.join(validationdir, '%s.validation' % identifier)

  osutils.SafeMakedirs(validationdir)
  cros_build_lib.RunCommand(['sha256sum'] + verified_files,
                            log_stdout_to_file=validation_file,
                            cwd=crxdir, print_cmd=False)
  logging.info('Hashes created.')


def CreateCacheTarball(extensions, outputdir, identifier, tarball):
  """Cache |extensions| in |outputdir| and pack them in |tarball|."""

  crxdir = os.path.join(outputdir, 'crx')
  jsondir = os.path.join(outputdir, 'json')
  validationdir = os.path.join(outputdir, 'validation')

  osutils.SafeMakedirs(os.path.join(crxdir, 'extensions', 'managed_users'))
  osutils.SafeMakedirs(os.path.join(jsondir, 'extensions', 'managed_users'))
  osutils.SafeMakedirs(os.path.join(jsondir, 'extensions', 'child_users'))
  was_errors = False
  for ext in extensions:
    managed_users = extensions[ext].get('managed_users', 'no')
    cache_crx = extensions[ext].get('cache_crx', 'yes')
    child_users = extensions[ext].get('child_users', 'no')

    # Remove fields that shouldn't be in the output file.
    for key in ('cache_crx', 'managed_users'):
      extensions[ext].pop(key, None)

    if cache_crx == 'yes':
      if not DownloadCrx(ext, extensions[ext], crxdir):
        was_errors = True
    elif cache_crx == 'no':
      pass
    else:
      cros_build_lib.Die('Unknown value for "cache_crx" %s for %s',
                         cache_crx, ext)

    if managed_users == 'yes':
      json_file = os.path.join(jsondir,
                               'extensions/managed_users/%s.json' % ext)
      json.dump(extensions[ext],
                open(json_file, 'w'),
                sort_keys=True,
                indent=2,
                separators=(',', ': '))

    if managed_users != 'only':
      target_json_dir = 'extensions'
      if child_users == 'yes':
        target_json_dir = 'extensions/child_users'
      json_file = os.path.join(jsondir, target_json_dir, '%s.json' % ext)
      json.dump(extensions[ext],
                open(json_file, 'w'),
                sort_keys=True,
                indent=2,
                separators=(',', ': '))

  if was_errors:
    cros_build_lib.Die('FAIL to download some extensions')

  CreateValidationFiles(validationdir, crxdir, identifier)
  cros_build_lib.CreateTarball(tarball, outputdir)
  logging.info('Tarball created %s', tarball)


def main(argv):
  parser = commandline.ArgumentParser(
      '%%(prog)s [options] <version>\n\n%s' % __doc__, caching=True)
  parser.add_argument('version', nargs=1)
  parser.add_argument('--path', default=None, type='path',
                      help='Path of files dir with external_extensions.json')
  parser.add_argument('--create', default=False, action='store_true',
                      help='Create cache tarball with specified name')
  parser.add_argument('--upload', default=False, action='store_true',
                      help='Upload cache tarball with specified name')
  options = parser.parse_args(argv)

  if options.path:
    os.chdir(options.path)

  if not (options.create or options.upload):
    cros_build_lib.Die('Need at least --create or --upload args')

  if not os.path.exists('external_extensions.json'):
    cros_build_lib.Die('No external_extensions.json in %s. Did you forget the '
                       '--path option?', os.getcwd())

  identifier = options.version[0]
  tarball = '%s.tar.xz' % identifier
  if options.create:
    extensions = json.load(open('external_extensions.json', 'r'))
    with osutils.TempDir() as tempdir:
      CreateCacheTarball(extensions, tempdir, identifier,
                         os.path.abspath(tarball))

  if options.upload:
    ctx = gs.GSContext()
    url = os.path.join(UPLOAD_URL_BASE, tarball)
    if ctx.Exists(url):
      cros_build_lib.Die('This version already exists on Google Storage (%s)!\n'
                         'NEVER REWRITE EXISTING FILE. IT WILL BREAK CHROME OS '
                         'BUILD!!!', url)
    ctx.Copy(os.path.abspath(tarball), url, acl='project-private')
    logging.info('Tarball uploaded %s', url)
    osutils.SafeUnlink(os.path.abspath(tarball))
