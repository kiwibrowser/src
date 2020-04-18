# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import re

from telemetry.internal.util import command_line


class _StoryMatcher(object):
  def __init__(self, pattern):
    self._regex = None
    self.has_compile_error = False
    if pattern:
      try:
        self._regex = re.compile(pattern)
      except re.error:
        self.has_compile_error = True

  def __nonzero__(self):
    return self._regex is not None

  def HasMatch(self, story):
    return self and bool(self._regex.search(story.name))


class _StoryTagMatcher(object):
  def __init__(self, tags_str):
    self._tags = tags_str.split(',') if tags_str else None

  def __nonzero__(self):
    return self._tags is not None

  def HasLabelIn(self, story):
    return self and bool(story.tags.intersection(self._tags))


class StoryFilter(command_line.ArgumentHandlerMixIn):
  """Filters stories in the story set based on command-line flags."""

  @classmethod
  def AddCommandLineArgs(cls, parser):
    group = optparse.OptionGroup(parser, 'User story filtering options')
    group.add_option(
        '--story-filter',
        help='Use only stories whose names match the given filter regexp.')
    group.add_option(
        '--story-filter-exclude',
        help='Exclude stories whose names match the given filter regexp.')
    group.add_option(
        '--story-tag-filter',
        help='Use only stories that have any of these tags')
    group.add_option(
        '--story-tag-filter-exclude',
        help='Exclude stories that have any of these tags')
    common_story_shard_help = (
        'Indices start at 0, and have the same rules as python slices,'
        ' e.g.  [4, 5, 6, 7, 8][0:3] -> [4, 5, 6])')
    group.add_option(
        '--experimental-story-shard-begin-index', type='int',
        help='EXPERIMENTAL. Beginning index of set of stories to run. ' +
        common_story_shard_help)
    group.add_option(
        '--experimental-story-shard-end-index', type='int',
        help='EXPERIMENTAL. End index of set of stories to run. Value will be'
             ' rounded down to the number of stories. Negative values not'
             ' allowed. ' + common_story_shard_help)
    parser.add_option_group(group)

  @classmethod
  def ProcessCommandLineArgs(cls, parser, args):
    cls._include_regex = _StoryMatcher(args.story_filter)
    cls._exclude_regex = _StoryMatcher(args.story_filter_exclude)

    cls._include_tags = _StoryTagMatcher(args.story_tag_filter)
    cls._exclude_tags = _StoryTagMatcher(args.story_tag_filter_exclude)

    cls._begin_index = args.experimental_story_shard_begin_index or 0
    cls._end_index = args.experimental_story_shard_end_index

    if cls._end_index is not None:
      if cls._end_index < 0:
        raise parser.error(
            '--experimental-story-shard-end-index cannot be less than 0')
      if cls._begin_index is not None and cls._end_index <= cls._begin_index:
        raise parser.error(
            '--experimental-story-shard-end-index cannot be less than'
            ' or equal to --experimental-story-shard-begin-index')


    if cls._include_regex.has_compile_error:
      raise parser.error('--story-filter: Invalid regex.')
    if cls._exclude_regex.has_compile_error:
      raise parser.error('--story-filter-exclude: Invalid regex.')

  @classmethod
  def FilterStorySet(cls, story_set):
    """Filters the given story set, using filters provided in the command line.

    Story sharding is done before exclusion and inclusion is done.
    """
    if cls._begin_index < 0:
      cls._begin_index = 0
    if cls._end_index is None:
      cls._end_index = len(story_set)

    story_set = story_set[cls._begin_index:cls._end_index]

    final_story_set = []
    for story in story_set:
      # Exclude filters take priority.
      if cls._exclude_tags.HasLabelIn(story):
        continue
      if cls._exclude_regex.HasMatch(story):
        continue

      if cls._include_tags and not cls._include_tags.HasLabelIn(story):
        continue
      if cls._include_regex and not cls._include_regex.HasMatch(story):
        continue

      final_story_set.append(story)

    return final_story_set
