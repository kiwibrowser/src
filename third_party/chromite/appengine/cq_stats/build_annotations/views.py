# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""All django views for the build_annotations app."""

from __future__ import print_function

from django import http
from django import shortcuts
from django.core import urlresolvers
from django.views import generic
from google.appengine.api import users

from build_annotations import build_row_controller
from build_annotations import models as ba_models
from build_annotations import forms as ba_forms
from chromite.lib import constants

import urllib

_DEFAULT_USERNAME = "SomeoneGotHereWithoutLoggingIn"

def _BuildUrl(url_name, url_kwargs=None, get_kwargs=None):
  """Build a url via urlresolver, and attach GET arguments

  Args:
    url_name: Canonical name of the url.
    url_kwargs: Arugment accepted by the url builder for the view.
    get_kwargs: Url arguments for GET query. Accepts a dict or django's
        QueryDict object.
  """
  url = urlresolvers.reverse(url_name, kwargs=url_kwargs)
  if get_kwargs is not None:
    url += '?' + urllib.urlencode(get_kwargs, doseq=True)
  return url


class ListBuildsView(generic.list.ListView):
  """The landing page view of the app. Lists requested builds."""

  template_name = 'build_annotations/index.html'

  def __init__(self, *args, **kwargs):
    super(ListBuildsView, self).__init__(*args, **kwargs)
    self._username = _DEFAULT_USERNAME
    self._build_config = None
    self._search_form = None
    self._controller = None
    self._builds_list = None
    self._latest_build_id = None
    self._num_builds = None
    self._hist = None

  def get_queryset(self):
    self._controller = build_row_controller.BuildRowController()
    build_config_q = None
    if self._build_config is not None:
      build_config_q = self._controller.GetQRestrictToBuildConfig(
          self._build_config)
    self._builds_list = self._controller.GetStructuredBuilds(
        latest_build_id=self._latest_build_id,
        num_builds=self._num_builds,
        extra_filter_q=build_config_q)
    self._hist = self._controller.GetHandlingTimeHistogram(
        latest_build_id=self._latest_build_id,
        num_builds=self._num_builds,
        extra_filter_q=build_config_q)
    return self._builds_list

  def get_context_object_name(self, _):
    return 'builds_list'

  def get_context_data(self, **kwargs):
    context = super(ListBuildsView, self).get_context_data(**kwargs)
    context['username'] = self._username
    context['search_form'] = self._GetSearchForm()
    context['build_config'] = self._build_config
    context['histogram_data'] = self._hist
    return context

  # pylint: disable=arguments-differ
  def get(self, request, build_config=None):
    # We're assured that a username exists in prod because our app sits behind
    # appengine login. Not so when running from dev_appserver.
    self._username = users.get_current_user()
    self._build_config = build_config

    self._PopulateUrlArgs(request.GET.get('latest_build_id'),
                          request.GET.get('num_builds'))
    return super(ListBuildsView, self).get(request)

  def post(self, request, build_config):
    self._username = users.get_current_user()
    self._build_config = build_config
    self._latest_build_id = request.GET.get('latest_build_id')
    self._num_builds = request.GET.get('num_builds')

    form = ba_forms.SearchForm(request.POST)
    self._search_form = form
    if not form.is_valid():
      return super(ListBuildsView, self).get(request)

    self._latest_build_id = form.cleaned_data['latest_build_id']
    self._num_builds = form.cleaned_data['num_builds']
    return http.HttpResponseRedirect(_BuildUrl(
        'build_annotations:builds_list',
        url_kwargs={'build_config': build_config},
        get_kwargs={'latest_build_id': self._latest_build_id,
                    'num_builds': self._num_builds}))

  def put(self, *args, **kwargs):
    return self.post(*args, **kwargs)

  def _GetSearchForm(self):
    if self._search_form is not None:
      return self._search_form
    return ba_forms.SearchForm(
        {'latest_build_id': self._latest_build_id,
         'num_builds': self._num_builds})

  def _PopulateUrlArgs(self, latest_build_id, num_builds):
    self._latest_build_id = latest_build_id
    self._num_builds = num_builds

    if self._latest_build_id is None or self._num_builds is None:
      controller = build_row_controller.BuildRowController()
      controller.GetStructuredBuilds(num_builds=1)

    if self._latest_build_id is None:
      self._latest_build_id = controller.latest_build_id
    if self._num_builds is None:
      self._num_builds = controller.DEFAULT_NUM_BUILDS

  def _GetLatestBuildId(self):
    controller = build_row_controller.BuildRowController()
    controller.GetStructuredBuilds(num_builds=1)
    return controller.latest_build_id


class EditAnnotationsView(generic.base.View):
  """View that handles annotation editing page."""

  template_name = 'build_annotations/edit_annotations.html'

  def __init__(self, *args, **kwargs):
    self._username = _DEFAULT_USERNAME
    self._annotations_formset = None
    self._finalize_form = None
    self._context = {}
    self._build_config = None
    self._build_id = None
    super(EditAnnotationsView, self).__init__(*args, **kwargs)

  def get(self, request, build_config, build_id):
    # We're assured that a username exists in prod because our app sits behind
    # appengine login. Not so when running from dev_appserver.
    self._username = users.get_current_user()
    self._build_config = build_config
    self._build_id = build_id
    self._PopulateContext()
    return shortcuts.render(request, self.template_name, self._context)

  def post(self, request, build_config, build_id):
    # We're assured that a username exists in prod because our app sits behind
    # appengine login. Not so when running from dev_appserver.
    self._username = users.get_current_user()
    self._build_config = build_config
    self._build_id = build_id

    finalize_form = ba_forms.FinalizeForm(request.POST)
    if finalize_form.is_valid() and finalize_form['finalize'].value():
      self._MaybeSaveFinalizeMessage()

    self._annotations_formset = ba_forms.AnnotationsFormSet(request.POST)
    if self._annotations_formset.is_valid():
      self._SaveAnnotations()
      return http.HttpResponseRedirect(_BuildUrl(
          'build_annotations:edit_annotations',
          url_kwargs={'build_config': build_config,
                      'build_id': build_id},
          get_kwargs=request.GET))
    else:
      self._PopulateContext()
      return shortcuts.render(request, self.template_name, self._context)

  def _PopulateContext(self):
    build_row = self._GetBuildRow()
    if build_row is None:
      raise http.Http404

    self._context = {}
    self._context['username'] = self._username
    self._context['build_config'] = self._build_config
    self._context['build_row'] = build_row
    self._context['annotations_formset'] = self._GetAnnotationsFormSet()
    self._context['finalize_form'] = self._GetFinalizeForm()

  def _MaybeSaveFinalizeMessage(self):
    """Creates a finalize buildMessage for this build."""
    has_finalize_message = self._GetFinalizeRow().exists()
    if not has_finalize_message:
      build = self._GetBuildRow().build_entry
      ba_models.BuildMessageTable(
          build_id=build,
          message_type=
              ba_models.BuildMessageTable.MESSAGE_TYPES.ANNOTATIONS_FINALIZED,
      ).save()

  def _GetFinalizeRow(self):
    """Gets the existing finalize message for this build_id, if it exists."""
    return ba_models.BuildMessageTable.objects.filter(
        build_id=self._build_id,
        message_type=
            ba_models.BuildMessageTable.MESSAGE_TYPES.ANNOTATIONS_FINALIZED,
    )

  def _GetFinalizeForm(self):
    """Creates a finalize form for marking annotations as finalized.

    'You FOOL! This isn't even my final form!' - Frieza
    """
    has_finalize_message = self._GetFinalizeRow().exists()
    return ba_forms.FinalizeForm({'finalize': has_finalize_message})

  def _GetBuildRow(self):
    controller = build_row_controller.BuildRowController()
    build_row_list = controller.GetStructuredBuilds(
        latest_build_id=self._build_id,
        num_builds=1)
    if not build_row_list:
      return None
    return build_row_list[0]

  def _GetAnnotationsFormSet(self):
    if self._annotations_formset is None:
      build_row = self._GetBuildRow()
      if build_row is not None:
        queryset = build_row.GetAnnotationsQS()
      else:
        queryset = ba_models.AnnotationsTable.objects.none()
      self._annotations_formset = ba_forms.AnnotationsFormSet(queryset=queryset)
    return self._annotations_formset

  def _SaveAnnotations(self):
    models_to_save = self._annotations_formset.save(commit=False)
    build_row = self._GetBuildRow()
    for model in models_to_save:
      if not hasattr(model, 'build_id') or model.build_id is None:
        model.build_id = build_row.build_entry
      model.last_annotator = self._username
      model.save()
