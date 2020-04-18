# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities for performing pixel-by-pixel image comparision."""

import itertools
import StringIO
from PIL import Image


def _AreTheSameSize(images):
  """Returns whether a set of images are the size size.

  Args:
    images: a list of images to compare.

  Returns:
    boolean.

  Raises:
    Exception: One image or fewer is passed in.
  """
  if len(images) > 1:
    return all(images[0].size == img.size for img in images[1:])
  else:
    raise Exception('No images passed in.')


def _GetDifferenceWithMask(image1, image2, mask=None,
                           masked_color=(225, 225, 225, 255),
                           same_color=(255, 255, 255, 255),
                           different_color=(210, 0, 0, 255)):
  """Returns an image representing the difference between the two images.

  This function computes the difference between two images taking into
  account a mask if it is provided. The final three arguments represent
  the coloration of the generated image.

  Args:
    image1: the first image to compare.
    image2: the second image to compare.
    mask: an optional mask image consisting of only black and white pixels
      where white pixels indicate the portion of the image to be masked out.
    masked_color: the color of a masked section in the resulting image.
    same_color: the color of an unmasked section that is the same.
      between images 1 and 2 in the resulting image.
    different_color: the color of an unmasked section that is different
      between images 1 and 2 in the resulting image.

  Returns:
    A 2-tuple with an image representing the unmasked difference between the
    two input images and the number of different pixels.

  Raises:
    Exception: if image1, image2, and mask are not the same size.
  """
  image_mask = mask
  if not mask:
    image_mask = Image.new('RGBA', image1.size, (0, 0, 0, 255))
  if not _AreTheSameSize([image1, image2, image_mask]):
    raise Exception('images and mask must be the same size.')
  image_diff = Image.new('RGBA', image1.size, (0, 0, 0, 255))
  data = []
  diff_pixels = 0
  for m, px1, px2 in itertools.izip(image_mask.getdata(),
                                    image1.getdata(),
                                    image2.getdata()):
    if m == (255, 255, 255, 255):
      data.append(masked_color)
    elif px1 == px2:
      data.append(same_color)
    else:
      data.append(different_color)
      diff_pixels += 1

  image_diff.putdata(data)
  return (image_diff, diff_pixels)


def CreateMask(images):
  """Computes a mask for a set of images.

  Returns a difference mask that is computed from the images
  which are passed in. The mask will have a white pixel
  anywhere that the input images differ and a black pixel
  everywhere else.

  Args:
    images: list of images to compute the mask from.

  Returns:
    an image of only black and white pixels where white pixels represent
      areas in the input images that have differences.

  Raises:
    Exception: if the images passed in are not of the same size.
    Exception: if fewer than one image is passed in.
  """
  if not images:
    raise Exception('mask must be created from one or more images.')
  mask = Image.new('RGBA', images[0].size, (0, 0, 0, 255))
  image = images[0]
  for other_image in images[1:]:
    mask = _GetDifferenceWithMask(
        image,
        other_image,
        mask,
        masked_color=(255, 255, 255, 255),
        same_color=(0, 0, 0, 255),
        different_color=(255, 255, 255, 255))[0]
  return mask


def AddMasks(masks):
  """Combines a list of mask images into one mask image.

  Args:
    masks: a list of mask-images.

  Returns:
    a new mask that represents the sum of the masked
      regions of the passed in list of mask-images.

  Raises:
    Exception: if masks is an empty list, or if masks are not the same size.
  """
  if not masks:
    raise Exception('masks must be a list containing at least one image.')
  if len(masks) > 1 and not _AreTheSameSize(masks):
    raise Exception('masks in list must be of the same size.')
  white = (255, 255, 255, 255)
  black = (0, 0, 0, 255)
  masks_data = [mask.getdata() for mask in masks]
  image = Image.new('RGBA', masks[0].size, black)
  image.putdata([white if white in px_set else black
                 for px_set in itertools.izip(*masks_data)])
  return image


def ConvertDiffToMask(diff):
  """Converts a Diff image into a Mask image.

  Args:
    diff: the diff image to convert.

  Returns:
    a new mask image where everything that was masked or different in the diff
    is now masked.
  """
  white = (255, 255, 255, 255)
  black = (0, 0, 0, 255)
  diff_data = diff.getdata()
  image = Image.new('RGBA', diff.size, black)
  image.putdata([black if px == white else white for px in diff_data])
  return image


def VisualizeImageDifferences(image1, image2, mask=None):
  """Returns an image repesenting the unmasked differences between two images.

  Iterates through the pixel values of two images and an optional
  mask. If the pixel values are the same, or the pixel is masked,
  (0,0,0) is stored for that pixel. Otherwise, (255,255,255) is stored.
  This ultimately produces an image where unmasked differences between
  the two images are white pixels, and everything else is black.

  Args:
    image1: an RGB image
    image2: another RGB image of the same size as image1.
    mask: an optional RGB image consisting of only white and black pixels
      where the white pixels represent the parts of the images to be masked
      out.

  Returns:
    A 2-tuple with an image representing the unmasked difference between the
    two input images and the number of different pixels.

  Raises:
    Exception: if the two images and optional mask are different sizes.
  """
  return _GetDifferenceWithMask(image1, image2, mask)


def InflateMask(image, passes):
  """A function that adds layers of pixels around the white edges of a mask.

  This function evaluates a 'frontier' of valid pixels indices. Initially,
    this frontier contains all indices in the image. However, with each pass
    only the pixels' indices which were added to the mask by inflation
    are added to the next pass's frontier. This gives the algorithm a
    large upfront cost that scales negligably when the number of passes
    is increased.

  Args:
    image: the RGBA PIL.Image mask to inflate.
    passes: the number of passes to inflate the image by.

  Returns:
    A RGBA PIL.Image.
  """
  inflated = Image.new('RGBA', image.size)
  new_dataset = list(image.getdata())
  old_dataset = list(image.getdata())

  frontier = set(range(len(old_dataset)))
  new_frontier = set()

  l = [-1, 1]

  def _ShadeHorizontal(index, px):
    col = index % image.size[0]
    if px == (255, 255, 255, 255):
      for x in l:
        if 0 <= col + x < image.size[0]:
          if old_dataset[index + x] != (255, 255, 255, 255):
            new_frontier.add(index + x)
          new_dataset[index + x] = (255, 255, 255, 255)

  def _ShadeVertical(index, px):
    row = index / image.size[0]
    if px == (255, 255, 255, 255):
      for x in l:
        if 0 <= row + x < image.size[1]:
          if old_dataset[index + image.size[0] * x] != (255, 255, 255, 255):
            new_frontier.add(index + image.size[0] * x)
          new_dataset[index + image.size[0] * x] = (255, 255, 255, 255)

  for _ in range(passes):
    for index in frontier:
      _ShadeHorizontal(index, old_dataset[index])
      _ShadeVertical(index, old_dataset[index])
    old_dataset, new_dataset = new_dataset, new_dataset
    frontier, new_frontier = new_frontier, set()
  inflated.putdata(new_dataset)
  return inflated


def TotalDifferentPixels(image1, image2, mask=None):
  """Computes the number of different pixels between two images.

  Args:
    image1: the first RGB image to be compared.
    image2: the second RGB image to be compared.
    mask: an optional RGB image of only black and white pixels
      where white pixels indicate the parts of the image to be masked out.

  Returns:
    the number of differing pixels between the images.

  Raises:
    Exception: if the images to be compared and the mask are not the same size.
  """
  image_mask = mask
  if not mask:
    image_mask = Image.new('RGBA', image1.size, (0, 0, 0, 255))
  if _AreTheSameSize([image1, image2, image_mask]):
    total_diff = 0
    for px1, px2, m in itertools.izip(image1.getdata(),
                                      image2.getdata(),
                                      image_mask.getdata()):
      if m == (255, 255, 255, 255):
        continue
      elif px1 != px2:
        total_diff += 1
      else:
        continue
    return total_diff
  else:
    raise Exception('images and mask must be the same size')


def SameImage(image1, image2, mask=None):
  """Returns a boolean representing whether the images are the same.

  Returns a boolean indicating whether two images are similar
  enough to be considered the same. Essentially wraps the
  TotalDifferentPixels function.


  Args:
    image1: an RGB image to compare.
    image2: an RGB image to compare.
    mask: an optional image of only black and white pixels
    where white pixels are masked out

  Returns:
    True if the images are similar, False otherwise.

  Raises:
    Exception: if the images (and mask) are different sizes.
  """
  different_pixels = TotalDifferentPixels(image1, image2, mask)
  return different_pixels == 0


def EncodePNG(image):
  """Returns the PNG file-contents of the image.

  Args:
    image: an RGB image to be encoded.

  Returns:
    a base64 encoded string representing the image.
  """
  f = StringIO.StringIO()
  image.save(f, 'PNG')
  encoded_image = f.getvalue()
  f.close()
  return encoded_image


def DecodePNG(png):
  """Returns a RGB image from PNG file-contents.

  Args:
    encoded_image: PNG file-contents of an RGB image.

  Returns:
    an RGB image
  """
  return Image.open(StringIO.StringIO(png))
