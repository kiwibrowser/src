#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import pretty_print


PRETTY_XML = """
<!-- Comment1 -->

<rappor-configuration>
<!-- Comment2 -->

<rappor-parameter-types>
<!-- Comment3 -->

<rappor-parameters name="TEST_RAPPOR_TYPE">
  <summary>
    Fake type for tests.
  </summary>
  <parameters num-cohorts="128" bytes="1" hash-functions="2" fake-prob="0.5"
      fake-one-prob="0.5" one-coin-prob="0.75" zero-coin-prob="0.25"
      reporting-level="COARSE"/>
</rappor-parameters>

</rappor-parameter-types>

<rappor-metrics>
<!-- Comment4 -->

<rappor-metric name="Test.Rappor.Metric" type="TEST_RAPPOR_TYPE">
  <owner>user1@chromium.org</owner>
  <owner>user2@chromium.org</owner>
  <summary>
    A fake metric summary.
  </summary>
</rappor-metric>

<rappor-metric name="Test.Rappor.Metric2" type="TEST_RAPPOR_TYPE">
  <owner>user1@chromium.org</owner>
  <owner>user2@chromium.org</owner>
  <summary>
    A fake metric summary.
  </summary>
  <string-field name="Url">
    <summary>
      The url of the event.
    </summary>
  </string-field>
  <flags-field name="Flags">
    <flag>Flag bit #1</flag>
    <flag>Flag bit #2</flag>
  </flags-field>
</rappor-metric>

</rappor-metrics>

</rappor-configuration>
""".strip()

BASIC_METRIC = {
  'comments': [],
  'name': 'Test.Rappor.Metric',
  'type': 'TEST_RAPPOR_TYPE',
  'owners': ['user1@chromium.org', 'user2@chromium.org'],
  'summary': 'A fake metric summary.',
  'flags': [],
  'strings': [],
}

MULTI_FIELD_METRIC = {
  'comments': [],
  'name': 'Test.Rappor.Metric2',
  'type': 'TEST_RAPPOR_TYPE',
  'owners': ['user1@chromium.org', 'user2@chromium.org'],
  'summary': 'A fake metric summary.',
  'strings': [{
    'comments': [],
    'name': 'Url',
    'summary': 'The url of the event.',
  }],
  'flags': [{
    'comments': [],
    'name': 'Flags',
    'flags': [
      'Flag bit #1',
      'Flag bit #2',
    ]
  }]
}


class ActionXmlTest(unittest.TestCase):

  def testIsPretty(self):
    result = pretty_print.UpdateXML(PRETTY_XML)
    self.assertMultiLineEqual(PRETTY_XML, result.strip())

  def testParsing(self):
    comments, config = pretty_print.RAPPOR_XML_TYPE.Parse(PRETTY_XML)
    self.assertEqual(BASIC_METRIC, config['metrics']['metrics'][0])
    self.assertEqual(MULTI_FIELD_METRIC, config['metrics']['metrics'][1])
    self.assertEqual(set(['TEST_RAPPOR_TYPE']),
                     pretty_print.GetTypeNames(config))

  def testMissingOwners(self):
    self.assertFalse(pretty_print.GetMissingOwnerErrors([BASIC_METRIC]))
    no_owners = BASIC_METRIC.copy()
    no_owners['owners'] = []
    self.assertTrue(pretty_print.GetMissingOwnerErrors([no_owners]))

  def testInvalidTypes(self):
    self.assertFalse(pretty_print.GetInvalidTypeErrors(
        set(['TEST_RAPPOR_TYPE']), [BASIC_METRIC]))
    self.assertTrue(pretty_print.GetInvalidTypeErrors(
        set(['OTHER_TYPE']), [BASIC_METRIC]))


if __name__ == '__main__':
  unittest.main()
