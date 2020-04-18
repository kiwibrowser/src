# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides the web interface for the memory infra graph picker."""

from dashboard import chart_handler

class MemoryReportHandler(chart_handler.ChartHandler):
  """URL endpoint for /memory_report page."""

  def get(self):
    """Renders the memory infra specific graph selection UI."""
    self.RenderStaticHtml('memory_report.html')
