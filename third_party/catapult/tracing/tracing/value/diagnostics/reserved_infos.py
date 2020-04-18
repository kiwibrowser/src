# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

class _Info(object):

  def __init__(self, name, _type=None, entry_type=None):
    self._name = name
    self._type = _type
    if entry_type is not None and self._type != 'GenericSet':
      raise ValueError(
          'entry_type should only be specified if _type is GenericSet')
    self._entry_type = entry_type

  @property
  def name(self):
    return self._name

  @property
  def type(self):
    return self._type

  @property
  def entry_type(self):
    return self._entry_type


ANGLE_REVISIONS = _Info('angleRevisions', 'GenericSet', str)
ARCHITECTURES = _Info('architectures', 'GenericSet', str)
BENCHMARKS = _Info('benchmarks', 'GenericSet', str)
BENCHMARK_START = _Info('benchmarkStart', 'DateRange')
BENCHMARK_DESCRIPTIONS = _Info('benchmarkDescriptions', 'GenericSet', str)
BOTS = _Info('bots', 'GenericSet', str)
BUG_COMPONENTS = _Info('bugComponents', 'GenericSet', str)
BUILDS = _Info('builds', 'GenericSet', int)
CATAPULT_REVISIONS = _Info('catapultRevisions', 'GenericSet', str)
CHROMIUM_COMMIT_POSITIONS = _Info('chromiumCommitPositions', 'GenericSet', int)
CHROMIUM_REVISIONS = _Info('chromiumRevisions', 'GenericSet', str)
DEVICE_IDS = _Info('deviceIds', 'GenericSet', str)
FUCHSIA_GARNET_REVISIONS = _Info('fuchsiaGarnetRevisions', 'GenericSet', str)
FUCHSIA_PERIDOT_REVISIONS = _Info('fuchsiaPeridotRevisions', 'GenericSet', str)
FUCHSIA_TOPAZ_REVISIONS = _Info('fuchsiaTopazRevisions', 'GenericSet', str)
FUCHSIA_ZIRCON_REVISIONS = _Info('fuchsiaZirconRevisions', 'GenericSet', str)
GPUS = _Info('gpus', 'GenericSet', str)
GROUPING_PATH = _Info('groupingPath')
HAD_FAILURES = _Info('hadFailures', 'GenericSet', bool)
IS_REFERENCE_BUILD = _Info('isReferenceBuild', 'GenericSet', bool)
LABELS = _Info('labels', 'GenericSet', str)
LOG_URLS = _Info('logUrls', 'GenericSet', str)
MASTERS = _Info('masters', 'GenericSet', str)
MEMORY_AMOUNTS = _Info('memoryAmounts', 'GenericSet', int)
MERGED_FROM = _Info('mergedFrom', 'RelatedHistogramMap')
MERGED_TO = _Info('mergedTo', 'RelatedHistogramMap')
OS_NAMES = _Info('osNames', 'GenericSet', str)
OS_VERSIONS = _Info('osVersions', 'GenericSet', str)
OWNERS = _Info('owners', 'GenericSet', str)
PRODUCT_VERSIONS = _Info('productVersions', 'GenericSet', str)
RELATED_NAMES = _Info('relatedNames', 'GenericSet', str)
SKIA_REVISIONS = _Info('skiaRevisions', 'GenericSet', str)
STORIES = _Info('stories', 'GenericSet', str)
STORYSET_REPEATS = _Info('storysetRepeats', 'GenericSet', int)
STORY_TAGS = _Info('storyTags', 'GenericSet', str)
SUMMARY_KEYS = _Info('summaryKeys', 'GenericSet', str)
TAG_MAP = _Info('tagmap', 'TagMap')
TEST_PATH = _Info('testPath', 'GenericSet', str)
TRACE_START = _Info('traceStart', 'DateRange')
TRACE_URLS = _Info('traceUrls', 'GenericSet', str)
V8_COMMIT_POSITIONS = _Info('v8CommitPositions', 'DateRange')
V8_REVISIONS = _Info('v8Revisions', 'GenericSet', str)
WEBRTC_REVISIONS = _Info('webrtcRevisions', 'GenericSet', str)


def _CreateCachedInfoTypes():
  info_types = {}
  for info in globals().itervalues():
    if isinstance(info, _Info):
      info_types[info.name] = info
  return info_types

_CACHED_INFO_TYPES = _CreateCachedInfoTypes()

def GetTypeForName(name):
  info = _CACHED_INFO_TYPES.get(name)
  if info:
    return info.type

def AllInfos():
  for info in _CACHED_INFO_TYPES.itervalues():
    yield info

def AllNames():
  for info in AllInfos():
    yield info.name
