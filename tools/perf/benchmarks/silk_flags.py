# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


def CustomizeBrowserOptionsForSoftwareRasterization(options):
  """Enables flags needed for forced software rasterization."""
  options.AppendExtraBrowserArgs('--disable-gpu-rasterization')


def CustomizeBrowserOptionsForGpuRasterization(options):
  """Enables flags needed for forced GPU rasterization using Ganesh."""
  options.AppendExtraBrowserArgs('--force-gpu-rasterization')


def CustomizeBrowserOptionsForSyncScrolling(options):
  """Enables flags needed for synchronous (main thread) scrolling."""
  options.AppendExtraBrowserArgs('--disable-threaded-scrolling')

def CustomizeBrowserOptionsForThreadTimes(options):
  """Disables options known to cause noise in thread times"""
  # Remove once crbug.com/621128 is fixed.
  options.AppendExtraBrowserArgs('--disable-top-sites')
