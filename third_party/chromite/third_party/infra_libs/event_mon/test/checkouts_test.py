# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
from infra_libs.event_mon import checkouts


class TestParseRevInfo(unittest.TestCase):
  def test_empty_file(self):
    self.assertEqual(checkouts.parse_revinfo(''), {})

  def test_one_checkout(self):
    self.assertEqual(checkouts.parse_revinfo('a: b@c'),
                     {'a': {'source_url': 'b', 'revision': 'c'}})

  def test_multiple_checkouts(self):
    self.assertEqual(checkouts.parse_revinfo('c: d@e\nf: g@h'),
                     {'c': {'source_url': 'd', 'revision': 'e'},
                      'f': {'source_url': 'g', 'revision': 'h'}})

  def test_missing_revision(self):
    self.assertEqual(checkouts.parse_revinfo('i: j'),
                     {'i': {'source_url': 'j', 'revision': None}})

  def test_on_actual_output(self):
    # pylint: disable=line-too-long
    output = """
build: https://chromium.googlesource.com/chromium/tools/build.git
build/scripts/command_wrapper/bin: https://chromium.googlesource.com/chromium/tools/command_wrapper/bin.git
build/scripts/gsd_generate_index: https://chromium.googlesource.com/chromium/tools/gsd_generate_index.git
build/scripts/private/data/reliability: https://chromium.googlesource.com/chromium/src/chrome/test/data/reliability.git
build/scripts/tools/deps2git: https://chromium.googlesource.com/chromium/tools/deps2git.git
build/third_party/gsutil: https://chromium.googlesource.com/external/gsutil/src.git@5cba434b828da428a906c8197a23c9ae120d2636
build/third_party/gsutil/boto: https://chromium.googlesource.com/external/boto.git@98fc59a5896f4ea990a4d527548204fed8f06c64
build/third_party/infra_libs: https://chromium.googlesource.com/infra/infra/packages/infra_libs.git@15ea0920b5f83d0aff4bd042e95bc388d069d51c
build/third_party/lighttpd: https://chromium.googlesource.com/chromium/deps/lighttpd.git@9dfa55d15937a688a92cbf2b7a8621b0927d06eb
build/third_party/pyasn1: https://chromium.googlesource.com/external/github.com/etingof/pyasn1.git@4181b2379eeae3d6fd9f4f76d0e6ae3789ed56e7
build/third_party/pyasn1-modules: https://chromium.googlesource.com/external/github.com/etingof/pyasn1-modules.git@956fee4f8e5fd3b1c500360dc4aa12dc5a766cb2
build/third_party/python-rsa: https://chromium.googlesource.com/external/github.com/sybrenstuvel/python-rsa.git@version-3.1.4
build/third_party/xvfb: https://chromium.googlesource.com/chromium/tools/third_party/xvfb.git
depot_tools: https://chromium.googlesource.com/chromium/tools/depot_tools.git
expect_tests: https://chromium.googlesource.com/infra/testing/expect_tests.git
infra: https://chromium.googlesource.com/infra/infra.git@c860aab0056389b04731e5f0a084a3e22c0bc8b6
infra/appengine/third_party/bootstrap: https://chromium.googlesource.com/external/github.com/twbs/bootstrap.git@b4895a0d6dc493f17fe9092db4debe44182d42ac
infra/appengine/third_party/catapult: https://chromium.googlesource.com/external/github.com/catapult-project/catapult.git@506457cbd726324f327b80ae11f46c1dfeb8710d
infra/appengine/third_party/cloudstorage: https://chromium.googlesource.com/external/github.com/GoogleCloudPlatform/appengine-gcs-client.git@76162a98044f2a481e2ef34d32b7e8196e534b78
infra/appengine/third_party/dateutil: https://chromium.googlesource.com/external/code.launchpad.net/dateutil@8c6026ba09716a4e164f5420120bfe2ebb2d9d82
infra/appengine/third_party/difflibjs: https://chromium.googlesource.com/external/github.com/qiao/difflib.js.git@e11553ba3e303e2db206d04c95f8e51c5692ca28
infra/appengine/third_party/endpoints-proto-datastore: https://chromium.googlesource.com/external/github.com/GoogleCloudPlatform/endpoints-proto-datastore.git@971bca8e31a4ab0ec78b823add5a47394d78965a
infra/appengine/third_party/gae-pytz: https://chromium.googlesource.com/external/code.google.com/p/gae-pytz@4d72fd095c91f874aaafb892859acbe3f927b3cd
infra/appengine/third_party/google-api-python-client: https://chromium.googlesource.com/external/github.com/google/google-api-python-client.git@49d45a6c3318b75e551c3022020f46c78655f365
infra/appengine/third_party/httplib2: https://chromium.googlesource.com/external/github.com/jcgregorio/httplib2.git@058a1f9448d5c27c23772796f83a596caf9188e6
infra/appengine/third_party/npm_modules: https://chromium.googlesource.com/infra/third_party/npm_modules.git@029f11ae7e3189fd57f7764b4bb43d0900e16c92
infra/appengine/third_party/oauth2client: https://chromium.googlesource.com/external/github.com/google/oauth2client.git@e8b1e794d28f2117dd3e2b8feeb506b4c199c533
infra/appengine/third_party/pipeline: https://chromium.googlesource.com/external/github.com/GoogleCloudPlatform/appengine-pipelines.git@58cf59907f67db359fe626ee06b6d3ac448c9e15
infra/appengine/third_party/six: https://chromium.googlesource.com/external/bitbucket.org/gutworth/six.git@e0898d97d5951af01ba56e86acaa7530762155c8
infra/appengine/third_party/src/github.com/golang/oauth2: https://chromium.googlesource.com/external/github.com/golang/oauth2.git@cb029f4c1f58850787981eefaf9d9bf547c1a722
infra/appengine/third_party/uritemplate: https://chromium.googlesource.com/external/github.com/uri-templates/uritemplate-py.git@1e780a49412cdbb273e9421974cb91845c124f3f
infra/go/src/github.com/luci/gae: https://chromium.googlesource.com/external/github.com/luci/gae@cfd4c03f8de6f353360064aca2d2fcf517bedf8b
infra/go/src/github.com/luci/luci-go: https://chromium.googlesource.com/external/github.com/luci/luci-go@e0e407209a72f692ef4de3908753e308d4ab798b
infra/luci: https://chromium.googlesource.com/external/github.com/luci/luci-py@3f374f9f2abc38d3a9a08a50e83bf1ada15b2e52
infra/recipes-py: https://chromium.googlesource.com/external/github.com/luci/recipes-py@origin/master
testing/expect_tests: https://chromium.googlesource.com/infra/testing/expect_tests.git
testing/testing_support: https://chromium.googlesource.com/infra/testing/testing_support.git
testing_support: https://chromium.googlesource.com/infra/testing/testing_support.git
"""
    revinfo = checkouts.parse_revinfo(output)
    # Check some invariants
    self.assertTrue(len(revinfo) > 0, 'revinfo should contain values')
    for v in revinfo.itervalues():
      self.assertEqual(len(v), 2)
      self.assertIn('://', v['source_url'],
                    msg='"://" not in url string. Got %s' % v['source_url'])
      self.assertEqual(v['source_url'].split('://')[0], 'https')
