# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import _winreg

import verifier


class RegistryVerifier(verifier.Verifier):
  """Verifies that the current registry matches the specified criteria."""

  _REGISTRY_VIEW_MAPPING = {
      'KEY_WOW64_32KEY': _winreg.KEY_WOW64_32KEY,
      'KEY_WOW64_64KEY': _winreg.KEY_WOW64_64KEY,
  }

  _ROOT_KEY_MAPPING = {
      'HKEY_CLASSES_ROOT': _winreg.HKEY_CLASSES_ROOT,
      'HKEY_CURRENT_USER': _winreg.HKEY_CURRENT_USER,
      'HKEY_LOCAL_MACHINE': _winreg.HKEY_LOCAL_MACHINE,
      'HKEY_USERS': _winreg.HKEY_USERS,
  }

  def _RootKeyConstant(self, root_key):
    """Converts a root registry key string into a _winreg.HKEY_* constant."""
    if root_key not in RegistryVerifier._ROOT_KEY_MAPPING:
      raise KeyError("Unknown root registry key '%s'" % root_key)
    return RegistryVerifier._ROOT_KEY_MAPPING[root_key]

  def _RegistryViewConstant(self, registry_view):
    """Converts a registry view string into a _winreg.KEY_WOW64* constant."""
    if registry_view not in RegistryVerifier._REGISTRY_VIEW_MAPPING:
      raise KeyError("Unknown registry view '%s'" % registry_view)
    return RegistryVerifier._REGISTRY_VIEW_MAPPING[registry_view]

  def _ValueTypeConstant(self, value_type):
    """Converts a registry value type string into a _winreg.REG_* constant."""
    value_type_mapping = {
        'BINARY': _winreg.REG_BINARY,
        'DWORD': _winreg.REG_DWORD,
        'DWORD_LITTLE_ENDIAN': _winreg.REG_DWORD_LITTLE_ENDIAN,
        'DWORD_BIG_ENDIAN': _winreg.REG_DWORD_BIG_ENDIAN,
        'EXPAND_SZ': _winreg.REG_EXPAND_SZ,
        'LINK': _winreg.REG_LINK,
        'MULTI_SZ': _winreg.REG_MULTI_SZ,
        'NONE': _winreg.REG_NONE,
        'SZ': _winreg.REG_SZ,
    }
    if value_type not in value_type_mapping:
      raise KeyError("Unknown registry value type '%s'" % value_type)
    return value_type_mapping[value_type]

  def _VerifyExpectation(self, expectation_name, expectation,
                         variable_expander):
    """Overridden from verifier.Verifier.

    Verifies a registry key according to the |expectation|.

    Args:
      expectation_name: The registry key being verified. It is expanded using
          Expand.
      expectation: A dictionary with the following keys and values:
          'exists' a string indicating whether the registry key's existence is
              'required', 'optional', or 'forbidden'. Values are not checked if
              an 'optional' key is not present in the registry.
          'values' (optional) a dictionary where each key is a registry value
              (which is expanded using Expand) and its associated value is a
              dictionary with the following key and values:
                  'type' (optional) a string indicating the type of the registry
                      value. If not present, the corresponding value is expected
                      to be absent in the registry.
                  'data' (optional) the associated data of the registry value if
                      'type' is specified. If it is a string, it is expanded
                      using Expand. If not present, only the value's type is
                      verified.
          'wow_key' (optional) a string indicating whether the view of the
              registry is KEY_WOW64_32KEY or KEY_WOW64_64KEY. If not present,
              the view of registry is determined by the bitness of the installer
              binary.
      variable_expander: A VariableExpander object.
    """
    key = variable_expander.Expand(expectation_name)
    root_key, sub_key = key.split('\\', 1)
    try:
      # Query the Windows registry for the registry key. It will throw a
      # WindowsError if the key doesn't exist.
      registry_view = _winreg.KEY_WOW64_32KEY
      if 'wow_key' in expectation:
        registry_view = self._RegistryViewConstant(expectation['wow_key'])
      elif variable_expander.Expand('$MINI_INSTALLER_BITNESS') == '64':
        registry_view = _winreg.KEY_WOW64_64KEY

      key_handle = _winreg.OpenKey(self._RootKeyConstant(root_key), sub_key, 0,
                                   _winreg.KEY_QUERY_VALUE | registry_view)
    except WindowsError:
      # Key doesn't exist. See that it matches the expectation.
      assert expectation['exists'] != 'required', ('Registry key %s is '
                                                   'missing' % key)
      # Values are not checked if the missing key's existence is optional.
      return
    # The key exists, see that it matches the expectation.
    assert expectation['exists'] != 'forbidden', ('Registry key %s exists' %
                                                  key)

    # Verify the expected values.
    if 'values' not in expectation:
      return
    for value, value_expectation in expectation['values'].iteritems():
      # Query the value. It will throw a WindowsError if the value doesn't
      # exist.
      value = variable_expander.Expand(value)
      try:
        data, value_type = _winreg.QueryValueEx(key_handle, value)
      except WindowsError:
        # The value does not exist. See that this matches the expectation.
        assert 'type' not in value_expectation, ('Value %s of registry key %s '
                                                 'is missing' % (value, key))
        continue

      assert 'type' in value_expectation, ('Value %s of registry key %s exists '
                                           'with value %s' % (value, key, data))

      # Verify the type of the value.
      expected_value_type = value_expectation['type']
      assert self._ValueTypeConstant(expected_value_type) == value_type, \
          "Value '%s' of registry key %s has unexpected type '%s'" % (
              value, key, expected_value_type)

      if 'data' not in value_expectation:
        return

      # Verify the associated data of the value.
      expected_data = value_expectation['data']
      if isinstance(expected_data, basestring):
        expected_data = variable_expander.Expand(expected_data)
      assert expected_data == data, \
          ("Value '%s' of registry key %s has unexpected data.\n"
           "  Expected: %s\n"
           "  Actual: %s" % (value, key, expected_data, data))
