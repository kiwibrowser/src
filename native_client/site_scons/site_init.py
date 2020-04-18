#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Software construction toolkit site_scons configuration.

This module sets up SCons for use with this toolkit.  This should contain setup
which occurs outside of environments.  If a method operates within the context
of an environment, it should instead go in a tool in site_tools and be invoked
for the target environment.
"""

import __builtin__
import sys
import SCons
import usage_log
import time

def CheckSConsLocation():
  """Check that the version of scons we are running lives in the native_client
  tree.

  Without this, if system scons is used then it produces rather cryptic error
  messages.
  """
  scons_location = os.path.dirname(os.path.abspath(SCons.__file__))
  nacl_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
  if not scons_location.startswith(nacl_dir):
    raise SCons.Errors.UserError('native_client must be built with its local '
                                 'version of SCons.\n  You are running SCons '
                                 'from %s' % scons_location)


def _HostPlatform():
  """Returns the current host platform.

  That is, the platform we're actually running SCons on.  You shouldn't use
  this inside your SConscript files; instead, include the appropriate
  target_platform tool for your environments.  When you call
  BuildEnvironments(), only environments with the current host platform will be
  built.  If for some reason you really need to examine the host platform,
  check env.Bit('host_windows') / env.Bit('host_linux') / env.Bit('host_mac').

  Returns:
    The host platform name - one of ('WINDOWS', 'LINUX', 'MAC').
  """

  platform_map = {
      'win32': 'WINDOWS',
      'cygwin': 'WINDOWS',
      'linux': 'LINUX',
      'linux2': 'LINUX',
      'linux3': 'LINUX',
      'darwin': 'MAC',
  }

  if sys.platform not in platform_map:
    print ('site_init.py warning: platform "%s" is not in platfom map.' %
           sys.platform)

  return platform_map.get(sys.platform, sys.platform)


def BuildEnvironmentSConscripts(env):
  """Evaluates SConscripts for the environment.

  Called by BuildEnvironments().
  """
  # Read SConscript for each component
  # TODO: Remove BUILD_COMPONENTS once all projects have transitioned to the
  # BUILD_SCONSCRIPTS nomenclature.
  for c in env.SubstList2('$BUILD_SCONSCRIPTS', '$BUILD_COMPONENTS'):
    # Clone the environment so components can't interfere with each other
    ec = env.Clone()

    if ec.Entry(c).isdir():
      # The component is a directory, so assume it contains a SConscript
      # file.
      c_dir = ec.Dir(c)

      # Use 'build.scons' as the default filename, but if that doesn't
      # exist, fall back to 'SConscript'.
      c_script = c_dir.File('build.scons')
      if not c_script.exists():
        c_script = c_dir.File('SConscript')
    else:
      # The component is a SConscript file.
      c_script = ec.File(c)
      c_dir = c_script.dir

    # Make c_dir a string.
    c_dir = str(c_dir)

    # Use build_dir differently depending on where the SConscript is.
    if not ec.RelativePath('$TARGET_ROOT', c_dir).startswith('..'):
      # The above expression means: if c_dir is $TARGET_ROOT or anything
      # under it. Going from c_dir to $TARGET_ROOT and dropping the not fails
      # to include $TARGET_ROOT.
      # We want to be able to allow people to use addRepository to back things
      # under $TARGET_ROOT/$OBJ_ROOT with things from above the current
      # directory. When we are passed a SConscript that is already under
      # $TARGET_ROOT, we should not use build_dir.
      start = time.clock()
      ec.SConscript(c_script, exports={'env': ec}, duplicate=0)
      if SCons.Script.ARGUMENTS.get('verbose'):
        print "[%5d] Loaded" %  (1000 * (time.clock() - start)), c_script

    elif not ec.RelativePath('$MAIN_DIR', c_dir).startswith('..'):
      # The above expression means: if c_dir is $MAIN_DIR or anything
      # under it. Going from c_dir to $TARGET_ROOT and dropping the not fails
      # to include $MAIN_DIR.
      # Also, if we are passed a SConscript that
      # is not under $MAIN_DIR, we should fail loudly, because it is unclear how
      # this will correspond to things under $OBJ_ROOT.
      start = time.clock()
      ec.SConscript(c_script, variant_dir='$OBJ_ROOT/' + c_dir,
                    exports={'env': ec}, duplicate=0)
      if SCons.Script.ARGUMENTS.get('verbose'):
        print "[%5d] Loaded" %  (1000 * (time.clock() - start)), c_script
    else:
      raise SCons.Errors.UserError(
          'Bad location for a SConscript. "%s" is not under '
          '\$TARGET_ROOT or \$MAIN_DIR' % c_script)

def FilterEnvironments(environments):
  """Filters out the environments to be actually build from the specified list

  Args:
    environments: List of SCons environments.

  Returns:
    List of environments which were matched
  """
  # Get options
  build_modes = SCons.Script.GetOption('build_mode')
  # TODO: Remove support legacy MODE= argument, once everyone has transitioned
  # to --mode.
  legacy_mode_option = SCons.Script.ARGUMENTS.get('MODE')
  if legacy_mode_option:
    build_modes = legacy_mode_option

  environment_map = dict((env['BUILD_TYPE'], env) for env in environments)

  # Add aliases for the host platform so that the caller of Scons does
  # not need to work out which platform they are running on.
  platform_map = {
      'win32': 'win',
      'cygwin': 'win',
      'linux': 'linux',
      'linux2': 'linux',
      'darwin': 'mac',
      }
  if sys.platform in platform_map:
    name = platform_map[sys.platform]
    environment_map['opt-host'] = environment_map['opt-%s' % name]
    environment_map['dbg-host'] = environment_map['dbg-%s' % name]
    environment_map['coverage-host'] = environment_map['coverage-%s' % name]

  matched_envs = []
  for mode in build_modes.split(','):
    if mode not in environment_map:
      raise SCons.Errors.UserError('Build mode "%s" is not defined' % mode)
    matched_envs.append(environment_map[mode])
  return matched_envs


def BuildEnvironments(environments):
  """Build a collection of SConscripts under a collection of environments.

  The environments are subject to filtering (c.f. FilterEnvironments)

  Args:
    environments: List of SCons environments.

  Returns:
    List of environments which were actually evaluated (built).
  """
  usage_log.log.AddEntry('BuildEnvironments start')
  for e in environments:
    # Make this the root environment for deferred functions, so they don't
    # execute until our call to ExecuteDefer().
    e.SetDeferRoot()

    # Defer building the SConscripts, so that other tools can do
    # per-environment setup first.
    e.Defer(BuildEnvironmentSConscripts)

    # Execute deferred functions
    e.ExecuteDefer()

  # Add help on targets.
  AddTargetHelp()

  usage_log.log.AddEntry('BuildEnvironments done')


#------------------------------------------------------------------------------


def _ToolExists():
  """Replacement for SCons tool module exists() function, if one isn't present.

  Returns:
    True.  This enables modules which always exist not to need to include a
        dummy exists() function.
  """
  return True


def _ToolModule(self):
  """Thunk for SCons.Tool.Tool._tool_module to patch in exists() function.

  Returns:
    The module from the original SCons.Tool.Tool._tool_module call, with an
        exists() method added if it wasn't present.
  """
  module = self._tool_module_orig()
  if not hasattr(module, 'exists'):
    module.exists = _ToolExists

  return module

#------------------------------------------------------------------------------


def AddSiteDir(site_dir):
  """Adds a site directory, as if passed to the --site-dir option.

  Args:
    site_dir: Site directory path to add, relative to the location of the
        SConstruct file.

  This may be called from the SConscript file to add a local site scons
  directory for a project.  This does the following:
     * Adds site_dir/site_scons to sys.path.
     * Imports site_dir/site_init.py.
     * Adds site_dir/site_scons to the SCons tools path.
  """
  # Call the same function that SCons does for the --site-dir option.
  SCons.Script.Main._load_site_scons_dir(
      SCons.Node.FS.get_default_fs().SConstruct_dir, site_dir)


#------------------------------------------------------------------------------


_new_options_help = '''
Additional options for SCons:

  --mode=MODE                 Specify build mode, e.g. "dbg-linux,nacl".
  --host-platform=PLATFORM    Force SCons to use PLATFORM as the host platform,
                              instead of the actual platform on which SCons is
                              run.  Useful for examining the dependency tree
                              which would be created, but not useful for
                              actually running the build because it'll attempt
                              to use the wrong tools for your actual platform.
  --site-path=DIRLIST         Comma-separated list of additional site
                              directory paths; each is processed as if passed
                              to --site-dir.
  --usage-log=FILE            Write XML usage log to FILE.
'''

def SiteInitMain():
  """Main code executed in site_init."""

  # Bail out if we've been here before. This is needed to handle the case where
  # this site_init.py has been dropped into a project directory.
  if hasattr(__builtin__, 'BuildEnvironments'):
    return

  CheckSConsLocation()

  usage_log.log.AddEntry('Software Construction Toolkit site init')

  # Let people use new global methods directly.
  __builtin__.AddSiteDir = AddSiteDir
  __builtin__.FilterEnvironments = FilterEnvironments
  __builtin__.BuildEnvironments = BuildEnvironments
  # Legacy method names
  # TODO: Remove these once they're no longer used anywhere.
  __builtin__.BuildComponents = BuildEnvironments

  # Set list of default tools for component_setup
  __builtin__.component_setup_tools = [
      # Defer must be first so other tools can register environment
      # setup/cleanup functions.
      'defer',
      # Component_targets must precede component_builders so builders can
      # define target groups.
      'component_targets',
      'command_output',
      'component_bits',
      'component_builders',
      'environment_tools',
      'publish',
      'replicate',
      'wix',
  ]

  # Patch Tool._tool_module method to fill in an exists() method for the
  # module if it isn't present.
  # TODO: This functionality should be patched into SCons itself by changing
  # Tool.__init__().
  SCons.Tool.Tool._tool_module_orig = SCons.Tool.Tool._tool_module
  SCons.Tool.Tool._tool_module = _ToolModule

  # Add our options
  SCons.Script.AddOption(
      '--mode', '--build-mode',
      dest='build_mode',
      nargs=1, type='string',
      action='store',
      metavar='MODE',
      default='opt-host,nacl',
      help='build mode(s)')
  SCons.Script.AddOption(
      '--host-platform',
      dest='host_platform',
      nargs=1, type='string',
      action='store',
      metavar='PLATFORM',
      help='build mode(s)')
  SCons.Script.AddOption(
      '--site-path',
      dest='site_path',
      nargs=1, type='string',
      action='store',
      metavar='PATH',
      help='comma-separated list of site directories')
  SCons.Script.AddOption(
      '--usage-log',
      dest='usage_log',
      nargs=1, type='string',
      action='store',
      metavar='PATH',
      help='file to write XML usage log to')

  SCons.Script.Help(_new_options_help)

  # Set up usage log
  usage_log_file = SCons.Script.GetOption('usage_log')
  if usage_log_file:
    usage_log.log.SetOutputFile(usage_log_file)

  # Set current host platform
  host_platform = SCons.Script.GetOption('host_platform')
  if not host_platform:
    host_platform = _HostPlatform()
  __builtin__.HOST_PLATFORM = host_platform

  # Check for site path.  This is a list of site directories which each are
  # processed as if they were passed to --site-dir.
  site_path = SCons.Script.GetOption('site_path')
  if site_path:
    for site_dir in site_path.split(','):
      AddSiteDir(site_dir)

  # Since our site dir was specified on the SCons command line, SCons will
  # normally only look at our site dir.  Add back checking for project-local
  # site_scons directories.
  if not SCons.Script.GetOption('no_site_dir'):
    SCons.Script.Main._load_site_scons_dir(
        SCons.Node.FS.get_default_fs().SConstruct_dir, None)


# Run main code
SiteInitMain()
