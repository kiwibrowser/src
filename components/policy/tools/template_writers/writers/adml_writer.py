#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from xml.dom import minidom
from writers import xml_formatted_writer
from writers.admx_writer import AdmxElementType
import json
import re


def GetWriter(config):
  '''Factory method for instanciating the ADMLWriter. Every Writer needs a
  GetWriter method because the TemplateFormatter uses this method to
  instantiate a Writer.
  '''
  return ADMLWriter(['win'], config)


class ADMLWriter(xml_formatted_writer.XMLFormattedWriter):
  ''' Class for generating an ADML policy template. It is used by the
  PolicyTemplateGenerator to write the ADML file.
  '''

  # DOM root node of the generated ADML document.
  _doc = None

  # The string-table contains all ADML "string" elements.
  _string_table_elem = None

  # The presentation-table is the container for presentation elements, that
  # describe the presentation of Policy-Groups and Policies.
  _presentation_table_elem = None

  def _AddString(self, id, text):
    ''' Adds an ADML "string" element to _string_table_elem. The following
    ADML snippet contains an example:

    <string id="$(id)">$(text)</string>

    Args:
      id: ID of the newly created "string" element.
      text: Value of the newly created "string" element.
    '''
    id = id.replace('.', '_')
    if id in self.strings_seen:
      assert text == self.strings_seen[id]
    else:
      self.strings_seen[id] = text
      string_elem = self.AddElement(
          self._string_table_elem, 'string', {'id': id})
      string_elem.appendChild(self._doc.createTextNode(text))

  def _GetAdmxElementType(self, policy):
    '''Returns the ADMX element type for a particular Policy.'''
    return AdmxElementType.GetType(policy, allow_multi_strings = False)

  def WritePolicy(self, policy):
    '''Generates the ADML elements for a Policy.
    <stringTable>
      ...
      <string id="$(policy_group_name)">$(caption)</string>
      <string id="$(policy_group_name)_Explain">$(description)</string>
    </stringTable>

    <presentationTables>
      ...
      <presentation id=$(policy_group_name)/>
    </presentationTables>

    Args:
      policy: The Policy to generate ADML elements for.
    '''
    policy_name = policy['name']
    policy_caption = policy.get('caption', policy_name)
    policy_label = policy.get('label', policy_name)

    policy_example = policy.get('example_value')
    policy_desc = policy.get('desc')
    example_value_text = self._GetExampleValueText(policy_example)

    if policy_desc is not None and example_value_text is not None:
      policy_explain = policy_desc + '\n\n' + example_value_text
    elif policy_desc is not None:
      policy_explain = policy_desc
    elif example_value_text is not None:
      policy_explain = example_value_text
    else:
      # No explanation found at all.
      policy_explain = policy_name

    self._AddString(policy_name, policy_caption)
    self._AddString(policy_name + '_Explain', policy_explain)
    presentation_elem = self.AddElement(
        self._presentation_table_elem, 'presentation', {'id': policy_name})

    admx_element_type = self._GetAdmxElementType(policy)
    if admx_element_type == AdmxElementType.MAIN:
      pass
    elif admx_element_type == AdmxElementType.STRING:
      textbox_elem = self.AddElement(presentation_elem, 'textBox',
                                     {'refId': policy_name})
      label_elem = self.AddElement(textbox_elem, 'label')
      label_elem.appendChild(self._doc.createTextNode(policy_label))
    elif admx_element_type == AdmxElementType.MULTI_STRING:
      # We currently also show a single-line textbox - see http://crbug/829328
      textbox_elem = self.AddElement(presentation_elem, 'textBox',
                                     {'refId': policy_name + '_Legacy'})
      label_elem = self.AddElement(textbox_elem, 'label')
      legacy_label = self._GetLegacySingleLineLabel(policy_label)
      self._AddString(policy_name + '_Legacy', legacy_label)
      label_elem.appendChild(self._doc.createTextNode(legacy_label))
      # New multi-line textbox, easier to use than old single-line textbox:
      multitextbox_elem = self.AddElement(presentation_elem, 'multiTextBox',
          {'refId': policy_name, 'defaultHeight': '8'})
      multitextbox_elem.appendChild(self._doc.createTextNode(policy_label))
    elif admx_element_type == AdmxElementType.INT:
      textbox_elem = self.AddElement(presentation_elem, 'decimalTextBox',
                                     {'refId': policy_name})
      textbox_elem.appendChild(self._doc.createTextNode(policy_label + ':'))
    elif admx_element_type == AdmxElementType.ENUM:
      for item in policy['items']:
        self._AddString(policy_name + "_" + item['name'], item['caption'])
      dropdownlist_elem = self.AddElement(presentation_elem, 'dropdownList',
                                          {'refId': policy_name})
      dropdownlist_elem.appendChild(self._doc.createTextNode(policy_label))
    elif admx_element_type == AdmxElementType.LIST:
      self._AddString(policy_name + 'Desc', policy_caption)
      listbox_elem = self.AddElement(presentation_elem, 'listBox',
                                     {'refId': policy_name + 'Desc'})
      listbox_elem.appendChild(self._doc.createTextNode(policy_label))
    elif admx_element_type == AdmxElementType.GROUP:
      pass
    else:
      raise Exception('Unknown element type %s.' % admx_element_type)

  def BeginPolicyGroup(self, group):
    '''Generates ADML elements for a Policy-Group. For each Policy-Group two
    ADML "string" elements are added to the string-table. One contains the
    caption of the Policy-Group and the other a description. A Policy-Group also
    requires an ADML "presentation" element that must be added to the
    presentation-table. The "presentation" element is the container for the
    elements that define the visual presentation of the Policy-Goup's Policies.
    The following ADML snippet shows an example:

    Args:
      group: The Policy-Group to generate ADML elements for.
    '''
    # Add ADML "string" elements to the string-table that are required by a
    # Policy-Group.
    self._AddString(group['name'] + '_group', group['caption'])

  def _AddBaseStrings(self):
    ''' Adds ADML "string" elements to the string-table that are referenced by
    the ADMX file but not related to any specific Policy-Group or Policy.
    '''
    self._AddString(self.config['win_supported_os'],
                    self.messages['win_supported_win7']['text'])
    categories = self.winconfig['mandatory_category_path'] + \
                  self.winconfig['recommended_category_path']
    strings = self.winconfig['category_path_strings']
    for category in categories:
      if (category in strings):
        # Replace {...} by localized messages.
        string = re.sub(r"\{(\w+)\}", \
                        lambda m: self.messages[m.group(1)]['text'], \
                        strings[category])
        self._AddString(category, string)

  def _GetExampleValueText(self, example_value):
    '''Generates a string that describes the example value, if needed.
    Returns None if no string is needed. For instance, if the setting is a
    boolean, the user can only select true or false, so example text is not
    useful.'''
    if isinstance(example_value, str):
      return self._GetLocalizedMessage('example_value') + ' ' + example_value
    if isinstance(example_value, list):
      value_as_text = '\n'.join([str(v) for v in example_value])
      return self._GetLocalizedMessage('example_value') + '\n\n' + value_as_text
    if isinstance(example_value, dict):
      value_as_text = json.dumps(example_value)
      return self._GetLocalizedMessage('example_value') + '\n\n' + value_as_text
    return None

  def _GetLegacySingleLineLabel(self, policy_label):
    '''Generates a label for a legacy single-line textbox.'''
    return (self._GetLocalizedMessage('legacy_single_line_label')
        .replace('$6', policy_label))

  def _GetLocalizedMessage(self, msg_id):
    '''Returns the localized message of the given message ID.'''
    return self.messages['doc_' + msg_id]['text']

  def BeginTemplate(self):
    dom_impl = minidom.getDOMImplementation('')
    self._doc = dom_impl.createDocument(None, 'policyDefinitionResources',
                                        None)
    if self._GetChromiumVersionString() is not None:
      self.AddComment(self._doc.documentElement, self.config['build'] + \
          ' version: ' + self._GetChromiumVersionString())
    policy_definitions_resources_elem = self._doc.documentElement
    policy_definitions_resources_elem.attributes['revision'] = '1.0'
    policy_definitions_resources_elem.attributes['schemaVersion'] = '1.0'

    self.AddElement(policy_definitions_resources_elem, 'displayName')
    self.AddElement(policy_definitions_resources_elem, 'description')
    resources_elem = self.AddElement(policy_definitions_resources_elem,
                                     'resources')
    self._string_table_elem = self.AddElement(resources_elem, 'stringTable')
    self._AddBaseStrings()
    self._presentation_table_elem = self.AddElement(resources_elem,
                                                   'presentationTable')

  def Init(self):
    # Map of all strings seen.
    self.strings_seen = {}
    # Shortcut to platform-specific ADMX/ADM specific configuration.
    assert len(self.platforms) == 1
    self.winconfig = self.config['win_config'][self.platforms[0]]

  def GetTemplateText(self):
    # Using "toprettyxml()" confuses the Windows Group Policy Editor
    # (gpedit.msc) because it interprets whitespace characters in text between
    # the "string" tags. This prevents gpedit.msc from displaying the category
    # names correctly.
    # TODO(markusheintz): Find a better formatting that works with gpedit.
    return self._doc.toxml()


