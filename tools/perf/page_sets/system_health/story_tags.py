# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections


Tag = collections.namedtuple('Tag', ['name', 'description'])


# Below are tags that describe various aspect of system health stories.
# A story can have multiple tags. All the tags should be noun.

ACCESSIBILITY = Tag(
    'accessibility', 'Story tests performance when accessibility is enabled.')
AUDIO_PLAYBACK = Tag(
    'audio_playback', 'Story has audio playing.')
CANVAS_ANIMATION = Tag(
    'canvas_animation', 'Story has animations that are implemented using '
    'html5 canvas.')
CSS_ANIMATION = Tag(
    'css_animation', 'Story has animations that are implemented using CSS.')
EXTENSION = Tag(
    'extension', 'Story has browser with extension installed.')
IMAGES = Tag(
    'images', 'Story has sites with heavy uses of images.')
INFINITE_SCROLL = Tag('infinite_scroll', 'Story has infinite scroll action.')
INTERNATIONAL = Tag(
    'international', 'Story has navigations to websites with content in non '
    'English languages.')
EMERGING_MARKET = Tag(
    'emerging_market', 'Story has significant usage in emerging markets with '
    'low-end mobile devices and slow network connections.')
JAVASCRIPT_HEAVY = Tag(
    'javascript_heavy', 'Story has navigations to websites with heavy usages '
    'of JavaScript. The story uses 20Mb+ memory for javascript and local '
    'run with "v8" category enabled also shows the trace has js slices across '
    'the whole run.')
KEYBOARD_INPUT = Tag(
    'keyboard_input', 'Story does keyboard input.')
SCROLL = Tag(
    'scroll', 'Story has scroll gestures & scroll animation.')
PINCH_ZOOM = Tag(
    'pinch_zoom', 'Story has pinch zoom gestures & pinch zoom animation.')
TABS_SWITCHING = Tag(
    'tabs_switching', 'Story has multi tabs and tabs switching action.')
VIDEO_PLAYBACK = Tag(
    'video_playback', 'Story has video playing.')
WEBGL = Tag(
    'webgl', 'Story has sites with heavy uses of WebGL.')
WEB_STORAGE = Tag(
    'web_storage', 'Story has sites with heavy uses of Web storage.')


def _ExtractAllTags():
  all_tag_names = set()
  all_tags = []
  # Collect all the tags defined in this module. Also assert that there is no
  # duplicate tag names.
  for obj in globals().values():
    if isinstance(obj, Tag):
      all_tags.append(obj)
      assert obj.name not in all_tag_names, 'Duplicate tag name: %s' % obj.name
      all_tag_names.add(obj.name)
  return all_tags


ALL_TAGS = _ExtractAllTags()
