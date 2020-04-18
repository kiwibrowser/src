# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import sys
import os
from PIL import Image

import image_tools


def _GenImage(size, color):
  return Image.new('RGBA', size, color)


def _AllPixelsOfColor(image, color):
  return not any(px != color for px in image.getdata())


class ImageToolsTest(unittest.TestCase):

  def setUp(self):
    self.black25 = _GenImage((25, 25), (0, 0, 0, 255))
    self.black50 = _GenImage((50, 50), (0, 0, 0, 255))
    self.white25 = _GenImage((25, 25), (255, 255, 255, 255))
    self.white50 = _GenImage((50, 50), (255, 255, 255, 255))

  def testAreTheSameSize(self):
    self.assertTrue(image_tools._AreTheSameSize([self.black25, self.black25]))
    self.assertTrue(image_tools._AreTheSameSize([self.white25, self.white25]))
    self.assertTrue(image_tools._AreTheSameSize([self.black50, self.black50]))
    self.assertTrue(image_tools._AreTheSameSize([self.white50, self.white50]))
    self.assertTrue(image_tools._AreTheSameSize([self.black25, self.white25]))
    self.assertTrue(image_tools._AreTheSameSize([self.black50, self.white50]))

    self.assertFalse(image_tools._AreTheSameSize([self.black50, self.black25]))
    self.assertFalse(image_tools._AreTheSameSize([self.white50, self.white25]))
    self.assertFalse(image_tools._AreTheSameSize([self.black25, self.white50]))
    self.assertFalse(image_tools._AreTheSameSize([self.black50, self.white25]))

    self.assertRaises(Exception, image_tools._AreTheSameSize, [])
    self.assertRaises(Exception, image_tools._AreTheSameSize, [self.black50])

  def testGetDifferenceWithMask(self):
    self.assertTrue(_AllPixelsOfColor(image_tools._GetDifferenceWithMask(
        self.black25, self.black25)[0], (255, 255, 255, 255)))
    self.assertTrue(_AllPixelsOfColor(image_tools._GetDifferenceWithMask(
        self.white25, self.black25)[0], (210, 0, 0, 255)))
    self.assertTrue(_AllPixelsOfColor(image_tools._GetDifferenceWithMask(
        self.black25, self.black25, mask=self.black25)[0],
                                      (255, 255, 255, 255)))
    self.assertTrue(_AllPixelsOfColor(image_tools._GetDifferenceWithMask(
        self.black25, self.black25, mask=self.white25)[0],
                                      (225, 225, 225, 255)))
    self.assertTrue(_AllPixelsOfColor(image_tools._GetDifferenceWithMask(
        self.black25, self.white25, mask=self.black25)[0],
                                      (210, 0, 0, 255)))
    self.assertTrue(_AllPixelsOfColor(image_tools._GetDifferenceWithMask(
        self.black25, self.white25, mask=self.white25)[0],
                                      (225, 225, 225, 255)))
    self.assertRaises(Exception, image_tools._GetDifferenceWithMask,
                      self.white25,
                      self.black50)
    self.assertRaises(Exception, image_tools._GetDifferenceWithMask,
                      self.white25,
                      self.white25,
                      mask=self.black50)

  def testCreateMask(self):
    m1 = image_tools.CreateMask([self.black25, self.white25])
    self.assertTrue(_AllPixelsOfColor(m1, (255, 255, 255, 255)))
    m2 = image_tools.CreateMask([self.black25, self.black25])
    self.assertTrue(_AllPixelsOfColor(m2, (0, 0, 0, 255)))
    m3 = image_tools.CreateMask([self.white25, self.white25])
    self.assertTrue(_AllPixelsOfColor(m3, (0, 0, 0, 255)))

  def testAddMasks(self):
    m1 = image_tools.CreateMask([self.black25, self.white25])
    m2 = image_tools.CreateMask([self.black25, self.black25])
    m3 = image_tools.CreateMask([self.black50, self.black50])
    self.assertTrue(_AllPixelsOfColor(image_tools.AddMasks([m1]),
                                      (255, 255, 255, 255)))
    self.assertTrue(_AllPixelsOfColor(image_tools.AddMasks([m2]),
                                      (0, 0, 0, 255)))
    self.assertTrue(_AllPixelsOfColor(image_tools.AddMasks([m1, m2]),
                                      (255, 255, 255, 255)))
    self.assertTrue(_AllPixelsOfColor(image_tools.AddMasks([m1, m1]),
                                      (255, 255, 255, 255)))
    self.assertTrue(_AllPixelsOfColor(image_tools.AddMasks([m2, m2]),
                                      (0, 0, 0, 255)))
    self.assertTrue(_AllPixelsOfColor(image_tools.AddMasks([m3]),
                                      (0, 0, 0, 255)))
    self.assertRaises(Exception, image_tools.AddMasks, [])
    self.assertRaises(Exception, image_tools.AddMasks, [m1, m3])

  def testTotalDifferentPixels(self):
    self.assertEquals(image_tools.TotalDifferentPixels(self.white25,
                                                       self.white25),
                      0)
    self.assertEquals(image_tools.TotalDifferentPixels(self.black25,
                                                       self.black25),
                      0)
    self.assertEquals(image_tools.TotalDifferentPixels(self.white25,
                                                       self.black25),
                      25*25)
    self.assertEquals(image_tools.TotalDifferentPixels(self.white25,
                                                       self.black25,
                                                       mask=self.white25),
                      0)
    self.assertEquals(image_tools.TotalDifferentPixels(self.white25,
                                                       self.white25,
                                                       mask=self.white25),
                      0)
    self.assertEquals(image_tools.TotalDifferentPixels(self.white25,
                                                       self.black25,
                                                       mask=self.black25),
                      25*25)
    self.assertEquals(image_tools.TotalDifferentPixels(self.white25,
                                                       self.white25,
                                                       mask=self.black25),
                      0)
    self.assertRaises(Exception, image_tools.TotalDifferentPixels,
                      self.white25, self.white50)
    self.assertRaises(Exception, image_tools.TotalDifferentPixels,
                      self.white25, self.white25, mask=self.white50)

  def testSameImage(self):
    self.assertTrue(image_tools.SameImage(self.white25, self.white25))
    self.assertFalse(image_tools.SameImage(self.white25, self.black25))

    self.assertTrue(image_tools.SameImage(self.white25, self.black25,
                                          mask=self.white25))
    self.assertFalse(image_tools.SameImage(self.white25, self.black25,
                                           mask=self.black25))
    self.assertTrue(image_tools.SameImage(self.black25, self.black25))
    self.assertTrue(image_tools.SameImage(self.black25, self.black25,
                                          mask=self.white25))
    self.assertTrue(image_tools.SameImage(self.white25, self.white25,
                                          mask=self.white25))
    self.assertRaises(Exception, image_tools.SameImage,
                      self.white25, self.white50)
    self.assertRaises(Exception, image_tools.SameImage,
                      self.white25, self.white25,
                      mask=self.white50)

  def testInflateMask(self):
    cross_image = Image.new('RGBA', (3, 3))
    white_image = Image.new('RGBA', (3, 3))
    dot_image = Image.new('RGBA', (3, 3))
    b = (0, 0, 0, 255)
    w = (255, 255, 255, 255)
    dot_image.putdata([b, b, b,
                       b, w, b,
                       b, b, b])
    cross_image.putdata([b, w, b,
                         w, w, w,
                         b, w, b])
    white_image.putdata([w, w, w,
                         w, w, w,
                         w, w, w])
    self.assertEquals(list(image_tools.InflateMask(dot_image, 1).getdata()),
                      list(cross_image.getdata()))
    self.assertEquals(list(image_tools.InflateMask(dot_image, 0).getdata()),
                      list(dot_image.getdata()))
    self.assertEquals(list(image_tools.InflateMask(dot_image, 2).getdata()),
                      list(white_image.getdata()))
    self.assertEquals(list(image_tools.InflateMask(dot_image, 3).getdata()),
                      list(white_image.getdata()))
    self.assertEquals(list(image_tools.InflateMask(self.black25, 1).getdata()),
                      list(self.black25.getdata()))

  def testPNGEncodeDecode(self):
    self.assertTrue(_AllPixelsOfColor(
        image_tools.DecodePNG(
            image_tools.EncodePNG(self.white25)), (255, 255, 255, 255)))
    self.assertTrue(_AllPixelsOfColor(
        image_tools.DecodePNG(
            image_tools.EncodePNG(self.black25)), (0, 0, 0, 255)))


if __name__ == '__main__':
  unittest.main()
