#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Software construction toolkit target management for SCons."""


import __builtin__
import SCons.Script


# Dict of target groups (TargetGroup indexed by group name)
__target_groups = {}

# Dict of targets (Target indexed by target name)
__targets = {}

# Dict of target modes (TargetMode indexed by mode name)
__target_modes = {}

#------------------------------------------------------------------------------


class TargetGroup(object):
  """Target group, as used by AddTargetGroup() and GetTargetGroups()."""

  def __init__(self, name, description):
    """Initializes the target group.

    Args:
      name: Name of the target group.
      description: Description of group.
    """
    self.name = name
    self.description = description

  def GetTargetNames(self):
    """Returns a list of target name strings for the group."""
    items = map(str, SCons.Script.Alias(self.name)[0].sources)
    # Remove duplicates from multiple environments
    return list(set(items))

#------------------------------------------------------------------------------


class TargetMode(object):
  """Target mode, as used by GetTargetModes()."""

  def __init__(self, name, description):
    """Initializes the target mode.

    Args:
      name: Name of the target mode.
      description: Description of mode.
    """
    self.name = name
    self.description = description

  def GetTargetNames(self):
    """Returns a list of target name strings for the group."""
    items = map(str, SCons.Script.Alias(self.name)[0].sources)
    # Remove duplicates from multiple environments
    return list(set(items))

#------------------------------------------------------------------------------


class Target(object):
  """Target object."""

  def __init__(self, name):
    """Initializes the target.

    Args:
      name: Name of the target.
    """
    self.name = name
    self.properties = {}        # Global properties
    self.mode_properties = {}   # Dict of modes to mode-specific properties

#------------------------------------------------------------------------------


def AddTargetGroup(name, description):
  """Adds a target group, used for printing help.

  Args:
    name: Name of target group.  This should be the name of an alias which
        points to other aliases for the specific targets.
    description: Description of the target group.  Should read properly when
        appended to 'The following ' - for example, 'programs can be built'.
  """

  # Warn if the target group already exists with a different description
  if (name in __target_groups
      and __target_groups[name].description != description):
    print ('Warning: Changing description of target group "%s" from "%s" to '
           '"%s"' % (name, __target_groups[name].description, description))
    __target_groups[name].description = description
  else:
    __target_groups[name] = TargetGroup(name, description)


def GetTargetGroups():
  """Gets the dict of target groups.

  Returns:
    The dict of target groups, indexed by group name.

  This dict is not fully populated until after BuildEnvironments() has been
  called.
  """
  return __target_groups


def GetTargetModes():
  """Gets the dict of target modes.

  Returns:
    The dict of target modes, indexed by mode name.

  This dict is not fully populated until after BuildEnvironments() has been
  called.
  """
  # TODO: Better to rename this to # GetTargetBuildEnvironments()?  That's a
  # more description name.
  return __target_modes


def GetTargets():
  """Gets the dict of targets.

  Returns:
    The dict of targets, indexed by target name.

  This dict is not fully populated until after BuildEnvironments() has been
  called.
  """
  return __targets


def SetTargetProperty(self, target_name, all_modes=False, **kwargs):
  """Sets one or more properties for a target.

  Args:
    self: Environment context.
    target_name: Name of the target.
    all_modes: If True, property applies to all modes.  If false, it applies
        only to the current mode (determined by self['BUILD_TYPE']).
    kwargs: Keyword args are used to set properties.  Properties will be
        converted to strings via env.subst().

  For example:
    foo_test = env.Program(...)[0]
    env.SetTargetProperty('foo_test', global=True, DESCRIPTION='Foo test')
    env.SetTargetProperty('foo_test', EXE=foo_test)
  """
  # Get the target
  if target_name not in __targets:
    __targets[target_name] = Target(target_name)
  target = __targets[target_name]

  if all_modes:
    add_to_dict = target.properties
  else:
    mode = self.get('BUILD_TYPE')
    if mode not in target.mode_properties:
      target.mode_properties[mode] = {}
    add_to_dict = target.mode_properties[mode]

  # Add values
  for k, v in kwargs.items():
    add_to_dict[k] = self.subst(str(v))


def AddTargetHelp():
  """Adds SCons help for the targets, groups, and modes.

  This is called automatically by BuildEnvironments()."""
  help_text = ''

  for group in GetTargetGroups().values():
    items = group.GetTargetNames()
    items.sort()
    if items:
      help_text += '\nThe following %s:' % group.description
      colwidth = max(map(len, items)) + 2
      cols = 77 / colwidth
      if cols < 1:
        cols = 1      # If target names are really long, one per line
      rows = (len(items) + cols - 1) / cols
      for row in range(0, rows):
        help_text += '\n  '
        for i in range(row, len(items), rows):
          help_text += '%-*s' % (colwidth, items[i])
      help_text += '\n  %s (do all of the above)\n' % group.name

  SCons.Script.Help(help_text)


def SetTargetDescription(self, target_name, description):
  """Convenience function to set a target's global DESCRIPTION property.

  Args:
    self: Environment context.
    target_name: Name of the target.
    description: Description of the target.
  """
  self.SetTargetProperty(target_name, all_modes=True, DESCRIPTION=description)


def AddTargetMode(env):
  """Adds the environment as a target mode.

  Args:
    env: Environment context.

  Called via env.Defer() for each build mode.
  """
  # Save the build mode and description
  mode = env.get('BUILD_TYPE')
  __target_modes[mode] = TargetMode(mode, env.get('BUILD_TYPE_DESCRIPTION'))


#------------------------------------------------------------------------------


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""
  env = env     # Silence gpylint

  __builtin__.AddTargetGroup = AddTargetGroup
  __builtin__.AddTargetHelp = AddTargetHelp
  __builtin__.GetTargetGroups = GetTargetGroups
  __builtin__.GetTargetModes = GetTargetModes
  __builtin__.GetTargets = GetTargets

  env.AddMethod(SetTargetDescription)
  env.AddMethod(SetTargetProperty)

  # Defer per-mode setup
  env.Defer(AddTargetMode)
