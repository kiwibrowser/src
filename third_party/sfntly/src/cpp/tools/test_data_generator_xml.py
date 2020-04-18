# Copyright 2011 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limit

"""Main module, generates XML for all test files."""

import os
from font_data_generator_xml import FontDataGeneratorXML


class TestDataGeneratorXML(object):
  """Generates XML data for fonts using the provided |table_data_generators|."""

  def __init__(self, table_data_generators, fonts, destination):
    self.table_data_generators = table_data_generators
    self.fonts = fonts
    self.destination = destination

  def Generate(self):
    for font_path in self.fonts:
      print 'Processing font ' + font_path
      document = FontDataGeneratorXML(self.table_data_generators,
                                      font_path).Generate()
      xml_path = (font_path
                  if not self.destination else
                  self.destination + os.path.basename(font_path)) + '.xml'
      xml_file = open(xml_path, 'w')
      xml_file.write(document.toprettyxml().replace('\t', '  '))
      xml_file.close()
