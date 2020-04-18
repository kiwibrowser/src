# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for the command module."""

from __future__ import print_function

import argparse
import glob
import os

from chromite.lib import constants
from chromite.cli import command
from chromite.lib import commandline
from chromite.lib import cros_import
from chromite.lib import cros_test_lib
from chromite.lib import partial_mock


# pylint:disable=protected-access

_COMMAND_NAME = 'superAwesomeCommandOfFunness'


@command.CommandDecorator(_COMMAND_NAME)
class TestCommand(command.CliCommand):
  """A fake command."""
  def Run(self):
    print('Just testing')


class TestCommandTest(cros_test_lib.MockTestCase):
  """This test class tests that Commands method."""

  def testParserSetsCommandClass(self):
    """Tests that our parser sets command_class correctly."""
    my_parser = argparse.ArgumentParser()
    command.CliCommand.AddParser(my_parser)
    ns = my_parser.parse_args([])
    self.assertEqual(ns.command_class, command.CliCommand)

  def testCommandDecorator(self):
    """Tests that our decorator correctly adds TestCommand to _commands."""
    # Note this exposes an implementation detail of _commands.
    self.assertEqual(command._commands[_COMMAND_NAME], TestCommand)

  def testBadUseOfCommandDecorator(self):
    """Tests that our decorator correctly rejects bad test commands."""
    try:
      # pylint: disable=W0612
      @command.CommandDecorator('bad')
      class BadTestCommand(object):
        """A command that wasn't implemented correctly."""
        pass

    except command.InvalidCommandError:
      pass
    else:
      self.fail('Invalid command was accepted by the CommandDecorator')

  def testAddDeviceArgument(self):
    """Tests CliCommand.AddDeviceArgument()."""
    parser = argparse.ArgumentParser()
    command.CliCommand.AddDeviceArgument(parser)
    # Device should be a positional argument.
    parser.parse_args(['device'])


class MockCommand(partial_mock.PartialMock):
  """Mock class for a generic CLI command."""
  ATTRS = ('Run',)
  COMMAND = None
  TARGET_CLASS = None

  def __init__(self, args, base_args=None):
    partial_mock.PartialMock.__init__(self)
    self.args = args
    self.rc_mock = cros_test_lib.RunCommandMock()
    self.rc_mock.SetDefaultCmdResult()
    parser = commandline.ArgumentParser(caching=True)
    subparsers = parser.add_subparsers()
    subparser = subparsers.add_parser(self.COMMAND, caching=True)
    self.TARGET_CLASS.AddParser(subparser)

    args = base_args if base_args else []
    args += [self.COMMAND] + self.args
    options = parser.parse_args(args)
    self.inst = options.command_class(options)

  def Run(self, inst):
    with self.rc_mock:
      return self.backup['Run'](inst)


class CommandTest(cros_test_lib.MockTestCase):
  """This test class tests that we can load modules correctly."""

  # pylint: disable=W0212

  def testFindModules(self):
    """Tests that we can return modules correctly when mocking out glob."""
    fake_command_file = 'cros_command_test.py'
    filtered_file = 'cros_command_unittest.py'
    mydir = 'mydir'

    self.PatchObject(glob, 'glob',
                     return_value=[fake_command_file, filtered_file])

    self.assertEqual(command._FindModules(mydir), [fake_command_file])

  def testLoadCommands(self):
    """Tests import commands correctly."""
    fake_module = 'cros_command_test'
    fake_command_file = os.path.join(constants.CHROMITE_DIR, 'foo', fake_module)
    module_path = ['chromite', 'foo', fake_module]

    self.PatchObject(command, '_FindModules', return_value=[fake_command_file])
    # The code doesn't use the return value, so stub it out lazy-like.
    load_mock = self.PatchObject(cros_import, 'ImportModule', return_value=None)

    command._ImportCommands()

    load_mock.assert_called_with(module_path)

  def testListCrosCommands(self):
    """Tests we get a sane `cros` list back."""
    cros_commands = command.ListCommands()
    # Pick some commands that are likely to not go away.
    self.assertIn('chrome-sdk', cros_commands)
    self.assertIn('flash', cros_commands)
