# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from path_util import AssertIsValid


_EXTENSION_TYPES = {'extensions': 'extension', 'apps': 'platform_app'}


def GetPlatforms():
  return ('apps', 'extensions')


def GetExtensionTypes():
  return ('platform_app', 'extension')


def ExtractPlatformFromURL(url):
  '''Returns 'apps' or 'extensions' depending on the URL.
  '''
  AssertIsValid(url)
  platform = url.split('/', 1)[0]
  if platform not in GetPlatforms():
    return None
  return platform


def PluralToSingular(platform):
  '''Converts 'apps' to 'app' and 'extensions' to 'extension'.
  '''
  assert platform in GetPlatforms(), platform
  return platform[:-1]


def PlatformToExtensionType(platform):
  assert platform in GetPlatforms(), platform
  return _EXTENSION_TYPES[platform]
