# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file defines performance test scenarios related to playing video and
# audio files using HTML5 APIs (as opposed to older tech like Flash and
# Silverlight). These scenarios simply exercise the Chromium code in particular
# ways. The metrics that are produced are calculated in a separate step.

from telemetry.page import page as page_module
from telemetry.page import traffic_setting as traffic_setting_module
from telemetry import story

# A complete list of page tags to check. This prevents misspellings and provides
# documentation of scenarios for code readers. These tags can be used to filter
# the list of pages to run using flags like --story-tag-filter=X.
_PAGE_TAGS_LIST = [
    # Audio codecs.
    'pcm',
    'mp3',
    'aac',
    'vorbis',
    'opus',
    # Video codecs.
    'h264',
    'vp8',
    'vp9',
    # Test types.
    'audio_video',
    'audio_only',
    'video_only',
    # Other filter tags.
    'is_50fps',
    'is_4k',
    # Play action.
    'seek',
    'beginning_to_end',
    'background',
    # Add javascript load.
    'busyjs',
    # Constrained network settings.
    'cns',
    # VideoStack API.
    'src',
    'mse'
]

# A list of traffic setting names to append to page names when used.
# Traffic settings is a way to constrain the network to match real-world
# scenarios.
_TRAFFIC_SETTING_NAMES = {
    traffic_setting_module.GPRS: 'GPRS',
    traffic_setting_module.REGULAR_2G: 'Regular-2G',
    traffic_setting_module.GOOD_2G: 'Good-2G',
    traffic_setting_module.REGULAR_3G: 'Regular-3G',
    traffic_setting_module.GOOD_3G: 'Good-3G',
    traffic_setting_module.REGULAR_4G: 'Regular-4G',
    traffic_setting_module.DSL: 'DSL',
    traffic_setting_module.WIFI: 'WiFi',
}

_URL_BASE = 'file://media_cases/'

#
# The following section contains base classes for pages.
#

class _MediaPage(page_module.Page):

  def __init__(self, url, page_set, tags, extra_browser_args=None,
               traffic_setting=traffic_setting_module.NONE):
    name = url.split('/')[-1]
    if traffic_setting != traffic_setting_module.NONE:
      name += '_' + _TRAFFIC_SETTING_NAMES[traffic_setting]
      tags.append('cns')
    if tags:
      for t in tags:
        assert t in _PAGE_TAGS_LIST
    assert not ('src' in tags and 'mse' in tags)
    super(_MediaPage, self).__init__(
        url=url, page_set=page_set, tags=tags, name=name,
        extra_browser_args=extra_browser_args,
        traffic_setting=traffic_setting)


class _BeginningToEndPlayPage(_MediaPage):
  """A normal play page simply plays the given media until the end."""

  def __init__(self, url, page_set, tags, extra_browser_args=None,
               traffic_setting=traffic_setting_module.NONE):
    tags.append('beginning_to_end')
    tags.append('src')
    super(_BeginningToEndPlayPage, self).__init__(
        url, page_set, tags, extra_browser_args,
        traffic_setting=traffic_setting)

  def RunPageInteractions(self, action_runner):
    # Play the media until it has finished or it times out.
    action_runner.PlayMedia(playing_event_timeout_in_seconds=60,
                            ended_event_timeout_in_seconds=60)
    # Generate memory dump for memoryMetric.
    if self.page_set.measure_memory:
      action_runner.MeasureMemory()


class _SeekPage(_MediaPage):
  """A seek page seeks twice in the video and measures the seek time."""

  def __init__(self, url, page_set, tags, extra_browser_args=None,
               action_timeout_in_seconds=60,
               traffic_setting=traffic_setting_module.NONE):
    tags.append('seek')
    tags.append('src')
    self._action_timeout = action_timeout_in_seconds
    super(_SeekPage, self).__init__(
        url, page_set, tags, extra_browser_args,
        traffic_setting=traffic_setting)

  def RunPageInteractions(self, action_runner):
    timeout = self._action_timeout
    # Start the media playback.
    action_runner.PlayMedia(
        playing_event_timeout_in_seconds=timeout)
    # Wait for 1 second so that we know the play-head is at ~1s.
    action_runner.Wait(1)
    # Seek to before the play-head location.
    action_runner.SeekMedia(seconds=0.5, timeout_in_seconds=timeout,
                            label='seek_warm')
    # Seek to after the play-head location.
    action_runner.SeekMedia(seconds=9, timeout_in_seconds=timeout,
                            label='seek_cold')
    # Generate memory dump for memoryMetric.
    if self.page_set.measure_memory:
      action_runner.MeasureMemory()


class _BackgroundPlaybackPage(_MediaPage):
  """A Background playback page plays the given media in a background tab.

  The motivation for this test case is crbug.com/678663.
  """

  def __init__(self, url, page_set, tags, extra_browser_args=None,
               background_time=10,
               traffic_setting=traffic_setting_module.NONE):
    self._background_time = background_time
    tags.append('background')
    tags.append('src')
    # disable-media-suspend is required since for Android background playback
    # gets suspended. This flag makes Android work the same way as desktop and
    # not turn off video playback in the background.
    extra_browser_args = extra_browser_args or []
    extra_browser_args.append('--disable-media-suspend')
    super(_BackgroundPlaybackPage, self).__init__(
        url, page_set, tags, extra_browser_args)

  def RunPageInteractions(self, action_runner):
    # Steps:
    # 1. Play a video
    # 2. Open new tab overtop to obscure the video
    # 3. Close the tab to go back to the tab that is playing the video.
    action_runner.PlayMedia(
        playing_event_timeout_in_seconds=60)
    action_runner.Wait(.5)
    new_tab = action_runner.tab.browser.tabs.New()
    new_tab.Activate()
    action_runner.Wait(self._background_time)
    new_tab.Close()
    action_runner.Wait(.5)
    # Generate memory dump for memoryMetric.
    if self.page_set.measure_memory:
      action_runner.MeasureMemory()


class _MSEPage(_MediaPage):

  def __init__(self, url, page_set, tags, extra_browser_args=None,
               number_of_runs=10):
    assert number_of_runs >= 1
    self._number_of_runs = number_of_runs
    tags.append('mse')
    super(_MSEPage, self).__init__(
        url, page_set, tags, extra_browser_args)

  def RunPageInteractions(self, action_runner):
    # The page automatically runs the test at load time.
    self._CheckTestResult(action_runner)
    # Now run it a few more times to get more reliable data.
    # Note that each run takes ~.5s, so running a bunch of times to get reliable
    # data is reasonable.
    for _ in range(self._number_of_runs - 1):
      url = action_runner.tab.url
      action_runner.tab.ClearCache(force=True)
      action_runner.tab.Navigate(url)
      self._CheckTestResult(action_runner)

  def _CheckTestResult(self, action_runner):
    action_runner.WaitForJavaScriptCondition('window.__testDone == true')
    test_failed = action_runner.EvaluateJavaScript('window.__testFailed')
    if test_failed:
      raise RuntimeError(action_runner.EvaluateJavaScript('window.__testError'))


class MediaCasesStorySet(story.StorySet):
  """
  Description: Video Stack Perf pages that report time_to_play, seek time and
  many other media-specific and generic metrics.
  """
  def __init__(self, measure_memory=False):
    super(MediaCasesStorySet, self).__init__(
            cloud_storage_bucket=story.PARTNER_BUCKET)

    self.measure_memory = measure_memory

    pages = [
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=crowd.ogg&type=audio',
            page_set=self,
            tags=['vorbis', 'audio_only']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=crowd1080.webm',
            page_set=self,
            tags=['is_50fps', 'vp8', 'vorbis', 'audio_video']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=tulip2.ogg&type=audio',
            page_set=self,
            tags=['vorbis', 'audio_only']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=tulip2.wav&type=audio',
            page_set=self,
            tags=['pcm', 'audio_only']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=crowd1080.mp4',
            page_set=self,
            tags=['is_50fps', 'h264', 'aac', 'audio_video']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=tulip2.mp3&type=audio',
            page_set=self,
            tags=['mp3', 'audio_only']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=tulip2.mp4',
            page_set=self,
            tags=['h264', 'aac', 'audio_video']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=tulip2.m4a&type=audio',
            page_set=self,
            tags=['aac', 'audio_only']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=garden2_10s.webm',
            page_set=self,
            tags=['is_4k', 'vp8', 'vorbis', 'audio_video']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=garden2_10s.mp4',
            page_set=self,
            tags=['is_4k', 'h264', 'aac', 'audio_video']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=tulip2.vp9.webm',
            page_set=self,
            tags=['vp9', 'opus', 'audio_video'],
            traffic_setting=traffic_setting_module.REGULAR_3G),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=tulip2.vp9.webm',
            page_set=self,
            tags=['vp9', 'opus', 'audio_video']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=crowd1080_vp9.webm',
            page_set=self,
            tags=['vp9', 'video_only']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=crowd720_vp9.webm',
            page_set=self,
            tags=['vp9', 'video_only']),
        _BeginningToEndPlayPage(
            url=_URL_BASE + 'video.html?src=tulip2.mp4&busyjs',
            page_set=self,
            tags=['h264', 'aac', 'audio_video', 'busyjs']),
        _SeekPage(
            url=_URL_BASE + 'video.html?src=tulip2.ogg&type=audio&seek',
            page_set=self,
            tags=['vorbis', 'audio_only']),
        _SeekPage(
            url=_URL_BASE + 'video.html?src=tulip2.wav&type=audio&seek',
            page_set=self,
            tags=['pcm', 'audio_only']),
        _SeekPage(
            url=_URL_BASE + 'video.html?src=tulip2.mp3&type=audio&seek',
            page_set=self,
            tags=['mp3', 'audio_only']),
        _SeekPage(
            url=_URL_BASE + 'video.html?src=tulip2.mp4&seek',
            page_set=self,
            tags=['h264', 'aac', 'audio_video']),
        _SeekPage(
            url=_URL_BASE + 'video.html?src=garden2_10s.webm&seek',
            page_set=self,
            tags=['is_4k', 'vp8', 'vorbis', 'audio_video']),
        _SeekPage(
            url=_URL_BASE + 'video.html?src=garden2_10s.mp4&seek',
            page_set=self,
            tags=['is_4k', 'h264', 'aac', 'audio_video']),
        _SeekPage(
            url=_URL_BASE + 'video.html?src=tulip2.vp9.webm&seek',
            page_set=self,
            tags=['vp9', 'opus', 'audio_video']),
        _SeekPage(
            url=_URL_BASE + 'video.html?src=crowd1080_vp9.webm&seek',
            page_set=self,
            tags=['vp9', 'video_only', 'seek']),
        _SeekPage(
            url=(_URL_BASE + 'video.html?src='
                 'smpte_3840x2160_60fps_vp9.webm&seek'),
            page_set=self,
            tags=['is_4k', 'vp9', 'video_only'],
            action_timeout_in_seconds=120),
        _BackgroundPlaybackPage(
            url=_URL_BASE + 'video.html?src=tulip2.vp9.webm&background',
            page_set=self,
            tags=['vp9', 'opus', 'audio_video']),
        _MSEPage(
            url=_URL_BASE + 'mse.html?media=aac_audio.mp4,h264_video.mp4',
            page_set=self,
            tags=['h264', 'aac', 'audio_video']),
        _MSEPage(
            url=(_URL_BASE + 'mse.html?'
                 'media=aac_audio.mp4,h264_video.mp4&waitForPageLoaded=true'),
            page_set=self,
            tags=['h264', 'aac', 'audio_video']),
        _MSEPage(
            url=_URL_BASE + 'mse.html?media=aac_audio.mp4',
            page_set=self,
            tags=['aac', 'audio_only']),
        _MSEPage(
            url=_URL_BASE + 'mse.html?media=h264_video.mp4',
            page_set=self,
            tags=['h264', 'video_only']),
    ]

    for page in pages:
      self.AddStory(page)
