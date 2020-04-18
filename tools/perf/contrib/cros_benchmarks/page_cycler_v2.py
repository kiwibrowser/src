# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The page cycler v2.

For details, see design doc:
https://docs.google.com/document/d/1EZQX-x3eEphXupiX-Hq7T4Afju5_sIdxPWYetj7ynd0
"""

from core import perf_benchmark
import page_sets

from benchmarks import loading_metrics_category
from telemetry import benchmark
from telemetry.page import cache_temperature
from telemetry.web_perf import timeline_based_measurement

class _PageCyclerV2(perf_benchmark.PerfBenchmark):
  options = {'pageset_repeat': 2}

  def CreateCoreTimelineBasedMeasurementOptions(self):
    tbm_options = timeline_based_measurement.Options()
    loading_metrics_category.AugmentOptionsForLoadingMetrics(tbm_options)
    return tbm_options


@benchmark.Owner(emails=['kouhei@chromium.org', 'ksakamoto@chromium.org'])
class PageCyclerV2Typical25(_PageCyclerV2):
  """Page load time benchmark for a 25 typical web pages.

  Designed to represent typical, not highly optimized or highly popular web
  sites. Runs against pages recorded in June, 2014.
  """

  @classmethod
  def Name(cls):
    return 'page_cycler_v2.typical_25'

  def CreateStorySet(self, options):
    return page_sets.Typical25PageSet(run_no_page_interactions=True,
        cache_temperatures=[
          cache_temperature.COLD, cache_temperature.WARM])


@benchmark.Owner(emails=['kouhei@chromium.org', 'ksakamoto@chromium.org'])
class PageCyclerV2IntlArFaHe(_PageCyclerV2):
  """Page load time for a variety of pages in Arabic, Farsi and Hebrew.

  Runs against pages recorded in April, 2013.
  """
  page_set = page_sets.IntlArFaHePageSet

  @classmethod
  def Name(cls):
    return 'page_cycler_v2.intl_ar_fa_he'

  def CreateStorySet(self, options):
    return page_sets.IntlArFaHePageSet(cache_temperatures=[
          cache_temperature.COLD, cache_temperature.WARM])


@benchmark.Owner(emails=['kouhei@chromium.org', 'ksakamoto@chromium.org'])
class PageCyclerV2IntlEsFrPtBr(_PageCyclerV2):
  """Page load time for a pages in Spanish, French and Brazilian Portuguese.

  Runs against pages recorded in April, 2013.
  """
  page_set = page_sets.IntlEsFrPtBrPageSet

  @classmethod
  def Name(cls):
    return 'page_cycler_v2.intl_es_fr_pt-BR'

  def CreateStorySet(self, options):
    return page_sets.IntlEsFrPtBrPageSet(cache_temperatures=[
          cache_temperature.COLD, cache_temperature.WARM])


@benchmark.Owner(emails=['kouhei@chromium.org', 'ksakamoto@chromium.org'])
class PageCyclerV2IntlHiRu(_PageCyclerV2):
  """Page load time benchmark for a variety of pages in Hindi and Russian.

  Runs against pages recorded in April, 2013.
  """
  page_set = page_sets.IntlHiRuPageSet

  @classmethod
  def Name(cls):
    return 'page_cycler_v2.intl_hi_ru'

  def CreateStorySet(self, options):
    return page_sets.IntlHiRuPageSet(cache_temperatures=[
          cache_temperature.COLD, cache_temperature.WARM])


@benchmark.Owner(emails=['kouhei@chromium.org', 'ksakamoto@chromium.org'])
class PageCyclerV2IntlJaZh(_PageCyclerV2):
  """Page load time benchmark for a variety of pages in Japanese and Chinese.

  Runs against pages recorded in April, 2013.
  """

  @classmethod
  def Name(cls):
    return 'page_cycler_v2.intl_ja_zh'

  def CreateStorySet(self, options):
    return page_sets.IntlJaZhPageSet(cache_temperatures=[
          cache_temperature.COLD, cache_temperature.WARM])


@benchmark.Owner(emails=['kouhei@chromium.org', 'ksakamoto@chromium.org'])
class PageCyclerV2IntlKoThVi(_PageCyclerV2):
  """Page load time for a variety of pages in Korean, Thai and Vietnamese.

  Runs against pages recorded in April, 2013.
  """
  page_set = page_sets.IntlKoThViPageSet

  @classmethod
  def Name(cls):
    return 'page_cycler_v2.intl_ko_th_vi'

  def CreateStorySet(self, options):
    return page_sets.IntlKoThViPageSet(cache_temperatures=[
          cache_temperature.COLD, cache_temperature.WARM])
