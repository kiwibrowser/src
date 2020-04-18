# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from extensions_paths import PRIVATE_TEMPLATES
from file_system import FileNotFoundError


class TableOfContentsRenderer(object):
  '''Renders a table of contents pulled from a list of DocumentSections
  returned from document_parser.ParseDocument.

  This performs key functionality of DocumentRenderer, pulled into its own
  class for testability.
  '''

  def __init__(self,
               host_file_system,
               compiled_fs_factory,
               template_renderer):
    self._templates = compiled_fs_factory.ForTemplates(host_file_system)
    self._template_renderer = template_renderer

  def Render(self, sections):
    '''Renders a list of DocumentSections |sections| and returns a tuple
    (text, warnings).
    '''
    path = '%stable_of_contents.html' % PRIVATE_TEMPLATES
    try:
      table_of_contents_template = self._templates.GetFromFile(path).Get()
    except FileNotFoundError:
      return '', ['%s not found' % path]

    def make_toc_items(entries):
      return [{
        'attributes':  [{'key': key, 'value': val}
                        for key, val in entry.attributes.iteritems()
                        if key != 'id'],
        'link':        entry.attributes.get('id', ''),
        'subheadings': make_toc_items(entry.entries),
        'title':       entry.name,
      } for entry in entries]

    toc_items = []
    for section in sections:
      items_for_section = make_toc_items(section.structure)
      if toc_items and items_for_section:
        items_for_section[0]['separator'] = True
      toc_items.extend(items_for_section)

    return self._template_renderer.Render(
        self._templates.GetFromFile(
            '%stable_of_contents.html' % PRIVATE_TEMPLATES).Get(),
        None,  # no request
        data_sources=('partials'),
        additional_context={'items': toc_items})
