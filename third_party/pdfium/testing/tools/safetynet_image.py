# Copyright 2017 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compares pairs of page images and generates an HTML to look at differences.
"""

import functools
import glob
import multiprocessing
import os
import re
import subprocess
import sys
import webbrowser

from common import DirectoryFinder


def GenerateOneDiffParallel(image_comparison, image):
  return image_comparison.GenerateOneDiff(image)


class ImageComparison(object):
  """Compares pairs of page images and generates an HTML to look at differences.

  The images are all assumed to have the same name and be in two directories:
  [output_path]/[two_labels[0]] and [output_path]/[two_labels[1]]. For example,
  if output_path is "/tmp/images" and two_labels is ("before", "after"),
  images in /tmp/images/before will be compared to /tmp/images/after. The HTML
  produced will be in /tmp/images/compare.html and have relative links to these
  images, so /tmp/images is self-contained and can be moved around or shared.
  """

  def __init__(self, build_dir, output_path, two_labels,
               num_workers, threshold_fraction):
    """Constructor.

    Args:
      build_dir: Path to the build directory.
      output_path: Path with the pngs and where the html will be created.
      two_labels: Tuple of two strings that name the subdirectories in
          output_path containing the images.
      num_workers: Number of worker threads to start.
      threshold_fraction: Minimum percentage (0.0 to 1.0) of pixels below which
          an image is considered to have only small changes. They will not be
          displayed on the HTML, only listed.
    """
    self.build_dir = build_dir
    self.output_path = output_path
    self.two_labels = two_labels
    self.num_workers = num_workers
    self.threshold = threshold_fraction * 100

  def Run(self, open_in_browser):
    """Runs the comparison and generates an HTML with the results.

    Returns:
        Exit status.
    """

    if len(self.two_labels) != 2:
      print >> sys.stderr, 'two_labels must be a tuple of length 2'
      return 1

    finder = DirectoryFinder(self.build_dir)
    self.img_diff_bin = finder.ExecutablePath('pdfium_diff')

    html_path = os.path.join(self.output_path, 'compare.html')

    self.diff_path = os.path.join(self.output_path, 'diff')
    if not os.path.exists(self.diff_path):
      os.makedirs(self.diff_path)

    self.image_locations = ImageLocations(
        self.output_path, self.diff_path, self.two_labels)

    difference = self._GenerateDiffs()

    small_changes = []

    with open(html_path, 'w') as f:
      f.write('<html><body>')
      f.write('<table>')
      for image in self.image_locations.Images():
        diff = difference[image]
        if diff is None:
          print >> sys.stderr, 'Failed to compare image %s' % image
        elif diff > self.threshold:
          self._WriteImageRows(f, image, diff)
        else:
          small_changes.append((image, diff))
      self._WriteSmallChanges(f, small_changes)
      f.write('</table>')
      f.write('</body></html>')

    if open_in_browser:
      webbrowser.open(html_path)

    return 0

  def _GenerateDiffs(self):
    """Runs a diff over all pairs of page images, producing diff images.

    As a side effect, the diff images will be saved to [output_path]/diff
    with the same image name.

    Returns:
      A dict mapping image names to percentage of pixels changes.
    """
    difference = {}
    pool = multiprocessing.Pool(self.num_workers)
    worker_func = functools.partial(GenerateOneDiffParallel, self)

    try:
      # The timeout is a workaround for http://bugs.python.org/issue8296
      # which prevents KeyboardInterrupt from working.
      one_year_in_seconds = 3600 * 24 * 365
      worker_results = (pool.map_async(worker_func,
                                       self.image_locations.Images())
                        .get(one_year_in_seconds))
      for worker_result in worker_results:
        image, result = worker_result
        difference[image] = result
    except KeyboardInterrupt:
      pool.terminate()
      sys.exit(1)
    else:
      pool.close()

    pool.join()

    return difference

  def GenerateOneDiff(self, image):
    """Runs a diff over one pair of images, producing a diff image.

    As a side effect, the diff image will be saved to [output_path]/diff
    with the same image name.

    Args:
      image: Page image to compare.

    Returns:
      A tuple (image, diff), where image is the parameter and diff is the
      percentage of pixels changed.
    """
    try:
      subprocess.check_output(
          [self.img_diff_bin,
           self.image_locations.Left(image),
           self.image_locations.Right(image)])
    except subprocess.CalledProcessError as e:
      percentage_change = float(re.findall(r'\d+\.\d+', e.output)[0])
    else:
      return image, 0

    try:
      subprocess.check_output(
          [self.img_diff_bin,
           '--diff',
           self.image_locations.Left(image),
           self.image_locations.Right(image),
           self.image_locations.Diff(image)])
    except subprocess.CalledProcessError as e:
      return image, percentage_change
    else:
      print >> sys.stderr, 'Warning: Should have failed the previous diff.'
      return image, 0

  def _GetRelativePath(self, absolute_path):
    return os.path.relpath(absolute_path, start=self.output_path)

  def _WriteImageRows(self, f, image, diff):
    """Write table rows for a page image comparing its two versions.

    Args:
      f: Open HTML file to write to.
      image: Image file name.
      diff: Percentage of different pixels.
    """
    f.write('<tr><td colspan="2">')
    f.write('%s (%.4f%% changed)' % (image, diff))
    f.write('</td></tr>')

    f.write('<tr>')
    self._WritePageCompareTd(
        f,
        self._GetRelativePath(self.image_locations.Left(image)),
        self._GetRelativePath(self.image_locations.Right(image)))
    self._WritePageTd(
        f,
        self._GetRelativePath(self.image_locations.Diff(image)))
    f.write('</tr>')

  def _WritePageTd(self, f, image_path):
    """Write table column with a single image.

    Args:
      f: Open HTML file to write to.
      image_path: Path to image file.
    """
    f.write('<td>')
    f.write('<img src="%s">' % image_path)
    f.write('</td>')

  def _WritePageCompareTd(self, f, normal_image_path, hover_image_path):
    """Write table column for an image comparing its two versions.

    Args:
      f: Open HTML file to write to.
      normal_image_path: Path to image to be used in the "normal" state.
      hover_image_path: Path to image to be used in the "hover" state.
    """
    f.write('<td>')
    f.write('<img src="%s" '
            'onmouseover="this.src=\'%s\';" '
            'onmouseout="this.src=\'%s\';">' % (normal_image_path,
                                                hover_image_path,
                                                normal_image_path))
    f.write('</td>')

  def _WriteSmallChanges(self, f, small_changes):
    """Write table rows for all images considered to have only small changes.

    Args:
      f: Open HTML file to write to.
      small_changes: List of (image, change) tuples, where image is the page
          image and change is the percentage of pixels changed.
    """
    for image, change in small_changes:
      f.write('<tr><td colspan="2">')
      if not change:
        f.write('No change for: %s' % image)
      else:
        f.write('Small change of %.4f%% for: %s' % (change, image))
      f.write('</td></tr>')


class ImageLocations(object):
  """Contains the locations of input and output image files.
  """

  def __init__(self, output_path, diff_path, two_labels):
    """Constructor.

    Args:
      output_path: Path to directory with the pngs.
      diff_path: Path to directory where the diffs will be generated.
      two_labels: Tuple of two strings that name the subdirectories in
          output_path containing the images.
    """
    self.output_path = output_path
    self.diff_path = diff_path
    self.two_labels = two_labels

    self.left = self._FindImages(self.two_labels[0])
    self.right = self._FindImages(self.two_labels[1])

    self.images = list(self.left.viewkeys() & self.right.viewkeys())

    # Sort by pdf filename, then page number
    def KeyFn(s):
      pieces = s.rsplit('.', 2)
      return (pieces[0], int(pieces[1]))

    self.images.sort(key=KeyFn)
    self.diff = {image: os.path.join(self.diff_path, image)
                 for image in self.images}

  def _FindImages(self, label):
    """Traverses a dir and builds a dict of all page images to compare in it.

    Args:
      label: name of subdirectory of output_path to traverse.

    Returns:
      Dict mapping page image names to the path of the image file.
    """
    image_path_matcher = os.path.join(self.output_path, label, '*.*.png')
    image_paths = glob.glob(image_path_matcher)

    image_dict = {os.path.split(image_path)[1]: image_path
                  for image_path in image_paths}

    return image_dict

  def Images(self):
    """Returns a list of all page images present in both directories."""
    return self.images

  def Left(self, test_case):
    """Returns the path for a page image in the first subdirectory."""
    return self.left[test_case]

  def Right(self, test_case):
    """Returns the path for a page image in the second subdirectory."""
    return self.right[test_case]

  def Diff(self, test_case):
    """Returns the path for a page diff image."""
    return self.diff[test_case]
