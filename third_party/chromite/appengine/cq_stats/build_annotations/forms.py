# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Forms used by the build_annotations app."""

from __future__ import print_function

from django import forms

from build_annotations import models as ba_models


class SearchForm(forms.Form):
  """Form to limit builds shown on the landing page."""
  latest_build_id = forms.IntegerField()
  num_builds = forms.IntegerField(label='Number of results')


class AnnotationsForm(forms.ModelForm):
  """Form to add/edit a single annotation to a build."""

  # pylint: disable=no-init, old-style-class
  class Meta:
    """Set meta options for the form."""
    model = ba_models.AnnotationsTable
    fields = ['failure_category', 'failure_message', 'blame_url', 'notes',
              'deleted']

class FinalizeForm(forms.Form):
  """Form to add/remove an annotations_finalized buildMessage."""
  finalize = forms.BooleanField(
      required=False, initial=False, label='Annotations Finalized')

# NB: Explicitly set can_delete=False for clarity.
# Due to a bug in (< django-1.7), models get deleted when the formset is saved
# even if we request not to commit changes.
AnnotationsFormSet = forms.models.modelformset_factory(
    ba_models.AnnotationsTable,
    form=AnnotationsForm,
    can_delete=False)
