# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from telemetry.testing import serially_executed_browser_test_case

from py_utils import cloud_storage

_WPR_ARCHIVE_PATH = os.path.join(os.path.dirname(__file__),
                                 'popular_urls_000.wpr')
_POPULAR_URLS = [
    'http://www.google.com/',
    'http://www.facebook.com/',
    'http://www.youtube.com/',
    'http://www.yahoo.com/',
    'http://www.baidu.com/',
    'http://www.wikipedia.org/',
    'http://www.live.com/',
    'http://www.twitter.com/',
    'http://www.qq.com/',
    'http://www.amazon.com/',
    'http://www.blogspot.com/',
    'http://www.linkedin.com/',
    'http://www.google.co.in/',
    'http://www.taobao.com/',
    'http://www.sina.com.cn/',
    'http://www.yahoo.co.jp/',
    'http://www.msn.com/',
    'http://www.wordpress.com/',
    'http://www.t.co/',
    'http://www.google.de/',
    'http://www.google.com.hk/',
    'http://www.ebay.com/',
    'http://www.googleusercontent.com/',
    'http://www.google.co.uk/',
    'http://www.google.co.jp/',
    'http://www.yandex.ru/',
    'http://www.163.com/',
    'http://www.google.fr/',
    'http://www.weibo.com/',
    'http://www.bing.com/',
    'http://www.microsoft.com/',
    'http://www.google.com.br/',
    'http://www.mail.ru/',
    'http://www.tumblr.com/',
    'http://www.pinterest.com/',
    'http://www.paypal.com/',
    'http://www.apple.com/',
    'http://www.google.it/',
    'http://www.blogger.com/',
    'http://www.babylon.com/',
    'http://www.google.es/',
    'http://www.google.ru/',
    'http://www.craigslist.org/',
    'http://www.sohu.com/',
    'http://www.imdb.com/',
    'http://www.flickr.com/',
    'http://www.bbc.co.uk/',
    'http://www.xvideos.com/',
    'http://www.fc2.com/',
    'http://www.go.com/',
    'http://www.xhamster.com/',
    'http://www.ask.com/',
    'http://www.ifeng.com/',
    'http://www.google.com.mx/',
    'http://www.youku.com/',
    'http://www.google.ca/',
    'http://www.livejasmin.com/',
    'http://www.tmall.com/',
    'http://www.zedo.com/',
    'http://www.imgur.com/',
    'http://www.conduit.com/',
    'http://www.odnoklassniki.ru/',
    'http://www.cnn.com/',
    'http://www.adobe.com/',
    'http://www.google.co.id/',
    'http://www.mediafire.com/',
    'http://www.thepiratebay.se/',
    'http://www.pornhub.com/',
    'http://www.aol.com/',
    'http://www.hao123.com/',
    'http://www.espn.go.com/',
    'http://www.alibaba.com/',
    'http://www.ebay.de/',
    'http://www.google.com.tr/',
    'http://www.rakuten.co.jp/',
    'http://www.about.com/',
    'http://www.avg.com/',
    'http://www.google.com.au/',
    'http://www.blogspot.in/',
    'http://www.wordpress.org/',
    'http://www.chinaz.com/',
    'http://www.ameblo.jp/',
    'http://www.ebay.co.uk/',
    'http://www.godaddy.com/',
    'http://www.amazon.de/',
    'http://www.google.pl/',
    'http://www.uol.com.br/',
    'http://www.360buy.com/',
    'http://www.stackoverflow.com/',
    'http://www.amazon.co.jp/',
    'http://www.bp.blogspot.com/',
    'http://www.dailymotion.com/',
    'http://www.huffingtonpost.com/',
    'http://www.4shared.com/',
    'http://www.mercadolivre.com.br/',
    'http://www.vagalume.com.br/',
    'http://www.letras.mus.br/',
    'http://www.abril.com.br/',
    'http://www.detik.com/',
    'http://www.stafaband.info/',
    'http://www.kaskus.co.id/',
    'http://www.tribunnews.com/',
    'http://www.kompas.com/',
    'http://www.indianrail.gov.in/',
    'http://www.flipkart.com/',
    'http://www.snapdeal.com/',
    'http://www.espncricinfo.com/',
    'http://www.amazon.in/',
]


def ConvertPathToTestName(url):
  return url.replace('.', '_').replace('/', '').replace(':', '')


class PopularUrlsTest(
    serially_executed_browser_test_case.SeriallyExecutedBrowserTestCase):

  @classmethod
  def GenerateTestCases_PageLoadTest(cls, options):
    # pylint: disable=unused-argument
    for url in _POPULAR_URLS:
      yield 'page_load_%s' % ConvertPathToTestName(url), [url]

  def PageLoadTest(self, url):
    self.action_runner.Navigate(url)

  @classmethod
  def setUpClass(cls):
    super(cls, PopularUrlsTest).setUpClass()
    cls.SetBrowserOptions(cls._finder_options)
    cls.StartWPRServer(archive_path=_WPR_ARCHIVE_PATH,
                       archive_bucket=cloud_storage.PARTNER_BUCKET)
    cls.StartBrowser()
    cls.action_runner = cls.browser.tabs[0].action_runner

  @classmethod
  def tearDownClass(cls):
    super(cls, PopularUrlsTest).tearDownClass()
    cls.StopWPRServer()
