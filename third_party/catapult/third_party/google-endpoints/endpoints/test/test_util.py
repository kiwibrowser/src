# Copyright 2016 Google Inc. All Rights Reserved.
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
# limitations under the License.

"""Test utilities for API modules.

Classes:
  ModuleInterfaceTest: Test framework for developing public modules.
"""

# pylint: disable=g-bad-name

import __future__
import json
import os
import types


def AssertDictEqual(expected, actual, testcase):
  """Utility method to dump diffs if the dictionaries aren't equal.

  Args:
    expected: dict, the expected results.
    actual: dict, the actual results.
    testcase: unittest.TestCase, the test case this assertion is used within.
  """
  if expected != actual:
    testcase.assertMultiLineEqual(
        json.dumps(expected, indent=2, sort_keys=True),
        json.dumps(actual, indent=2, sort_keys=True))


class ModuleInterfaceTest(object):
  r"""Test to ensure module interface is carefully constructed.

  A module interface is the set of public objects listed in the module __all__
  attribute.  Modules that will be used by the public should have this interface
  carefully declared.  At all times, the __all__ attribute should have objects
  intended to be used by the public and other objects in the module should be
  considered unused.

  Protected attributes (those beginning with '_') and other imported modules
  should not be part of this set of variables.  An exception is for variables
  that begin and end with '__' which are implicitly part of the interface
  (eg. __name__, __file__, __all__ itself, etc.).

  Modules that are imported in to the tested modules are an exception and may
  be left out of the __all__ definition. The test is done by checking the value
  of what would otherwise be a public name and not allowing it to be exported
  if it is an instance of a module.  Modules that are explicitly exported are
  for the time being not permitted.

  To use this test class a module should define a new class that inherits first
  from ModuleInterfaceTest and then from unittest.TestCase.  No other tests
  should be added to this test case, making the order of inheritance less
  important, but if setUp for some reason is overidden, it is important that
  ModuleInterfaceTest is first in the list so that its setUp method is
  invoked.

  Multiple inheretance is required so that ModuleInterfaceTest is not itself
  a test, and is not itself executed as one.

  The test class is expected to have the following class attributes defined:

    MODULE: A reference to the module that is being validated for interface
      correctness.

  Example:
    Module definition (hello.py):

      import sys

      __all__ = ['hello']

      def _get_outputter():
        return sys.stdout

      def hello():
        _get_outputter().write('Hello\n')

    Test definition:

      import test_util
      import unittest

      import hello

      class ModuleInterfaceTest(module_testutil.ModuleInterfaceTest,
                                unittest.TestCase):

        MODULE = hello


      class HelloTest(unittest.TestCase):
        ... Test 'hello' module ...


      def main(unused_argv):
        unittest.main()


      if __name__ == '__main__':
        app.run()
  """

  def setUp(self):
    """Set up makes sure that MODULE and IMPORTED_MODULES is defined.

    This is a basic configuration test for the test itself so does not
    get it's own test case.
    """
    if not hasattr(self, 'MODULE'):
      self.fail(
          "You must define 'MODULE' on ModuleInterfaceTest sub-class %s." %
          type(self).__name__)

  def testAllExist(self):
    """Test that all attributes defined in __all__ exist."""
    missing_attributes = []
    for attribute in self.MODULE.__all__:
      if not hasattr(self.MODULE, attribute):
        missing_attributes.append(attribute)
    if missing_attributes:
      self.fail('%s of __all__ are not defined in module.' %
                missing_attributes)

  def testAllExported(self):
    """Test that all public attributes not imported are in __all__."""
    missing_attributes = []
    for attribute in dir(self.MODULE):
      if not attribute.startswith('_'):
        if attribute not in self.MODULE.__all__:
          attribute_value = getattr(self.MODULE, attribute)
          if isinstance(attribute_value, types.ModuleType):
            continue
          # pylint: disable=protected-access
          if isinstance(attribute_value, __future__._Feature):
            continue
          missing_attributes.append(attribute)
    if missing_attributes:
      self.fail('%s are not modules and not defined in __all__.' %
                missing_attributes)

  def testNoExportedProtectedVariables(self):
    """Test that there are no protected variables listed in __all__."""
    protected_variables = []
    for attribute in self.MODULE.__all__:
      if attribute.startswith('_'):
        protected_variables.append(attribute)
    if protected_variables:
      self.fail('%s are protected variables and may not be exported.' %
                protected_variables)

  def testNoExportedModules(self):
    """Test that no modules exist in __all__."""
    exported_modules = []
    for attribute in self.MODULE.__all__:
      try:
        value = getattr(self.MODULE, attribute)
      except AttributeError:
        # This is a different error case tested for in testAllExist.
        pass
      else:
        if isinstance(value, types.ModuleType):
          exported_modules.append(attribute)
    if exported_modules:
      self.fail('%s are modules and may not be exported.' % exported_modules)


class DevServerTest(object):

  @staticmethod
  def setUpDevServerEnv(server_software_key='SERVER_SOFTWARE',
                        server_software_value='Development/2.0.0'):
    original_env_value = os.environ.get(server_software_key)
    os.environ[server_software_key] = server_software_value
    return server_software_key, original_env_value

  @staticmethod
  def restoreEnv(server_software_key, server_software_value):
    if server_software_value is None:
      os.environ.pop(server_software_key, None)
    else:
      os.environ[server_software_key] = server_software_value

