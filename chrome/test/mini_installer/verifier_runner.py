# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import file_verifier
import process_verifier
import registry_verifier


class VerifierRunner:
  """Runs all Verifiers."""

  def __init__(self):
    """Constructor."""
    # TODO(sukolsak): Implement other verifiers
    self._verifiers = {
      'Files': file_verifier.FileVerifier(),
      'Processes': process_verifier.ProcessVerifier(),
      'RegistryEntries': registry_verifier.RegistryVerifier(),
    }

  def VerifyAll(self, property, variable_expander):
    """Verifies that the current machine states match the property dictionary.

    A property dictionary is a dictionary where each key is a verifier's name
    and the associated value is the input to that verifier. For details about
    the input format for each verifier, take a look at http://goo.gl/1P85WL

    Args:
      property: A property dictionary.
      variable_expander: A VariableExpander object.
    """
    for verifier_name, verifier_input in property.iteritems():
      if verifier_name not in self._verifiers:
        raise KeyError('Unknown verifier %s' % verifier_name)
      self._verifiers[verifier_name].VerifyInput(verifier_input,
                                                 variable_expander)
