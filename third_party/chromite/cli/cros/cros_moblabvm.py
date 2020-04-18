# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cros moblabvm: Interact with moblab VM setups.

A typical moblab VM setup involves:
  - A cros VM running a moblab image.
  - A cros VM running another board image, to act as a sub-DUT for moblab.
  - Networking to allow these VMs to interact.
  - Special virtual disk used by the moblab VM for bootstrapping and for result
    collection; seeded with some configuration files.
  - [not implemented yet] Special image staging onto the moblab VM, so that it
    can be used to provision the sub-DUT VM.

See README.moblab_vm.md in this folder for usage examples.
"""

from __future__ import print_function

import os

from chromite.cli import command
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import moblab_vm
from chromite.lib import osutils


@command.CommandDecorator('moblabvm')
class MoblabvmCommand(command.CliCommand):
  """Manage a moblab VM setup."""

  EPILOG = u"""
A typical moblabvm session looks so:
  $ cros moblabvm --workspace /moblabvm/workspace create /moblab/image/ \
        --dut-image-dir /link/image/
  $ cros moblabvm --workspace /moblabvm/workspace start
  ...
  $ cros moblabvm --workspace /moblabvm/workspace describe
  ...
  moblabvm started. SSH port: 66443
  ...

  # Do something with the setup. Perhpas run a test.
  $ test_that 127.0.0.1:66443 moblab_SmokeSuite

  # Can start/stop as needed:
  $ cros moblabvm --workspace /moblabvm/workspace stop
  # or, directly
  $ cros moblabvm --workspace /moblabvm/workspace start --restart

  # When done, teardown everything.
  $ cros moblabvm --workspace /moblabvm/workspace destroy
  """

  @classmethod
  def AddParser(cls, parser):
    """Add moblabvm specific subcommands and options."""
    super(MoblabvmCommand, cls).AddParser(parser)
    parser.add_argument(
        '--workspace',
        required=True,
        help='Workspace directory used as the workspace for this moblabvm. '
             'Must be used consistently across moblabvm calls',
    )
    subparsers = parser.add_subparsers(title='moblabvm subcommands',
                                       dest='moblabvm_command')

    create_parser = _AddSubparser(parser, subparsers, 'create',
                                  'Create a new moblabvm setup.')
    create_parser.add_argument(
        'moblab_image_dir',
        help='Path to the directory containing moblab image. NOTE: This '
             'directory must contain all the files output by build_image.',
    )
    create_parser.add_argument(
        '--dut-image-dir',
        help='Path to the directory containing DUT image. NOTE: This directory '
             'must contain all the files output by build_image.',
    )
    create_parser.add_argument(
        '--source-vm-images',
        action='store_true',
        default=False,
        help='Copy already created VM images from source folders. '
             'Default behaviour is to convert the test images to VM images.',
    )
    create_parser.add_argument(
        '--clean',
        action='store_true',
        default=False,
        help='Destroy any previously setup moblabvm from the workspace. '
             'Default is False.',
    )

    start_parser = _AddSubparser(parser, subparsers, 'start',
                                 'Launch an existing moblabvm setup.')
    start_parser.add_argument(
        '--restart',
        action='store_true',
        default=False,
        help='Stop the VMs if they\'re already running. Default is False.',
    )

    _AddSubparser(parser, subparsers, 'stop',
                  'Stop a running moblabvm setup.')
    _AddSubparser(parser, subparsers, 'destroy',
                  'Teardown a moblabvm setup.')
    _AddSubparser(parser, subparsers, 'describe',
                  'Describe a running instance of the moblabvm setup. '
                  'Provides useful information for using the VMs.')

  def Run(self):
    """The main handler of this CLI."""
    self.options.Freeze()
    cmd = self.options.moblabvm_command
    if cmd == 'create':
      # Allow the workspace to not exist. This makes the CLI uniform across the
      # 'cros moblabvm' commands, without burdening the user with a mkdir before
      # 'create'
      osutils.SafeMakedirsNonRoot(self.options.workspace)

    if not os.path.isdir(self.options.workspace):
      cros_build_lib.Die('Workspace directory %s does not exist!',
                         self.options.workspace)

    vm = moblab_vm.MoblabVm(self.options.workspace)
    if cmd == 'create':
      if self.options.clean:
        vm.Destroy()
      vm.Create(self.options.moblab_image_dir, self.options.dut_image_dir,
                create_vm_images=not self.options.source_vm_images)
    elif cmd == 'start':
      if self.options.restart:
        vm.Stop()
      vm.Start()
      _Describe(vm)
    elif cmd == 'stop':
      vm.Stop()
    elif cmd == 'destroy':
      vm.Destroy()
    elif cmd == 'describe':
      _Describe(vm)
    else:
      assert False, 'Unrecognized command %s!' % cmd


def _AddSubparser(parser, subparsers, name, description):
  """Adds a subparser to the given parser, with common options.

  Forwards some options from the parser to the new subparser to ensure
  consistent formatting of output etc.

  Args:
    parser: The parent parser for this subparser.
    subparsers: The subparsers group to add this subparser to.
    name: Name of the new sub-command.
    description: Description to be used for the sub-command.

  Returns:
    The new subparser.
  """
  return subparsers.add_parser(
      name,
      description=description,
      caching=parser.caching,
      formatter_class=parser.formatter_class,
  )


def _Describe(vm):
  """Prints some information relevant to using the VMs."""
  if not vm.initialized:
    logging.notice('No MoblabVM setup found. You need to create one.')
    logging.notice('See "cros moblabvm --help"')
    return

  if not vm.running:
    logging.notice('MoblabVm is not running.')
    return

  logging.notice('MoblabVm is running.')
  logging.notice('Moblab VM information:')
  logging.notice('  SSH Port to connect from host: %s', vm.moblab_ssh_port)
  # TODO(pprabhu) Pull out VNC and AFE port forwarding from cros_vm_lib.sh so
  # that it can be reported better, and so that we don't need to special case
  # moblab in cros_vm_lib anymore.
  logging.notice('  MAC address of moblab-internal network in the VM: %s',
                 vm.moblab_internal_mac)

  if not vm.dut_running:
    logging.notice('There is no vm-DUT attached to moblab.')
    return
  logging.notice('sub-DUT information:')
  logging.notice('  SSH Port to connect from host: %s', vm.dut_ssh_port)
  logging.notice('  MAC address of moblab-internal network in the VM: %s',
                 vm.dut_internal_mac)
