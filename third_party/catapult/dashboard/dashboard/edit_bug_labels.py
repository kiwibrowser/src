# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides the web interface for adding and removing bug labels."""

import json

from dashboard.common import request_handler
from dashboard.common import xsrf
from dashboard.models import bug_label_patterns


class EditBugLabelsHandler(request_handler.RequestHandler):
  """Handles editing the info about perf sheriff rotations."""

  def get(self):
    """Renders the UI with all of the forms."""
    patterns_dict = bug_label_patterns.GetBugLabelPatterns()
    self.RenderHtml('edit_bug_labels.html', {
        'bug_labels': sorted(patterns_dict),
        'bug_labels_json': json.dumps(patterns_dict, indent=2, sort_keys=True)
    })

  @xsrf.TokenRequired
  def post(self):
    """Updates the sheriff configurations.

    Each form on the edit sheriffs page has a hidden field called action, which
    tells us which form was submitted. The other particular parameters that are
    expected depend on which form was submitted.
    """
    action = self.request.get('action')
    if action == 'add_buglabel_pattern':
      self._AddBuglabelPattern()
    if action == 'remove_buglabel_pattern':
      self._RemoveBuglabelPattern()

  def _AddBuglabelPattern(self):
    """Adds a bug label to be added to a group of tests.

    Request parameters:
      buglabel_to_add: The bug label, which is a BugLabelPattern entity name.
      pattern: A test path pattern.
    """
    label = self.request.get('buglabel_to_add')
    pattern = self.request.get('pattern')
    bug_label_patterns.AddBugLabelPattern(label, pattern)
    self.RenderHtml('result.html', {
        'headline': 'Added label %s' % label,
        'results': [{'name': 'Pattern', 'value': pattern}]
    })

  def _RemoveBuglabelPattern(self):
    """Removes a BugLabelPattern so that the label no longer applies.

    Request parameters:
      buglabel_to_remove: The bug label, which is the name of a
      BugLabelPattern entity.
    """
    label = self.request.get('buglabel_to_remove')
    bug_label_patterns.RemoveBugLabel(label)
    self.RenderHtml('result.html', {
        'headline': 'Deleted label %s' % label
    })
