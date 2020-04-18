#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from server_instance import ServerInstance
from third_party.motemplate import Motemplate


class TemplateRendererTest(unittest.TestCase):
  '''Basic test for TemplateRenderer.

  When the DataSourceRegistry conversion is finished then we could do some more
  meaningful tests by injecting a different set of DataSources.
  '''

  def setUp(self):
    self._template_renderer = ServerInstance.ForLocal().template_renderer

  def testSimpleWiring(self):
    template = Motemplate('hello {{?true}}{{strings.extension}}{{/}}')
    text, warnings = self._template_renderer.Render(template, None)
    self.assertEqual('hello extension', text)
    self.assertEqual([], warnings)


if __name__ == '__main__':
  unittest.main()
