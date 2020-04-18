# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .callback_function import CallbackFunction
from .callback_interface import CallbackInterface
from .dictionary import Dictionary
from .enumeration import Enumeration
from .implements import Implements
from .interface import Interface
from .namespace import Namespace
from .typedef import Typedef


class Collection(object):
    """
    Collection class stores Web IDL definitions and some meta information.
    """

    def __init__(self, component=None):
        self._interfaces = {}
        self._callback_interfaces = {}
        self._namespaces = {}
        self._dictionaries = {}
        self._enumerations = {}
        self._callback_functions = {}
        self._typedefs = {}
        # In spec, different partial definitions can have same identifiers.
        # So they are stored in a list, which is indexed by the identifier.
        # i.e. {'identifer': [definition, definition, ...]}
        self._partial_interfaces = {}
        self._partial_namespaces = {}
        self._partial_dictionaries = {}
        # Implements statements are not named definitions.
        self._implements = []
        # These members are not in spec., but they are necessary for code generators.
        self._metadata_store = []
        self._component = component

    def find_non_partial_definition(self, identifier):
        """Returns a non-partial named definition, if it is defined. Otherwise returns None."""
        if identifier in self._interfaces:
            return self._interfaces[identifier]
        if identifier in self._callback_interfaces:
            return self._callback_interfaces[identifier]
        if identifier in self._namespaces:
            return self._namespaces[identifier]
        if identifier in self._dictionaries:
            return self._dictionaries[identifier]
        if identifier in self._enumerations:
            return self._enumerations[identifier]
        if identifier in self._callback_functions:
            return self._callback_functions[identifier]
        if identifier in self._typedefs:
            return self._typedefs[identifier]
        return None

    def find_partial_definition(self, identifier):
        if identifier in self._partial_interfaces:
            return self._partial_interfaces[identifier]
        if identifier in self._partial_namespaces:
            return self._partial_namespaces[identifier]
        if identifier in self._partial_dictionaries:
            return self._partial_dictionaries[identifier]
        return []

    def find_filepath(self, definition):
        for metadata in self._metadata_store:
            if metadata.definition == definition:
                return metadata.filepath
        return None

    def register_definition(self, definition, filepath):
        if type(definition) == Interface:
            if definition.is_partial:
                self._register_partial_definition(self._partial_interfaces, definition)
            else:
                self._register_definition(self._interfaces, definition)
        elif type(definition) == Namespace:
            if definition.is_partial:
                self._register_partial_definition(self._partial_namespaces, definition)
            else:
                self._register_definition(self._namespaces, definition)
        elif type(definition) == Dictionary:
            if definition.is_partial:
                self._register_partial_definition(self._partial_dictionaries, definition)
            else:
                self._register_definition(self._dictionaries, definition)
        elif type(definition) == CallbackInterface:
            self._register_definition(self._callback_interfaces, definition)
        elif type(definition) == Enumeration:
            self._register_definition(self._enumerations, definition)
        elif type(definition) == Typedef:
            self._register_definition(self._typedefs, definition)
        elif type(definition) == CallbackFunction:
            self._register_definition(self._callback_functions, definition)
        elif type(definition) == Implements:
            self._implements.append(definition)
        else:
            raise ValueError('Unrecognized class definition %s in %s' % (str(type(definition)), filepath))

        metadata = _Metadata(definition, filepath)
        self._metadata_store.append(metadata)

    @property
    def interface_identifiers(self):
        return self._interfaces.keys()

    @property
    def callback_interface_identifiers(self):
        return self._callback_interfaces.keys()

    @property
    def namespace_identifiers(self):
        return self._namespaces.keys()

    @property
    def dictionary_identifiers(self):
        return self._dictionaries.keys()

    @property
    def enumeration_identifiers(self):
        return self._enumerations.keys()

    @property
    def callback_function_identifiers(self):
        return self._callback_functions.keys()

    @property
    def typedef_identifiers(self):
        return self._typedefs.keys()

    @property
    def implements(self):
        return self._implements

    @property
    def component(self):
        return self._component

    def _register_definition(self, definitions, definition):
        identifier = definition.identifier
        previous_definition = self.find_non_partial_definition(identifier)
        if previous_definition and previous_definition == definition:
            raise ValueError('Conflict: %s is defined in %s and %s' %
                             (identifier, self.find_filepath(previous_definition),
                              self.find_filepath(definition)))
        definitions[identifier] = definition

    def _register_partial_definition(self, definitions, definition):
        identifier = definition.identifier
        if identifier not in definitions:
            definitions[identifier] = []
        definitions[identifier].append(definition)


class _Metadata(object):
    """Metadata holds information of a definition which is not in spec.
    - |filepath| shows the .idl file name where the definition is described.
    """
    def __init__(self, definition, filepath):
        self._definition = definition
        self._filepath = filepath
