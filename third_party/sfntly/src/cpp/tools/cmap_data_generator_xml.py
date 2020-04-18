# Copyright 2011 Google Inc. All Rights Reserved.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limit

"""Class that generates CMap data from a font using fontTools.ttLib."""


class CMapDataGeneratorXML(object):
  """Generate CMap table data from an XML.

  Can set the number of CMaps in the XML description and
  the number of mappings from a CMap.
  """

  def __init__(self, wanted_cmap_ids, num_mappings):
    """CMapDataGeneratorXML constructor.

    Args:
        wanted_cmap_ids: List of (platform_id, encoding_id) tuples for the CMaps
        to be dumped.
        num_mappings: Number of mappings from the CMap that will be output. They
        are as evenly spread-out as possible. If num_mappings = n and
        total_mappings is t, every t/n-th mapping.'
    """
    self.wanted_cmap_ids = wanted_cmap_ids
    self.num_mappings = num_mappings

  def Generate(self, font, document, root_element):
    self.document = document
    self.root_element = root_element
    self.font = font
    table = self.font['cmap']
    root_element.setAttribute('num_cmaps', str(len(table.tables)))

    for cmap_id in self.wanted_cmap_ids:
      cmap = table.getcmap(cmap_id[0], cmap_id[1])
      if not cmap:
        continue
      cmap_element = document.createElement('cmap')
      root_element.appendChild(cmap_element)
      self.CMapInfoToXML(cmap_element, cmap)

  def CMapInfoToXML(self, cmap_element, cmap):
    """Converts a CMap info tuple to XML by added data to the cmap_element."""
    cmap_element.setAttribute('byte_length', str(cmap.length))
    cmap_element.setAttribute('format', str(cmap.format))
    cmap_element.setAttribute('platform_id', str(cmap.platformID))
    cmap_element.setAttribute('encoding_id', str(cmap.platEncID))
    cmap_element.setAttribute('num_chars', str(len(cmap.cmap)))
    cmap_element.setAttribute('language', str(cmap.language))
    cmap_entries = cmap.cmap.items()
    cmap_entries.sort()
    increment = len(cmap_entries) / self.num_mappings
    if increment == 0:
      increment = 1
    for i in range(0, len(cmap_entries), increment):
      entry = cmap_entries[i]
      mapping_element = self.document.createElement('map')
      cmap_element.appendChild(mapping_element)
      mapping_element.setAttribute('char', '0x%05x' % entry[0])
      mapping_element.setAttribute('glyph_name', entry[1])
      mapping_element.setAttribute('gid', str(self.font.getGlyphID(entry[1])))
