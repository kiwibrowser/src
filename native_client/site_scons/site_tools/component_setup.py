#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Main setup for software construction toolkit.

This module is a SCons tool which should be include in all environments.
It is used as follows:
  env = Environment(tools = ['component_setup'])
and should be the first tool from this toolkit referenced by any environment.
"""

import os
import sys
import SCons
import usage_log


#------------------------------------------------------------------------------


def InstallUsingLink(target, source, env):
  """Install function for environment which uses link in preference to copy.

  Args:
    target: Destintion filename
    source: Source filename
    env: Environment

  Returns:
    Return code from SCons Node link function.
  """

  # Use link function for Install() and InstallAs(), since it's much much
  # faster than copying.  This is ok for the way we build clients, where we're
  # installing to a build output directory and not to a permanent location such
  # as /usr/bin.
  # Need to force the target and source to be lists of nodes
  return SCons.Node.FS.LinkFunc([env.Entry(target)], [env.Entry(source)], env)


def PreEvaluateVariables(env):
  """Deferred function to pre-evaluate SCons varables for each build mode.

  Args:
    env: Environment for the current build mode.
  """
  # Convert directory variables to strings.  Must use .abspath not str(), since
  # otherwise $OBJ_ROOT is converted to a relative path, which evaluates
  # improperly in SConscripts not in $MAIN_DIR.
  for var in env.SubstList2('$PRE_EVALUATE_DIRS'):
    env[var] = env.Dir('$' + var).abspath


#------------------------------------------------------------------------------


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  # Use MD5 to tell when files differ, if the timestamps differ.  This is
  # better than pure MD5 (since if the timestamps are the same, we don't need
  # to rescan the file), and also better than pure timestamp (since if a file
  # is rebuilt to the same contents, we don't need to trigger the build steps
  # which depend on it).
  env.Decider('MD5-timestamp')

  # For duplication order, use hard links then fall back to copying.  Don't use
  # soft links, since those won't do the right thing if the output directory
  # is tar'd up and moved elsewhere.
  SCons.Script.SetOption('duplicate', 'hard-copy')

  # Remove the alias namespace lookup function from the list which SCons uses
  # when coercing strings into nodes.  This prevents SCons from looking up
  # aliases in input/output lists if they're not explicitly coerced via
  # Alias(), and removes a conflict where a program has the same shorthand
  # alias as the program name itself.  This conflict manifests itself as a
  # python exception if you try to build a program in multiple modes on linux,
  # for example:
  #      hammer --mode=dbg,opt port_test
  new_lookup_list = []
  for func in env.lookup_list:
    if func.im_class != SCons.Node.Alias.AliasNameSpace:
      new_lookup_list.append(func)
  env.lookup_list = new_lookup_list

  # Add command line options
  SCons.Script.AddOption(
      '--brief',
      dest='brief_comstr',
      default=True,
      action='store_true',
      help='brief command line output')
  SCons.Script.AddOption(
      '--verbose',
      dest='brief_comstr',
      default=True,
      action='store_false',
      help='verbose command line output')

  # Add help for command line options
  SCons.Script.Help("""\
  --verbose                   Print verbose output while building, including
                              the full command lines for all commands.
  --brief                     Print brief output while building (the default).
                              This and --verbose are opposites.  Use --silent
                              to turn off all output.
""")

  # Cover part of the environment
  env.Replace(
      # Add a reference to our python executable, so subprocesses can find and
      # invoke python.
      PYTHON = env.File(sys.executable),

      # Get the absolute path to the directory containing main.scons (or
      # SConstruct).  This should be used in place of the SCons variable '#',
      # since '#' is not always replaced (for example, when being used to set
      # an environment variable).
      MAIN_DIR = env.Dir('#').abspath,
      # Supply deprecated SCONSTRUCT_DIR for legacy suport
      # TODO: remove legacy support once everyone has switched over.
      SCONSTRUCT_DIR = env.Dir('#').abspath,

      # Use install function above, which uses links in preference to copying.
      INSTALL = InstallUsingLink,
  )

  # Specify defaults for variables where we don't need to force replacement
  env.SetDefault(
      # Directories
      DESTINATION_ROOT='$MAIN_DIR/scons-out$HOST_PLATFORM_SUFFIX',
      TARGET_ROOT='$DESTINATION_ROOT/$BUILD_TYPE',
      OBJ_ROOT='$TARGET_ROOT/obj',
      ARTIFACTS_DIR='$TARGET_ROOT/artifacts',
  )

  # Add default list of variables we should pre-evaluate for each build mode
  env.Append(PRE_EVALUATE_DIRS = [
      'ARTIFACTS_DIR',
      'DESTINATION_ROOT',
      'OBJ_ROOT',
      'SOURCE_ROOT',
      'TOOL_ROOT',
  ])

  # If a host platform was specified on the command line, need to put the SCons
  # output in its own destination directory.
  force_host_platform = SCons.Script.GetOption('host_platform')
  if force_host_platform:
    env['HOST_PLATFORM_SUFFIX'] = '-' + force_host_platform

  # Put the .sconsign.dblite file in our destination root directory, so that we
  # don't pollute the source tree. Use the '_' + sys.platform suffix to prevent
  # the .sconsign.dblite from being shared between host platforms, even in the
  # case where the --host_platform option is not used (for instance when the
  # project has platform suffixes on all the build types).
  #
  # This will prevent host platforms from mistakenly using each other's
  # .sconsign databases and will allow two host platform builds to occur in the
  # same # shared tree simulataneously.
  #
  # Note that we use sys.platform here rather than HOST_PLATFORM, since we need
  # different sconsign databases for cygwin vs. win32.
  sconsign_dir = env.Dir('$DESTINATION_ROOT').abspath
  sconsign_filename = '$DESTINATION_ROOT/.sconsign_%s' % sys.platform
  sconsign_file = env.File(sconsign_filename).abspath
  # SConsignFile() doesn't seem to like it if the destination directory
  # doesn't already exist, so make sure it exists.
  # TODO: Remove once SCons has fixed this bug.
  if not os.path.isdir(sconsign_dir):
    os.makedirs(sconsign_dir)
  SCons.Script.SConsignFile(sconsign_file)

  # Build all by default
  # TODO: This would be more nicely done by creating an 'all' alias and mapping
  # that to $DESTINATION_ROOT (or the accumulation of all $TARGET_ROOT's for
  # the environments which apply to the current host platform).  Ideally, that
  # would be done in site_init.py and not here.  But since we can't do that,
  # just set the default to be DESTINATION_ROOT here.  Note that this currently
  # forces projects which want to override the default to do so after including
  # the component_setup tool (reasonable, since component_setup should pretty
  # much be the first thing in a SConstruct.
  env.Default('$DESTINATION_ROOT')

  # Use brief command line strings if necessary.
  # Since these get passed to PRINT_CMD_LINE_FUNC, which scons_to_ninja
  # relies on, don't do this if generate_ninja is enabled.
  if (env.GetOption('brief_comstr') and
      'generate_ninja' not in SCons.Script.ARGUMENTS):
    env.SetDefault(
        ARCOMSTR='________Creating library $TARGET',
        ASCOMSTR='________Assembling $TARGET',
        CCCOMSTR='________Compiling $TARGET',
        CXXCOMSTR='________Compiling $TARGET',
        LDMODULECOMSTR='________Building loadable module $TARGET',
        LINKCOMSTR='________Linking $TARGET',
        MANIFEST_COMSTR='________Updating manifest for $TARGET',
        MIDLCOMSTR='________Compiling IDL $TARGET',
        PCHCOMSTR='________Precompiling $TARGET',
        RANLIBCOMSTR='________Indexing $TARGET',
        RCCOMSTR='________Compiling resource $TARGET',
        SHCCCOMSTR='________Compiling $TARGET',
        SHCXXCOMSTR='________Compiling $TARGET',
        SHLINKCOMSTR='________Linking $TARGET',
        SHMANIFEST_COMSTR='________Updating manifest for $TARGET',
        # Strip doesn't seem to be a first-class citizen in SCons country,
        # so we have to add these *COM, *COMSTR manually.
        STRIPCOMSTR='________Stripping to create $TARGET',
        TRANSLATECOMSTR='________Translating $TARGET',
        PNACLFINALIZECOMSTR='________Finalizing pexe $TARGET',
    )

  # Add other default tools from our toolkit
  # TODO: Currently this needs to be before SOURCE_ROOT in case a tool needs to
  # redefine it.  Need a better way to handle order-dependency in tool setup.
  for t in component_setup_tools:
    env.Tool(t)

  # The following environment replacements use env.Dir() to force immediate
  # evaluation/substitution of SCons variables.  They can't be part of the
  # preceding env.Replace() since they they may rely indirectly on variables
  # defined there, and the env.Dir() calls would be evaluated before the
  # env.Replace().

  # Set default SOURCE_ROOT if there is none, assuming we're in a local
  # site_scons directory for the project.
  source_root_relative = os.path.normpath(
      os.path.join(os.path.dirname(__file__), '../..'))
  source_root = env.get('SOURCE_ROOT', source_root_relative)
  env['SOURCE_ROOT'] = env.Dir(source_root).abspath

  usage_log.log.SetParam('component_setup.project_path',
                         env.RelativePath('$SOURCE_ROOT', '$MAIN_DIR'))

  # Make tool root separate from source root so it can be overridden when we
  # have a common location for tools outside of the current clientspec.  Need
  # to check if it's defined already, so it can be set prior to this tool
  # being included.
  tool_root = env.get('TOOL_ROOT', '$SOURCE_ROOT')
  env['TOOL_ROOT'] = env.Dir(tool_root).abspath

  # Defer pre-evaluating some environment variables, but do before building
  # SConscripts.
  env.Defer(PreEvaluateVariables)
  env.Defer('BuildEnvironmentSConscripts', after=PreEvaluateVariables)
