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

"""Generate XML font test data using ttLib."""

import hashlib
import struct
import xml.dom.minidom as minidom
from fontTools import ttLib


class FontDataGeneratorXML(object):
  """Generates the XML description for a font."""

  def __init__(self, table_data_generators, font_path):
    self.table_data_generators = table_data_generators
    self.font_path = font_path

  def Generate(self):
    """Creates font a DOM with data for every table.

    Uses |table_data_generators| to plug in XML generators.

    Returns:
      A DOM ready for serialization.
    """
    doc = minidom.getDOMImplementation().createDocument(None,
                                                        'font_test_data', None)
    root_element = doc.documentElement
    # We need to set the path of the font as if in the root source directory
    # The assumption is that we have a '../' prefix
    root_element.setAttribute('path', self.font_path[3:])
    h = hashlib.new('sha1')
    h.update(open(self.font_path, 'r').read())
    root_element.setAttribute('sha1', h.hexdigest())
    font = ttLib.TTFont(self.font_path)
    # There is always a postscript name for Windows_BMP
    name_record = font['name'].getName(6, 3, 1)
    root_element.setAttribute('post_name',
                              self.Unpack(name_record.string))
    for (name, table_data_generator) in self.table_data_generators:
      name += '_table'
      table_element = doc.createElement(name)
      root_element.appendChild(table_element)
      table_data_generator.Generate(font, doc, table_element)
    return doc

  def Unpack(self, name):
    """Returns every other byte from name to comprensate for padding."""
    unpack_format = 'xc' * (len(name)/2)
    # This is string.join, which is deprecated :(
    return reduce(lambda a, h: a + h, struct.unpack(unpack_format, name), '')
