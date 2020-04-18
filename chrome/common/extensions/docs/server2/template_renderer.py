# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from data_source_registry import CreateDataSources
from third_party.motemplate import Motemplate
from url_constants import GITHUB_BASE, EXTENSIONS_SAMPLES


class TemplateRenderer(object):
  '''Renders templates with the server's available data sources.
  '''

  def __init__(self, server_instance):
    self._server_instance = server_instance

  def Render(self,
             template,
             request,
             data_sources=None,
             additional_context=None):
    '''Renders |template| using |request|.

    Specify |data_sources| to only include the DataSources with the given names
    when rendering the template.

    Specify |additional_context| to inject additional template context when
    rendering the template.
    '''
    assert isinstance(template, Motemplate), type(template)
    render_context = CreateDataSources(self._server_instance, request)
    if data_sources is not None:
      render_context = dict((name, d) for name, d in render_context.iteritems()
                            if name in data_sources)
    render_context.update({
      'apps_samples_url': GITHUB_BASE,
      'base_path': self._server_instance.base_path,
      'extensions_samples_url': EXTENSIONS_SAMPLES,
      'static': self._server_instance.base_path + 'static',
    })
    render_context.update(additional_context or {})
    render_data = template.Render(render_context)
    return render_data.text, render_data.errors
