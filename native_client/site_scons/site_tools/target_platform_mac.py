#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Build tool setup for MacOS.

This module is a SCons tool which should be include in the topmost mac
environment.
It is used as follows:
  env = base_env.Clone(tools = ['component_setup'])
  mac_env = base_env.Clone(tools = ['target_platform_mac'])
"""


import SCons.Script


def ComponentPlatformSetup(env, builder_name):
  """Hook to allow platform to modify environment inside a component builder.

  Args:
    env: Environment to modify
    builder_name: Name of the builder
  """
  if env.get('ENABLE_EXCEPTIONS'):
    env.FilterOut(CCFLAGS=['-fno-exceptions'])
    env.Append(CCFLAGS=['-fexceptions'])

#------------------------------------------------------------------------------
# TODO: This bundle builder here needs refactoring to use ComponentPackage().
# Until that refactoring, consider this code very experimental (i.e., don't use
# it unless you're ok with it changing out from underneath you).

def BundlePseudoBuilder(env, target, **kwargs):
  """MacOS Bundle PseudoBuilder.

  Args:
    env: Environment in which to build
    target: Name of the bundle to build
    kwargs: Additional parameters to set in the environment

  Returns:
    The target is returned.
  """
  # Don't change the environment passed into the pseudo-builder
  env = env.Clone()

  # Bring keywords args into the environment.
  for k, v in kwargs.items():
    env[k] = v
  # Make sure BUNDLE_RESOURCES is set and not empty; force it to be a list
  bundle_resources = env.Flatten(env.get('BUNDLE_RESOURCES', []))
  if not bundle_resources:
    raise ValueError('BUNDLE_RESOURCES must be set and non-empty')

  # Make each resource into a directory node.
  # TODO: this seems a little too restrictive.
  # bundle_resources = [env.Dir(i) for i in bundle_resources]
  bundle_resources = [i for i in bundle_resources]

  # Create a PkgInfo file only if BUNDLE_PKGINFO_FILENAME is useful.
  # (NPAPI bundles are unhappy with PkgInfo files.)
  if env.get('BUNDLE_PKGINFO_FILENAME'):
    pkginfo_create_command = ('$BUNDLE_GENERATE_PKGINFO '
                              '>$TARGET/$BUNDLE_PKGINFO_FILENAME')
  else:
    pkginfo_create_command = '/bin/echo no PkgInfo will be created' # noop

  # Add the build step for the bundle.
  p = env.Command(env.Dir(target),
              [env.File('$BUNDLE_EXE'),
               env.File('$BUNDLE_INFO_PLIST')] +
              bundle_resources,
              [SCons.Script.Delete('$TARGET'),
               SCons.Script.Mkdir('$TARGET/Contents'),
               SCons.Script.Mkdir('$TARGET/Contents/MacOS'),
               SCons.Script.Mkdir('$TARGET/Contents/Resources'),
               'cp -f $SOURCE $TARGET/Contents/MacOS',
               'cp -f ${SOURCES[1]} $TARGET/Contents',
               pkginfo_create_command,
               'cp -rf ${SOURCES[2:]} $TARGET/Contents/Resources'])

  # Add an alias for this target.
  # This also allows the 'all_bundles' target to build me.
  a = env.Alias(target, p)
  for group in env['COMPONENT_BUNDLE_GROUPS']:
    SCons.Script.Alias(group, a)

  return env.Dir(target)

#------------------------------------------------------------------------------


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  # Preserve some variables that get blown away by the tools.
  saved = dict()
  for k in ['ASFLAGS', 'CFLAGS', 'CCFLAGS', 'CXXFLAGS', 'LINKFLAGS', 'LIBS']:
    saved[k] = env.get(k, [])
    env[k] = []

  # Use g++
  env.Tool('g++')
  env.Tool('gcc')
  env.Tool('gnulink')
  env.Tool('ar')
  env.Tool('as')
  env.Tool('applelink')

  # Set target platform bits
  env.SetBits('mac', 'posix')

  env.Replace(
      TARGET_PLATFORM='MAC',
      COMPONENT_PLATFORM_SETUP=ComponentPlatformSetup,
      CCFLAG_INCLUDE='-include',     # Command line option to include a header

      # Code coverage related.
      COVERAGE_CCFLAGS=['--coverage', '-DCOVERAGE'],
      COVERAGE_LIBS=['profile_rt'],
      COVERAGE_LINKFLAGS=['--coverage'],
      COVERAGE_STOP_CMD=[
          '$COVERAGE_MCOV --directory "$TARGET_ROOT" --output "$TARGET"',
          ('$COVERAGE_GENHTML --output-directory $COVERAGE_HTML_DIR '
           '$COVERAGE_OUTPUT_FILE'),
      ],

      # Libraries expect to be in the same directory as their executables.
      # This is correct for unit tests, and for libraries which are published
      # in Contents/MacOS next to their executables.
      DYLIB_INSTALL_NAME_FLAGS=[
          '-install_name',
          '@loader_path/${TARGET.file}'
      ],
  )

  env.Append(
      # Mac apps and dylibs have a more strict relationship about where they
      # expect to find each other.  When an app is linked, it stores the
      # relative path from itself to any dylibs it links against.  Override
      # this so that it will store the relative path from $LIB_DIR instead.
      # This is similar to RPATH on Linux.
      LINKFLAGS = [
          '-Xlinker', '-executable_path',
          '-Xlinker', '$LIB_DIR',
      ],
      # Similarly, tell the library where it expects to find itself once it's
      # installed.
      SHLINKFLAGS = ['$DYLIB_INSTALL_NAME_FLAGS'],

      # Settings for debug
      CCFLAGS_DEBUG=[
          '-O0',     # turn off optimizations
          '-g',      # turn on debugging info
      ],
      LINKFLAGS_DEBUG=['-g'],

      # Settings for optimized
      # Optimized for space by default, which is what Xcode does
      CCFLAGS_OPTIMIZED=['-Os'],

      # Settings for component_builders
      COMPONENT_LIBRARY_LINK_SUFFIXES=['.dylib', '.a'],
      COMPONENT_LIBRARY_DEBUG_SUFFIXES=[],

      # New 'all' target.  Helpful: "hammer -h" now lists it!
      COMPONENT_BUNDLE_GROUPS=['all_bundles'],
  )


  # Set default values used by the Bundle pseudobuilder.
  env.Replace(
      BUNDLE_TYPE='APPL',
      BUNDLE_STRING='${BUNDLE_TYPE}????',
      BUNDLE_GENERATE_PKGINFO='echo "${BUNDLE_STRING}"',
      BUNDLE_PKGINFO_FILENAME='PkgInfo',
      BUNDLE_INFO_PLIST='Info.plist',
  )

  # Add the Bundle pseudobuilder.
  env.AddMethod(BundlePseudoBuilder, 'Bundle')

  # Add our target groups
  AddTargetGroup('all_bundles', 'bundles can be built')

  # Restore saved flags.
  env.Append(**saved)
