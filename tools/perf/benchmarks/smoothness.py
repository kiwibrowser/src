# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from core import perf_benchmark

from benchmarks import silk_flags
from measurements import smoothness
import page_sets
import page_sets.key_silk_cases
from telemetry import benchmark
from telemetry import story as story_module


class _Smoothness(perf_benchmark.PerfBenchmark):
  """Base class for smoothness-based benchmarks."""

  test = smoothness.Smoothness

  @classmethod
  def Name(cls):
    return 'smoothness'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class SmoothnessTop25(_Smoothness):
  """Measures rendering statistics while scrolling down the top 25 web pages.

  http://www.chromium.org/developers/design-documents/rendering-benchmarks
  """

  @classmethod
  def Name(cls):
    return 'smoothness.top_25_smooth'

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, parser):
    parser.add_option('--scroll-forever', action='store_true',
                      help='If set, continuously scroll up and down forever. '
                           'This is useful for analysing scrolling behaviour '
                           'with tools such as perf.')

  def CreateStorySet(self, options):
    return page_sets.Top25SmoothPageSet(scroll_forever=options.scroll_forever)


@benchmark.Owner(emails=['senorblanco@chromium.org'])
class SmoothnessToughFiltersCases(_Smoothness):
  """Measures frame rate and a variety of other statistics.

  Uses a selection of pages making use of SVG and CSS Filter Effects.
  """
  page_set = page_sets.ToughFiltersCasesPageSet

  @classmethod
  def Name(cls):
    return 'smoothness.tough_filters_cases'


@benchmark.Owner(emails=['senorblanco@chromium.org'])
class SmoothnessToughPathRenderingCases(_Smoothness):
  """Tests a selection of pages with SVG and 2D Canvas paths.

  Measures frame rate and a variety of other statistics.  """
  page_set = page_sets.ToughPathRenderingCasesPageSet

  @classmethod
  def Name(cls):
    return 'smoothness.tough_path_rendering_cases'


@benchmark.Owner(emails=['junov@chromium.org'])
class SmoothnessToughCanvasCases(_Smoothness):
  """Measures frame rate and a variety of other statistics.

  Uses a selection of pages making use of the 2D Canvas API.
  """
  page_set = page_sets.ToughCanvasCasesPageSet

  """To add a new smoothness test on an experimental canvas feature, one should
  append extra browser arg of
  '--enable-blink-features=ComaSeparatedList,OfIndividualFeatures' here"""

  @classmethod
  def Name(cls):
    return 'smoothness.tough_canvas_cases'


@benchmark.Owner(emails=['kbr@chromium.org', 'zmo@chromium.org'])
class SmoothnessToughWebGLCases(_Smoothness):
  page_set = page_sets.ToughWebglCasesPageSet

  @classmethod
  def Name(cls):
    return 'smoothness.tough_webgl_cases'


@benchmark.Owner(emails=['kbr@chromium.org', 'zmo@chromium.org'])
class SmoothnessMaps(_Smoothness):
  page_set = page_sets.MapsPageSet

  @classmethod
  def Name(cls):
    return 'smoothness.maps'


@benchmark.Owner(emails=['ssid@chromium.org'])
class SmoothnessKeyDesktopMoveCases(_Smoothness):
  page_set = page_sets.KeyDesktopMoveCasesPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_DESKTOP]

  @classmethod
  def Name(cls):
    return 'smoothness.key_desktop_move_cases'


@benchmark.Owner(emails=['bokan@chromium.org', 'nzolghadr@chromium.org',
                         'vmiura@chromium.org'])
class SmoothnessKeyMobileSites(_Smoothness):
  """Measures rendering statistics while scrolling down the key mobile sites.

  http://www.chromium.org/developers/design-documents/rendering-benchmarks
  """
  page_set = page_sets.KeyMobileSitesSmoothPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'smoothness.key_mobile_sites_smooth'


@benchmark.Owner(emails=['alancutter@chromium.org'])
class SmoothnessToughAnimationCases(_Smoothness):
  page_set = page_sets.ToughAnimationCasesPageSet

  @classmethod
  def Name(cls):
    return 'smoothness.tough_animation_cases'


@benchmark.Owner(emails=['ajuma@chromium.org'])
class SmoothnessKeySilkCases(_Smoothness):
  """Measures rendering statistics for the key silk cases without GPU
  rasterization.
  """
  page_set = page_sets.KeySilkCasesPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'smoothness.key_silk_cases'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class SmoothnessGpuRasterizationTop25(_Smoothness):
  """Measures rendering statistics for the top 25 with GPU rasterization.
  """
  tag = 'gpu_rasterization'
  page_set = page_sets.Top25SmoothPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MOBILE,
                         story_module.expectations.ALL_CHROMEOS]

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForGpuRasterization(options)

  @classmethod
  def Name(cls):
    return 'smoothness.gpu_rasterization.top_25_smooth'


# Although GPU rasterization is enabled on Mac, it is blacklisted for certain
# path cases, so it is still valuable to run both the GPU and non-GPU versions
# of this benchmark on Mac.
@benchmark.Owner(emails=['senorblanco@chromium.org'])
class SmoothnessGpuRasterizationToughPathRenderingCases(_Smoothness):
  """Tests a selection of pages with SVG and 2D canvas paths with GPU
  rasterization.
  """
  tag = 'gpu_rasterization'
  page_set = page_sets.ToughPathRenderingCasesPageSet

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForGpuRasterization(options)

  @classmethod
  def Name(cls):
    return 'smoothness.gpu_rasterization.tough_path_rendering_cases'


@benchmark.Owner(emails=['senorblanco@chromium.org'])
class SmoothnessGpuRasterizationFiltersCases(_Smoothness):
  """Tests a selection of pages with SVG and CSS filter effects with GPU
  rasterization.
  """
  tag = 'gpu_rasterization'
  page_set = page_sets.ToughFiltersCasesPageSet

  # With GPU Raster enabled on Mac, there's no reason to run this
  # benchmark in addition to SmoothnessFiltersCases.
  SUPPORTED_PLATFORMS = [
      story_module.expectations.ALL_LINUX,
      story_module.expectations.ALL_MOBILE,
      story_module.expectations.ALL_WIN
  ]

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForGpuRasterization(options)

  @classmethod
  def Name(cls):
    return 'smoothness.gpu_rasterization.tough_filters_cases'


@benchmark.Owner(emails=['bokan@chromium.org', 'nzolghadr@chromium.org'])
class SmoothnessSyncScrollKeyMobileSites(_Smoothness):
  """Measures rendering statistics for the key mobile sites with synchronous
  (main thread) scrolling.
  """
  tag = 'sync_scroll'
  page_set = page_sets.KeyMobileSitesSmoothPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MOBILE]

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForSyncScrolling(options)

  @classmethod
  def Name(cls):
    return 'smoothness.sync_scroll.key_mobile_sites_smooth'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class SmoothnessSimpleMobilePages(_Smoothness):
  """Measures rendering statistics for simple mobile sites page set.
  """
  page_set = page_sets.SimpleMobileSitesPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'smoothness.simple_mobile_sites'


@benchmark.Owner(emails=['bokan@chromium.org'])
class SmoothnessToughPinchZoomCases(_Smoothness):
  """Measures rendering statistics for pinch-zooming in the tough pinch zoom
  cases.
  """
  page_set = page_sets.AndroidToughPinchZoomCasesPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'smoothness.tough_pinch_zoom_cases'


@benchmark.Owner(emails=['ericrk@chromium.org'])
class SmoothnessDesktopToughPinchZoomCases(_Smoothness):
  """Measures rendering statistics for pinch-zooming in the tough pinch zoom
  cases. Uses lower zoom levels customized for desktop limits.
  """
  page_set = page_sets.DesktopToughPinchZoomCasesPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MAC]

  @classmethod
  def Name(cls):
    return 'smoothness.desktop_tough_pinch_zoom_cases'


@benchmark.Owner(emails=['ericrk@chromium.org'])
class SmoothnessGpuRasterizationToughPinchZoomCases(_Smoothness):
  """Measures rendering statistics for pinch-zooming in the tough pinch zoom
  cases with GPU rasterization.
  """
  tag = 'gpu_rasterization'
  test = smoothness.Smoothness
  page_set = page_sets.AndroidToughPinchZoomCasesPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MOBILE]

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForGpuRasterization(options)

  @classmethod
  def Name(cls):
    return 'smoothness.gpu_rasterization.tough_pinch_zoom_cases'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class SmoothnessGpuRasterizationPolymer(_Smoothness):
  """Measures rendering statistics for the Polymer cases with GPU rasterization.
  """
  tag = 'gpu_rasterization'
  page_set = page_sets.PolymerPageSet

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForGpuRasterization(options)

  @classmethod
  def Name(cls):
    return 'smoothness.gpu_rasterization.polymer'


@benchmark.Owner(emails=['reveman@chromium.org'])
class SmoothnessToughScrollingCases(_Smoothness):
  page_set = page_sets.ToughScrollingCasesPageSet

  @classmethod
  def ShouldAddValue(cls, name, from_first_story_run):
    del from_first_story_run  # unused
    # Only keep 'mean_pixels_approximated' and 'mean_pixels_checkerboarded'
    # metrics. (crbug.com/529331)
    return name in ('mean_pixels_approximated',
                    'mean_pixels_checkerboarded')

  @classmethod
  def Name(cls):
    return 'smoothness.tough_scrolling_cases'


@benchmark.Owner(emails=['ericrk@chromium.org'])
class SmoothnessGpuRasterizationToughScrollingCases(_Smoothness):
  tag = 'gpu_rasterization'
  test = smoothness.Smoothness
  page_set = page_sets.ToughScrollingCasesPageSet

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForGpuRasterization(options)

  @classmethod
  def Name(cls):
    return 'smoothness.gpu_rasterization.tough_scrolling_cases'


@benchmark.Owner(emails=['vmiura@chromium.org', 'sadrul@chromium.org'])
class SmoothnessToughImageDecodeCases(_Smoothness):
  page_set = page_sets.ToughImageDecodeCasesPageSet

  @classmethod
  def Name(cls):
    return 'smoothness.tough_image_decode_cases'


@benchmark.Owner(emails=['cblume@chromium.org'])
class SmoothnessImageDecodingCases(_Smoothness):
  """Measures decoding statistics for jpeg images.
  """
  page_set = page_sets.ImageDecodingCasesPageSet

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForGpuRasterization(options)
    options.AppendExtraBrowserArgs('--disable-accelerated-jpeg-decoding')

  @classmethod
  def Name(cls):
    return 'smoothness.image_decoding_cases'


@benchmark.Owner(emails=['cblume@chromium.org'])
class SmoothnessGpuImageDecodingCases(_Smoothness):
  """Measures decoding statistics for jpeg images with GPU rasterization.
  """
  tag = 'gpu_rasterization_and_decoding'
  page_set = page_sets.ImageDecodingCasesPageSet

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForGpuRasterization(options)
    # TODO(sugoi): Remove the following line once M41 goes stable
    options.AppendExtraBrowserArgs('--enable-accelerated-jpeg-decoding')

  @classmethod
  def Name(cls):
    return 'smoothness.gpu_rasterization_and_decoding.image_decoding_cases'


@benchmark.Owner(emails=['picksi@chromium.org'])
class SmoothnessPathologicalMobileSites(_Smoothness):
  """Measures task execution statistics while scrolling pathological sites.
  """
  page_set = page_sets.PathologicalMobileSitesPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'smoothness.pathological_mobile_sites'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class SmoothnessToughTextureUploadCases(_Smoothness):
  page_set = page_sets.ToughTextureUploadCasesPageSet

  @classmethod
  def Name(cls):
    return 'smoothness.tough_texture_upload_cases'


@benchmark.Owner(emails=['skyostil@chromium.org'])
class SmoothnessToughAdCases(_Smoothness):
  """Measures rendering statistics while displaying advertisements."""
  page_set = page_sets.SyntheticToughAdCasesPageSet

  @classmethod
  def Name(cls):
    return 'smoothness.tough_ad_cases'

  @classmethod
  def ShouldAddValue(cls, name, from_first_story_run):
    del from_first_story_run  # unused
    # These pages don't scroll so it's not necessary to measure input latency.
    return name != 'first_gesture_scroll_update_latency'


@benchmark.Owner(emails=['skyostil@chromium.org'])
class SmoothnessToughWebGLAdCases(_Smoothness):
  """Measures rendering statistics while scrolling advertisements."""
  page_set = page_sets.SyntheticToughWebglAdCasesPageSet

  @classmethod
  def Name(cls):
    return 'smoothness.tough_webgl_ad_cases'


@benchmark.Owner(emails=['skyostil@chromium.org', 'brianderson@chromium.org'])
class SmoothnessToughSchedulingCases(_Smoothness):
  """Measures rendering statistics while interacting with pages that have
  challenging scheduling properties.

  https://docs.google.com/a/chromium.org/document/d/
      17yhE5Po9By0sCdM1yZT3LiUECaUr_94rQt9j-4tOQIM/view"""
  page_set = page_sets.ToughSchedulingCasesPageSet

  @classmethod
  def Name(cls):
    # This should be smoothness.tough_scheduling_cases but since the benchmark
    # has been named this way for long time, we keep the name as-is to avoid
    # data migration.
    return 'scheduler.tough_scheduling_cases'
