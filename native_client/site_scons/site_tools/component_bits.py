#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Environment bit support for software construction toolkit.

This module is automatically included by the component_setup tool.
"""


import __builtin__
import types
import SCons


_bit_descriptions = {}
_bits_with_options = set()
_bit_exclusive_groups = {}

#------------------------------------------------------------------------------


def _CheckDeclared(bits):
  """Checks each of the bits to make sure it's been declared.

  Args:
    bits: List of bits to check.

  Raises:
    ValueError: A bit has not been declared.
  """
  for bit in bits:
    if bit not in _bit_descriptions:
      raise ValueError('Bit "%s" used before DeclareBit()' %
                       bit)


def _CheckExclusive(already_set, proposed):
  """Checks if setting proposed bits would violate any exclusive groups.

  Args:
    already_set: List of bits already set.
    proposed: List of bits attempting to be set.

  Raises:
    ValueError: A proposed bit belongs to an exclusive group which already has
        a bit set.
  """
  # Remove any already-set bits from proposed (note this makes a copy of
  # proposed so we don't alter the passed list).
  proposed = [bit for bit in proposed if bit not in already_set]

  for group_name, group_bits in _bit_exclusive_groups.items():
    set_match = group_bits.intersection(already_set)
    proposed_match = group_bits.intersection(proposed)
    if set_match and proposed_match:
      raise ValueError('Unable to set bit "%s" because it belongs to the same '
                       'exclusive group "%s" as already-set bit "%s"' % (
                       proposed_match.pop(), group_name, set_match.pop()))


#------------------------------------------------------------------------------


def DeclareBit(bit_name, desc, exclusive_groups=None):
  """Declares and describes the bit.

  Args:
    bit_name: Name of the bit being described.
    desc: Description of bit.
    exclusive_groups: Bit groups which this bit belongs to.  At most one bit
        may be set in each exclusive group.  May be a string, list of string,
        or None.

  Raises:
    ValueError: The bit has already been defined with a different description,
        or the description is empty.

  Adds a description for the bit in the global dictionary of bit names.  All
  bits should be described before being used in Bit()/AllBits()/AnyBits().
  """

  if not desc:
    raise ValueError('Must supply a description for bit "%s"' % bit_name)

  existing_desc = _bit_descriptions.get(bit_name)
  if existing_desc and desc != existing_desc:
    raise ValueError('Cannot describe bit "%s" as "%s" because it has already'
                     'been described as "%s".' %
                     (bit_name, desc, existing_desc))

  _bit_descriptions[bit_name] = desc

  # Add bit to its exclusive groups
  if exclusive_groups:
    if type(exclusive_groups) == types.StringType:
      exclusive_groups = [exclusive_groups]
    for g in exclusive_groups:
      if g not in _bit_exclusive_groups:
        _bit_exclusive_groups[g] = set()
      _bit_exclusive_groups[g].add(bit_name)

#------------------------------------------------------------------------------

def Bit(env, bit_name):
  """Checks if the environment has the bit.

  Args:
    env: Environment to check.
    bit_name: Name of the bit to check.

  Returns:
    True if the bit is present in the environment.
  """
  _CheckDeclared([bit_name])
  return bit_name in env['_BITS']

#------------------------------------------------------------------------------


def AllBits(env, *args):
  """Checks if the environment has all the bits.

  Args:
    env: Environment to check.
    args: List of bit names to check.

  Returns:
    True if every bit listed is present in the environment.
  """
  _CheckDeclared(args)
  return set(args).issubset(env['_BITS'])

#------------------------------------------------------------------------------


def AnyBits(env, *args):
  """Checks if the environment has at least one of the bits.

  Args:
    env: Environment to check.
    args: List of bit names to check.

  Returns:
    True if at least one bit listed is present in the environment.
  """
  _CheckDeclared(args)
  return set(args).intersection(env['_BITS'])

#------------------------------------------------------------------------------


def SetBits(env, *args):
  """Sets the bits in the environment.

  Args:
    env: Environment to check.
    args: List of bit names to set.
  """
  _CheckDeclared(args)
  _CheckExclusive(env['_BITS'], args)
  env['_BITS'] = env['_BITS'].union(args)

#------------------------------------------------------------------------------


def ClearBits(env, *args):
  """Clears the bits in the environment.

  Args:
    env: Environment to check.
    args: List of bit names to clear (remove).
  """
  _CheckDeclared(args)
  env['_BITS'] = env['_BITS'].difference(args)

#------------------------------------------------------------------------------


def SetBitFromOption(env, bit_name, default):
  """Sets the bit in the environment from a command line option.

  Args:
    env: Environment to check.
    bit_name: Name of the bit to set from a command line option.
    default: Default value for bit if command line option is not present.
  """
  _CheckDeclared([bit_name])

  # Add the command line option, if not already present
  if bit_name not in _bits_with_options:
    _bits_with_options.add(bit_name)
    SCons.Script.AddOption('--' + bit_name,
                           dest=bit_name,
                           action='store_true',
                           help='set bit:' + _bit_descriptions[bit_name])
    SCons.Script.AddOption('--no-' + bit_name,
                           dest=bit_name,
                           action='store_false',
                           help='clear bit:' + _bit_descriptions[bit_name])

  bit_set = env.GetOption(bit_name)
  if bit_set is None:
    # Not specified on command line, so use default
    bit_set = default

  if bit_set:
    env['_BITS'].add(bit_name)
  elif bit_name in env['_BITS']:
    env['_BITS'].remove(bit_name)

#------------------------------------------------------------------------------


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  # Add methods to builtin
  __builtin__.DeclareBit = DeclareBit

  # Add methods to environment
  env.AddMethod(AllBits)
  env.AddMethod(AnyBits)
  env.AddMethod(Bit)
  env.AddMethod(ClearBits)
  env.AddMethod(SetBitFromOption)
  env.AddMethod(SetBits)

  env['_BITS'] = set()

  # Declare bits for common target platforms
  DeclareBit('linux', 'Target platform is linux.',
             exclusive_groups=('target_platform'))
  DeclareBit('mac', 'Target platform is mac.',
             exclusive_groups=('target_platform'))
  DeclareBit('windows', 'Target platform is windows.',
             exclusive_groups=('target_platform'))

  # Declare bits for common host platforms
  DeclareBit('host_linux', 'Host platform is linux.',
             exclusive_groups=('host_platform'))
  DeclareBit('host_mac', 'Host platform is mac.',
             exclusive_groups=('host_platform'))
  DeclareBit('host_windows', 'Host platform is windows.',
             exclusive_groups=('host_platform'))

  # Declare other common bits from target_ tools
  DeclareBit('debug', 'Build is debug, not optimized.')
  DeclareBit('posix', 'Target platform is posix.')

  # Set the appropriate host platform bit
  host_platform_to_bit = {
      'MAC': 'host_mac',
      'LINUX': 'host_linux',
      'WINDOWS': 'host_windows',
  }
  if HOST_PLATFORM in host_platform_to_bit:
    env.SetBits(host_platform_to_bit[HOST_PLATFORM])
