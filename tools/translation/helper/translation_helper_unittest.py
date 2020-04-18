# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for translation_helper.py."""
import unittest
import os
import sys

# pylint: disable=relative-import
import translation_helper

here = os.path.realpath(__file__)
testdata_path = os.path.normpath(os.path.join(here, '..', '..', 'testdata'))


class TcHelperTest(unittest.TestCase):

  def test_get_translatable_grds(self):
    grds = translation_helper.get_translatable_grds(
        testdata_path, ['test.grd', 'not_translated.grd'],
        os.path.join(testdata_path, 'translation_expectations.pyl'))
    self.assertEqual(1, len(grds))

    # There should be no references to not_translated.grd (mentioning the
    # filename here so that it doesn't appear unused).
    grd = grds[0]
    self.assertEqual(os.path.join(testdata_path, 'test.grd'), grd.path)
    self.assertEqual(testdata_path, grd.dir)
    self.assertEqual('test.grd', grd.name)
    self.assertEqual([os.path.join(testdata_path, 'part.grdp')], grd.grdp_paths)
    self.assertEqual([], grd.structure_paths)
    self.assertEqual([os.path.join(testdata_path, 'test_en-GB.xtb')],
                     grd.xtb_paths)
    self.assertEqual({
        'en-GB': os.path.join(testdata_path, 'test_en-GB.xtb')
    }, grd.lang_to_xtb_path)
    self.assertTrue(grd.appears_translatable)
    self.assertEquals(['en-GB'], grd.expected_languages)


if __name__ == '__main__':
  unittest.main()
