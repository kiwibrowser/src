# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import posixpath
import re

from compiled_file_system import Cache
from extensions_paths import EXAMPLES
from samples_data_source import SamplesDataSource
import third_party.json_schema_compiler.json_comment_eater as json_comment_eater
import url_constants


_DEFAULT_ICON_PATH = 'images/sample-default-icon.png'


def _GetAPIItems(js_file):
  chrome_pattern = r'chrome[\w.]+'
  # Add API calls that appear normally, like "chrome.runtime.connect".
  calls = set(re.findall(chrome_pattern, js_file))
  # Add API calls that have been assigned into variables, like
  # "var storageArea = chrome.storage.sync; storageArea.get", which should
  # be expanded like "chrome.storage.sync.get".
  for match in re.finditer(r'var\s+(\w+)\s*=\s*(%s);' % chrome_pattern,
                           js_file):
    var_name, api_prefix = match.groups()
    for var_match in re.finditer(r'\b%s\.([\w.]+)\b' % re.escape(var_name),
                                 js_file):
      api_suffix, = var_match.groups()
      calls.add('%s.%s' % (api_prefix, api_suffix))
  return calls


class SamplesModel(object):
  def __init__(self,
               extension_samples_file_system,
               app_samples_file_system,
               compiled_fs_factory,
               reference_resolver,
               base_path,
               platform):
    self._samples_fs = (extension_samples_file_system if
                        platform == 'extensions' else app_samples_file_system)
    self._samples_cache = compiled_fs_factory.Create(
        self._samples_fs,
        self._MakeSamplesList,
        SamplesDataSource,
        category=platform)
    self._text_cache = compiled_fs_factory.ForUnicode(self._samples_fs)
    self._reference_resolver = reference_resolver
    self._base_path = base_path
    self._platform = platform

  def GetCache(self):
    return self._samples_cache

  def FilterSamples(self, api_name):
    '''Fetches and filters the list of samples for this platform, returning
    a Future to the only the samples that use the API |api_name|.
    '''
    def filter_samples(samples_list):
      return [sample for sample in samples_list
          if any(call['name'].startswith(api_name + '.')
          for call in sample['api_calls'])]
    def handle_error(_):
      # TODO(rockot): This cache is probably not working as intended, since
      # it can still lead to underlying filesystem (e.g. gitiles) access
      # while processing live requests. Because this can fail, we at least
      # trap and log exceptions to prevent 500s from being thrown.
      logging.warning('Unable to get samples listing. Skipping.')
      return []
    platform_for_samples = '' if self._platform == 'apps' else EXAMPLES
    return (self._samples_cache.GetFromFileListing(platform_for_samples)
            .Then(filter_samples, error_handler=handle_error))

  def _GetDataFromManifest(self, path, file_system):
    manifest = self._text_cache.GetFromFile(path + '/manifest.json').Get()
    try:
      manifest_json = json.loads(json_comment_eater.Nom(manifest))
    except ValueError as e:
      logging.error('Error parsing manifest.json for %s: %s' % (path, e))
      return None
    l10n_data = {
      'name': manifest_json.get('name', ''),
      'description': manifest_json.get('description', None),
      'icon': manifest_json.get('icons', {}).get('128', None),
      'default_locale': manifest_json.get('default_locale', None),
      'locales': {}
    }
    if not l10n_data['default_locale']:
      return l10n_data
    locales_path = path + '/_locales/'
    locales_dir = file_system.ReadSingle(locales_path).Get()
    if locales_dir:
      def load_locale_json(path):
        return (path, json.loads(self._text_cache.GetFromFile(path).Get()))

      try:
        locales_json = [load_locale_json(locales_path + f + 'messages.json')
                        for f in locales_dir]
      except ValueError as e:
        logging.error('Error parsing locales files for %s: %s' % (path, e))
      else:
        for path, json_ in locales_json:
          l10n_data['locales'][path[len(locales_path):].split('/')[0]] = json_
    return l10n_data

  @Cache
  def _MakeSamplesList(self, base_path, files):
    samples_list = []
    for filename in sorted(files):
      if filename.rsplit('/')[-1] != 'manifest.json':
        continue

      # This is a little hacky, but it makes a sample page.
      sample_path = filename.rsplit('/', 1)[-2]
      sample_files = [path for path in files
                      if path.startswith(sample_path + '/')]
      js_files = [path for path in sample_files if path.endswith('.js')]
      js_contents = [self._text_cache.GetFromFile(
          posixpath.join(base_path, js_file)).Get()
          for js_file in js_files]
      api_items = set()
      for js in js_contents:
        api_items.update(_GetAPIItems(js))

      api_calls = []
      for item in sorted(api_items):
        if len(item.split('.')) < 3:
          continue
        if item.endswith('.removeListener') or item.endswith('.hasListener'):
          continue
        if item.endswith('.addListener'):
          item = item[:-len('.addListener')]
        if item.startswith('chrome.'):
          item = item[len('chrome.'):]
        ref_data = self._reference_resolver.GetLink(item)
        # TODO(kalman): What about references like chrome.storage.sync.get?
        # That should link to either chrome.storage.sync or
        # chrome.storage.StorageArea.get (or probably both).
        # TODO(kalman): Filter out API-only references? This can happen when
        # the API namespace is assigned to a variable, but it's very hard to
        # to disambiguate.
        if ref_data is None:
          continue
        api_calls.append({
          'name': ref_data['text'],
          'link': ref_data['href']
        })

      if self._platform == 'apps':
        url = url_constants.GITHUB_BASE + '/' + sample_path
        icon_base = url_constants.RAW_GITHUB_BASE + '/' + sample_path
        download_url = url
      else:
        extension_sample_path = posixpath.join('examples', sample_path)
        url = extension_sample_path
        icon_base = extension_sample_path
        download_url = extension_sample_path + '.zip'

      manifest_data = self._GetDataFromManifest(
          posixpath.join(base_path, sample_path),
          self._samples_fs)
      if manifest_data['icon'] is None:
        icon_path = posixpath.join(
            self._base_path, 'static', _DEFAULT_ICON_PATH)
      else:
        icon_path = '%s/%s' % (icon_base, manifest_data['icon'])
      manifest_data.update({
        'icon': icon_path,
        'download_url': download_url,
        'url': url,
        'files': [f.replace(sample_path + '/', '') for f in sample_files],
        'api_calls': api_calls
      })
      samples_list.append(manifest_data)

    return samples_list
