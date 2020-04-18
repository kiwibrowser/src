# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import common
from common import TestDriver
from common import IntegrationTest
from decorators import NotAndroid


class DataUseAscription(IntegrationTest):

  # This test uses a desktop extension and cannot be run on Android.
  @NotAndroid
  def testDataUseAscription(self):
    ext_path = os.path.join(os.path.dirname(__file__), os.pardir, os.pardir,
                            os.pardir, 'chrome', 'test', 'data',
                            'chromeproxy', 'extension')
    with TestDriver() as t:
      t.AddChromeArg('--enable-spdy-proxy-auth')
      t.AddChromeArg('--load-extension=%s' % ext_path)

      # Load the URL and verify the via header appears.
      t.LoadURL('http://check.googlezip.net/test.html')
      responses = t.GetHTTPResponses()
      for response in responses:
        self.assertHasChromeProxyViaHeader(response)

      # Load the extension page and verify the host appears in data use list.
      t.LoadURL('chrome-extension://pfmgfdlgomnbgkofeojodiodmgpgmkac/'
                'detailed_data_usage.html')
      xpath = ('//span[@class=\'hostname\' and '
               'contains(text(),\'check.googlezip.net\')]')
      js_xpath_query = ("document.evaluate(\"%s\", document, null, "
                        "XPathResult.ANY_UNORDERED_NODE_TYPE, null )"
                        ".singleNodeValue;" % xpath)
      has_expected_host = bool(t.ExecuteJavascriptStatement(js_xpath_query))
      self.assertTrue(has_expected_host,
                      "Test host failed to appear in data use page")

if __name__ == '__main__':
  IntegrationTest.RunAllTests()
