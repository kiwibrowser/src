# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from operator import itemgetter
import random

from data_source import DataSource
from docs_server_utils import MarkLast
from extensions_paths import BROWSER_API_PATHS, BROWSER_CHROME_EXTENSIONS
from future import All
from path_util import Join, Split


_COMMENT_START_MARKER = '#'
_CORE_OWNERS = 'Core Extensions/Apps Owners'
_OWNERS = 'OWNERS'


# Public for testing.
def ParseOwnersFile(content, randomize):
  '''Returns a tuple (owners, notes), where
  |owners| is a list of dicts formed from the owners in |content|,
  |notes| is a string formed from the comments in |content|.
  '''
  if content is None:
    return [], 'Use one of the ' + _CORE_OWNERS + '.'
  owners = []
  notes = []
  for line in content.splitlines():
    if line == '':
      continue
    if line.startswith(_COMMENT_START_MARKER):
      notes.append(line[len(_COMMENT_START_MARKER):].lstrip())
    else:
      # TODO(ahernandez): Mark owners no longer on the project.
      owners.append({'email': line, 'username': line[:line.find('@')]})
  # Randomize the list so owners toward the front of the list aren't
  # diproportionately inundated with reviews.
  if randomize:
    random.shuffle(owners)
  MarkLast(owners)
  return owners, '\n'.join(notes)


class OwnersDataSource(DataSource):
  def __init__(self, server_instance, _, randomize=True):
    self._host_fs = server_instance.host_file_system_provider.GetMaster()
    self._cache = server_instance.object_store_creator.Create(OwnersDataSource)
    self._owners_fs = server_instance.compiled_fs_factory.Create(
        self._host_fs, self._CreateAPIEntry, OwnersDataSource)
    self._randomize = randomize

  def _CreateAPIEntry(self, path, content):
    '''Creates a dict with owners information for an API, specified
    by |owners_file|.
    '''
    owners, notes = ParseOwnersFile(content, self._randomize)
    api_name = Split(path)[-2][:-1]
    return {
      'apiName': api_name,
      'owners': owners,
      'notes': notes,
      'id': api_name
    }

  def _CollectOwnersData(self):
    '''Walks through the file system, collecting owners data from
    API directories.
    '''
    def collect(api_owners):
      if api_owners is not None:
        return api_owners

      # Get API owners from every OWNERS file that exists.
      api_owners = []
      for root in BROWSER_API_PATHS:
        for base, dirs, _ in self._host_fs.Walk(root, depth=1):
          for dir_ in dirs:
            owners_file = Join(root, base, dir_, _OWNERS)
            api_owners.append(
                self._owners_fs.GetFromFile(owners_file, skip_not_found=True))

      # Add an entry for the core extensions/apps owners.
      def fix_core_owners(entry):
        entry['apiName'] = _CORE_OWNERS
        entry['id'] = 'core'
        return entry

      owners_file = Join(BROWSER_CHROME_EXTENSIONS, _OWNERS)
      api_owners.append(self._owners_fs.GetFromFile(owners_file).Then(
          fix_core_owners))
      def sort_and_cache(api_owners):
        api_owners.sort(key=itemgetter('apiName'))
        self._cache.Set('api_owners', api_owners)
        return api_owners
      return All(api_owners).Then(sort_and_cache)
    return self._cache.Get('api_owners').Then(collect)

  def get(self, key):
    return {
      'apis': self._CollectOwnersData()
    }.get(key).Get()

  def Refresh(self):
    return self._CollectOwnersData()
