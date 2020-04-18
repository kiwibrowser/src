# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_benchmark import ChromeProxyBenchmark
from integration_tests import chrome_proxy_measurements as measurements
from integration_tests import chrome_proxy_pagesets as pagesets
from telemetry import benchmark
from telemetry import decorators

DESKTOP_PLATFORMS = ['mac', 'linux', 'win', 'chromeos']
WEBVIEW_PLATFORMS = ['android-webview', 'android-webview-instrumentation']


class ChromeProxyBypassOnTimeout(ChromeProxyBenchmark):
  """Check that the proxy bypasses when origin times out.

  If the origin site does not make an HTTP response in a reasonable
  amount of time, the proxy should bypass.
  """
  tag = 'timeout_bypass'
  test = measurements.ChromeProxyBypassOnTimeout
  page_set = pagesets.BypassOnTimeoutStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.timeout_bypass.timeout_bypass'

class ChromeProxyBadHTTPSFallback(ChromeProxyBenchmark):
  """Check that the client falls back to HTTP on bad HTTPS response.

  If the HTTPS proxy responds with a bad response code (like 500) then the
  client should fallback to HTTP.
  """
  tag = 'badhttps_bypass'
  test = measurements.ChromeProxyBadHTTPSFallback
  page_set = pagesets.SyntheticStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.badhttps_fallback.badhttps_fallback'

class ChromeProxyClientType(ChromeProxyBenchmark):
  tag = 'client_type'
  test = measurements.ChromeProxyClientType
  page_set = pagesets.ClientTypeStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.client_type.client_type'


@decorators.Disabled(*WEBVIEW_PLATFORMS)
class ChromeProxyLoFi(ChromeProxyBenchmark):
  tag = 'lo_fi'
  test = measurements.ChromeProxyLoFi
  page_set = pagesets.LoFiStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.lo_fi.lo_fi'


@decorators.Disabled(*WEBVIEW_PLATFORMS)
class ChromeProxyCacheLoFiDisabled(ChromeProxyBenchmark):
  tag = 'cache_lo_fi_disabled'
  test = measurements.ChromeProxyCacheLoFiDisabled
  page_set = pagesets.LoFiCacheStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.lo_fi.cache_lo_fi_disabled'


@decorators.Disabled(*WEBVIEW_PLATFORMS)
class ChromeProxyCacheProxyDisabled(ChromeProxyBenchmark):
  tag = 'cache_proxy_disabled'
  test = measurements.ChromeProxyCacheProxyDisabled
  page_set = pagesets.LoFiCacheStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.lo_fi.cache_proxy_disabled'


@decorators.Disabled(*WEBVIEW_PLATFORMS)
class ChromeProxyLitePage(ChromeProxyBenchmark):
  tag = 'lite_page'
  test = measurements.ChromeProxyLitePage
  page_set = pagesets.LitePageStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.lite_page.lite_page'


class ChromeProxyExpDirective(ChromeProxyBenchmark):
  tag = 'exp_directive'
  test = measurements.ChromeProxyExpDirective
  page_set = pagesets.ExpDirectiveStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.exp_directive.exp_directive'


class ChromeProxyPassThrough(ChromeProxyBenchmark):
  tag = 'pass_through'
  test = measurements.ChromeProxyPassThrough
  page_set = pagesets.PassThroughStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.pass_through.pass_through'


class ChromeProxyBypass(ChromeProxyBenchmark):
  tag = 'bypass'
  test = measurements.ChromeProxyBypass
  page_set = pagesets.BypassStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.bypass.bypass'


class ChromeProxyHTTPSBypass(ChromeProxyBenchmark):
  tag = 'https_bypass'
  test = measurements.ChromeProxyHTTPSBypass
  page_set = pagesets.HTTPSBypassStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.https_bypass.https_bypass'


class ChromeProxyHTML5Test(ChromeProxyBenchmark):
  tag = 'html5test'
  test = measurements.ChromeProxyHTML5Test
  page_set = pagesets.HTML5TestStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.html5test.html5test'


@decorators.Enabled(*DESKTOP_PLATFORMS)
class ChromeProxyYouTube(ChromeProxyBenchmark):
  tag = 'youtube'
  test = measurements.ChromeProxyYouTube
  page_set = pagesets.YouTubeStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.youtube.youtube'


class ChromeProxyCorsBypass(ChromeProxyBenchmark):
  tag = 'bypass'
  test = measurements.ChromeProxyCorsBypass
  page_set = pagesets.CorsBypassStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.bypass.corsbypass'


class ChromeProxyBlockOnce(ChromeProxyBenchmark):
  tag = 'block_once'
  test = measurements.ChromeProxyBlockOnce
  page_set = pagesets.BlockOnceStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.block_once.block_once'


@decorators.Disabled(*(DESKTOP_PLATFORMS + WEBVIEW_PLATFORMS))
# Safebrowsing is enabled for Android and iOS.
class ChromeProxySafeBrowsingOn(ChromeProxyBenchmark):
  tag = 'safebrowsing_on'
  test = measurements.ChromeProxySafebrowsingOn

  # Override CreateStorySet so that we can instantiate SafebrowsingStorySet
  # with a non default param.
  def CreateStorySet(self, options):
    del options  # unused
    return pagesets.SafebrowsingStorySet(expect_timeout=True)

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.safebrowsing_on.safebrowsing'


@decorators.Enabled(*(DESKTOP_PLATFORMS + WEBVIEW_PLATFORMS))
# Safebrowsing is switched off for Android Webview and all desktop platforms.
class ChromeProxySafeBrowsingOff(ChromeProxyBenchmark):
  tag = 'safebrowsing_off'
  test = measurements.ChromeProxySafebrowsingOff
  page_set = pagesets.SafebrowsingStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.safebrowsing_off.safebrowsing'


class ChromeProxyHTTPFallbackProbeURL(ChromeProxyBenchmark):
  tag = 'fallback_probe'
  test = measurements.ChromeProxyHTTPFallbackProbeURL
  page_set = pagesets.SyntheticStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.fallback_probe.synthetic'


class ChromeProxyHTTPFallbackViaHeader(ChromeProxyBenchmark):
  tag = 'fallback_viaheader'
  test = measurements.ChromeProxyHTTPFallbackViaHeader
  page_set = pagesets.FallbackViaHeaderStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.fallback_viaheader.fallback_viaheader'


class ChromeProxyHTTPToDirectFallback(ChromeProxyBenchmark):
  tag = 'http_to_direct_fallback'
  test = measurements.ChromeProxyHTTPToDirectFallback
  page_set = pagesets.HTTPToDirectFallbackStorySet

  @classmethod
  def Name(cls):
    return ('chrome_proxy_benchmark.http_to_direct_fallback.'
            'http_to_direct_fallback')


class ChromeProxyReenableAfterBypass(ChromeProxyBenchmark):
  tag = 'reenable_after_bypass'
  test = measurements.ChromeProxyReenableAfterBypass
  page_set = pagesets.ReenableAfterBypassStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.reenable_after_bypass.reenable_after_bypass'


class ChromeProxyReenableAfterSetBypass(ChromeProxyBenchmark):
  tag = 'reenable_after_set_bypass'
  test = measurements.ChromeProxyReenableAfterSetBypass
  page_set = pagesets.ReenableAfterSetBypassStorySet

  @classmethod
  def Name(cls):
    return ('chrome_proxy_benchmark.reenable_after_set_bypass' +
            '.reenable_after_set_bypass')


class ChromeProxySmoke(ChromeProxyBenchmark):
  tag = 'smoke'
  test = measurements.ChromeProxySmoke
  page_set = pagesets.SmokeStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.smoke.smoke'

class ChromeProxyQuicSmoke(ChromeProxyBenchmark):
  tag = 'smoke'
  test = measurements.ChromeProxyQuicSmoke
  page_set = pagesets.SmokeStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.quic.smoke'

class ChromeProxyClientConfig(ChromeProxyBenchmark):
  tag = 'client_config'
  test = measurements.ChromeProxyClientConfig
  page_set = pagesets.SyntheticStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.client_config.synthetic'


@decorators.Enabled(*DESKTOP_PLATFORMS)
class ChromeProxyVideoDirect(benchmark.Benchmark):
  tag = 'video'
  test = measurements.ChromeProxyVideoValidation
  page_set = pagesets.VideoDirectStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.video.direct'


@decorators.Enabled(*DESKTOP_PLATFORMS)
class ChromeProxyVideoProxied(benchmark.Benchmark):
  tag = 'video'
  test = measurements.ChromeProxyVideoValidation
  page_set = pagesets.VideoProxiedStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.video.proxied'


@decorators.Enabled(*DESKTOP_PLATFORMS)
class ChromeProxyVideoCompare(benchmark.Benchmark):
  """Comparison of direct and proxied video fetches.

  This benchmark runs the ChromeProxyVideoDirect and ChromeProxyVideoProxied
  benchmarks, then compares their results.
  """

  tag = 'video'
  test = measurements.ChromeProxyVideoValidation
  page_set = pagesets.VideoCompareStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.video.compare'

@decorators.Enabled(*DESKTOP_PLATFORMS)
class ChromeProxyVideoFrames(benchmark.Benchmark):
  """Check for video frames similar to original video."""

  tag = 'video'
  test = measurements.ChromeProxyInstrumentedVideoValidation
  page_set = pagesets.VideoFrameStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.video.frames'

@decorators.Enabled(*DESKTOP_PLATFORMS)
class ChromeProxyVideoAudio(benchmark.Benchmark):
  """Check that audio is similar to original video."""

  tag = 'video'
  test = measurements.ChromeProxyInstrumentedVideoValidation
  page_set = pagesets.VideoAudioStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.video.audio'

class ChromeProxyPingback(ChromeProxyBenchmark):
  """Check that the pingback is sent and the server responds. """
  tag = 'pingback'
  test = measurements.ChromeProxyPingback
  page_set = pagesets.PingbackStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.pingback'

class ChromeProxyQuicTransaction(ChromeProxyBenchmark):
  """Check that Chrome uses QUIC correctly when connecting to a proxy
  that supports QUIC. """
  tag = 'quic-proxy'
  test = measurements.ChromeProxyQuicTransaction
  page_set = pagesets.QuicStorySet

  @classmethod
  def Name(cls):
    return 'chrome_proxy_benchmark.quic.transaction'
