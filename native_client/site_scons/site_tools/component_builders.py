#!/usr/bin/python2.4
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Software construction toolkit builders for SCons."""


import SCons
import library_deps

__component_list = {}


def _InitializeComponentBuilders(env):
  """Re-initializes component builders module.

  Args:
    env: Environment context
  """
  env = env     # Silence gpylint

  __component_list.clear()


def _RetrieveComponents(component_name, filter_components=None):
  """Get the list of all components required by the specified component.

  Args:
    component_name: Name of the base component.
    filter_components: List of components NOT to include.

  Returns:
    A list of the transitive closure of all components required by the base
    component.  That is, if A requires B and B requires C, this returns [B, C].

  """
  if filter_components:
    filter_components = set(filter_components)
  else:
    filter_components = set()

  components = set([component_name])    # Components always require themselves
  new_components = set(components)
  while new_components:
    # Take next new component and add it to the list we've already scanned.
    c = new_components.pop()
    components.add(c)
    # Add to the list of new components any of c's components that we haven't
    # seen before.
    new_components.update(__component_list.get(c, set())
                          - components - filter_components)

  return list(components)


def _StoreComponents(self, component_name):
  """Stores the list of child components for the specified component.

  Args:
    self: Environment containing component.
    component_name: Name of the component.

  Adds component references based on the LIBS and COMPONENTS variables in the
  current environment.  Should be called at primary SConscript execution time;
  use _RetrieveComponents() to get the final components lists in a Defer()'d
  function.
  """

  components = set()
  for clist in ('LIBS', 'COMPONENTS'):
    components.update(map(self.subst, self.Flatten(self[clist])))

  if component_name not in __component_list:
    __component_list[component_name] = set()
  __component_list[component_name].update(components)


def _ComponentPlatformSetup(env, builder_name, **kwargs):
  """Modify an environment to work with a component builder.

  Args:
    env: Environment to clone.
    builder_name: Name of the builder.
    kwargs: Keyword arguments.

  Returns:
    A modified clone of the environment.
  """
  # Clone environment so we can modify it
  env = env.Clone()

  # Add all keyword arguments to the environment
  for k, v in kwargs.items():
    env[k] = v

  # Add compiler flags for included headers, if any
  env['INCLUDES'] = env.Flatten(env.subst_list(['$INCLUDES']))
  for h in env['INCLUDES']:
    env.Append(CCFLAGS=['${CCFLAG_INCLUDE}%s' % h])

  # This supports a NaCl convention that was previously supported with a
  # modification to SCons.  Previously, EXTRA_LIBS was interpolated into LIBS
  # using the ${EXTRA_LIBS} syntax.  It appears, however, that SCons naturally
  # computes library dependencies before interpolation, so EXTRA_LIBS will not
  # be correctly depended upon if interpolated.  In the past, SCons was modified
  # to force interpolation before library dependencies were computed.  This new
  # approach allows us to use an unmodified version of SCons.
  # In general, the use of EXTRA_LIBS is discouraged.
  if 'EXTRA_LIBS' in env:
    # The SubstList2 method expands and flattens so that scons will
    # correctly know about the library dependencies in cases like
    # EXTRA_LIBS=['${FOO_LIBS}', 'bar'].
    env['LIBS'] = (library_deps.AddLibDeps(env,
                                           env['TARGET_FULLARCH'],
                                           env.SubstList2('${EXTRA_LIBS}')) +
                   env.SubstList2('${LIBS}'))

  # Call platform-specific component setup function, if any
  if env.get('COMPONENT_PLATFORM_SETUP'):
    env['COMPONENT_PLATFORM_SETUP'](env, builder_name)

  # Return the modified environment
  return env

#------------------------------------------------------------------------------

# TODO: Should be possible to refactor programs, test programs, libs to all
# publish as packages, for simplicity and code reuse.


def ComponentPackageDeferred(env):
  """Deferred build steps for component package.

  Args:
    env: Environment from ComponentPackage().

  Sets up the aliases to build the package.
  """
  package_name = env['PACKAGE_NAME']

  # Install program and resources
  all_outputs = []
  package_filter = env.Flatten(env.subst_list('$COMPONENT_PACKAGE_FILTER'))
  components = _RetrieveComponents(package_name, package_filter)
  for resource, dest_dir in env.get('COMPONENT_PACKAGE_RESOURCES').items():
    all_outputs += env.ReplicatePublished(dest_dir, components, resource)

  # Add installed program and resources to the alias
  env.Alias(package_name, all_outputs)


def ComponentPackage(self, package_name, dest_dir, **kwargs):
  """Pseudo-builder for package containing other components.

  Args:
    self: Environment in which we were called.
    package_name: Name of package.
    dest_dir: Destination directory for package.
    kwargs: Keyword arguments.

  Returns:
    The alias node for the package.
  """
  # Clone and modify environment
  env = _ComponentPlatformSetup(self, 'ComponentPackage', **kwargs)

  env.Replace(
      PACKAGE_NAME=package_name,
      PACKAGE_DIR=dest_dir,
  )

  # Add an empty alias for the package and add it to the right groups
  a = env.Alias(package_name, [])
  for group in env['COMPONENT_PACKAGE_GROUPS']:
    SCons.Script.Alias(group, a)

  # Store list of components for this program
  env._StoreComponents(package_name)

  # Let component_targets know this target is available in the current mode
  env.SetTargetProperty(package_name, TARGET_PATH=dest_dir)

  # Set up deferred call to replicate resources
  env.Defer(ComponentPackageDeferred)

  # Return the alias, since it's the only node we have
  return a

#------------------------------------------------------------------------------


def ComponentObject(self, *args, **kwargs):
  """Pseudo-builder for object to handle platform-dependent type.

  Args:
    self: Environment in which we were called.
    args: Positional arguments.
    kwargs: Keyword arguments.

  Returns:
    Passthrough return code from env.StaticLibrary() or env.SharedLibrary().

  TODO: Perhaps this should be a generator builder, so it can take a list of
  inputs and return a list of outputs?
  """
  # Clone and modify environment
  env = _ComponentPlatformSetup(self, 'ComponentObject', **kwargs)

  # Make appropriate object type
  if env.get('COMPONENT_STATIC'):
    o = env.StaticObject(*args, **kwargs)
  else:
    o = env.SharedObject(*args, **kwargs)

  # Add dependencies on includes
  env.Depends(o, env['INCLUDES'])

  return o

#------------------------------------------------------------------------------


def ComponentLibrary(self, lib_name, *args, **kwargs):
  """Pseudo-builder for library to handle platform-dependent type.

  Args:
    self: Environment in which we were called.
    lib_name: Library name.
    args: Positional arguments.
    kwargs: Keyword arguments.

  Returns:
    Passthrough return code from env.StaticLibrary() or env.SharedLibrary().
  """
  # Clone and modify environment
  env = _ComponentPlatformSetup(self, 'ComponentLibrary', **kwargs)

  # Make appropriate library type
  if env.get('COMPONENT_STATIC'):
    lib_outputs = env.StaticLibrary(lib_name, *args, **kwargs)
  else:
    lib_outputs = env.SharedLibrary(lib_name, *args, **kwargs)
    # TODO(robertm): arm hack, figure out a better way to do this
    #                we should not be modifying the env as a side-effect
    # BUG: http://code.google.com/p/nativeclient/issues/detail?id=2424
    env.FilterOut(LINKFLAGS=['-static'])

  # Add dependencies on includes
  env.Depends(lib_outputs, env['INCLUDES'])

  # Scan library outputs for files we need to link against this library, and
  # files we need to run executables linked against this library.
  need_for_link = []
  need_for_debug = []
  need_for_run = []
  for o in lib_outputs:
    if o.suffix in env['COMPONENT_LIBRARY_LINK_SUFFIXES']:
      need_for_link.append(o)
    if o.suffix in env['COMPONENT_LIBRARY_DEBUG_SUFFIXES']:
      need_for_debug.append(o)
    if o.suffix == env['SHLIBSUFFIX']:
      need_for_run.append(o)
  all_outputs = lib_outputs

  # Install library in intermediate directory, so other libs and programs can
  # link against it
  all_outputs += env.Replicate('$LIB_DIR', need_for_link)

  # Publish output
  env.Publish(lib_name, 'link', need_for_link)
  env.Publish(lib_name, 'run', need_for_run)
  env.Publish(lib_name, 'debug', need_for_debug)

  # Add an alias to build and copy the library, and add it to the right groups
  a = self.Alias(lib_name, all_outputs)
  for group in env['COMPONENT_LIBRARY_GROUPS']:
    SCons.Script.Alias(group, a)

  # Store list of components for this library
  env._StoreComponents(lib_name)

  # Let component_targets know this target is available in the current mode.
  env.SetTargetProperty(lib_name, TARGET_PATH=lib_outputs[0])

  # If library should publish itself, publish as if it was a program
  if env.get('COMPONENT_LIBRARY_PUBLISH'):
    env['PROGRAM_BASENAME'] = lib_name
    env.Defer(ComponentProgramDeferred)

  # Return the library
  return lib_outputs[0]

#------------------------------------------------------------------------------


def ComponentTestProgramDeferred(env):
  """Deferred build steps for test program.

  Args:
    env: Environment from ComponentTestProgram().

  Sets up the aliases to compile and run the test program.
  """
  prog_name = env['PROGRAM_BASENAME']

  # Install program and resources
  all_outputs = []
  components = _RetrieveComponents(prog_name)
  for resource, dest_dir in env.get('COMPONENT_TEST_RESOURCES').items():
    all_outputs += env.ReplicatePublished(dest_dir, components, resource)

  # Add installed program and resources to the alias
  env.Alias(prog_name, all_outputs)

  # Add target properties
  env.SetTargetProperty(
      prog_name,
      # The copy of the program we care about is the one in the tests dir
      EXE='$TESTS_DIR/$PROGRAM_NAME',
      RUN_CMDLINE='$COMPONENT_TEST_CMDLINE',
      RUN_DIR='$TESTS_DIR',
      TARGET_PATH='$TESTS_DIR/$PROGRAM_NAME',
  )

  # Add an alias for running the test in the test directory, if the test is
  # runnable and has a test command line.
  if env.get('COMPONENT_TEST_RUNNABLE') and env.get('COMPONENT_TEST_CMDLINE'):
    env.Replace(
        COMMAND_OUTPUT_CMDLINE=env['COMPONENT_TEST_CMDLINE'],
        COMMAND_OUTPUT_RUN_DIR='$TESTS_DIR',
    )
    test_out_name = '$TEST_OUTPUT_DIR/${PROGRAM_BASENAME}.out.txt'
    if (env.GetOption('component_test_retest')
        and env.File(test_out_name).exists()):
      # Delete old test results, so test will rerun.
      env.Execute(SCons.Script.Delete(test_out_name))

    # Set timeout based on test size
    timeout = env.get('COMPONENT_TEST_TIMEOUT')
    if type(timeout) is dict:
      timeout = timeout.get(env.get('COMPONENT_TEST_SIZE'))
    if timeout:
      env['COMMAND_OUTPUT_TIMEOUT'] = timeout

    # Test program is the first run resource we replicated.  (Duplicate
    # replicate is not harmful, and is a handy way to pick out the correct
    # file from all those we replicated above.)
    test_program = env.ReplicatePublished('$TESTS_DIR', prog_name, 'run')

    # Run the test.  Note that we need to refer to the file by name, so that
    # SCons will recreate the file node after we've deleted it; if we used the
    # env.File() we created in the if statement above, SCons would still think
    # it exists and not rerun the test.
    test_out = env.CommandOutput(test_out_name, test_program)

    # Running the test requires the test and its libs copied to the tests dir
    env.Depends(test_out, all_outputs)
    env.ComponentTestOutput('run_' + prog_name, test_out)

    # Add target properties
    env.SetTargetProperty(prog_name, RUN_TARGET='run_' + prog_name)


def ComponentTestProgram(self, prog_name, *args, **kwargs):
  """Pseudo-builder for test program to handle platform-dependent type.

  Args:
    self: Environment in which we were called.
    prog_name: Test program name.
    args: Positional arguments.
    kwargs: Keyword arguments.

  Returns:
    Output node list from env.Program().
  """
  # Clone and modify environment
  env = _ComponentPlatformSetup(self, 'ComponentTestProgram', **kwargs)

  env['PROGRAM_BASENAME'] = prog_name
  env['PROGRAM_NAME'] = '$PROGPREFIX$PROGRAM_BASENAME$PROGSUFFIX'

  # Call env.Program()
  out_nodes = env.Program(prog_name, *args, **kwargs)

  # Add dependencies on includes
  env.Depends(out_nodes, env['INCLUDES'])

  # Publish output
  env.Publish(prog_name, 'run', out_nodes[0])
  env.Publish(prog_name, 'debug', out_nodes[1:])

  # Add an alias to build the program to the right groups
  a = env.Alias(prog_name, out_nodes)
  for group in env['COMPONENT_TEST_PROGRAM_GROUPS']:
    SCons.Script.Alias(group, a)

  # Store list of components for this program
  env._StoreComponents(prog_name)

  # Let component_targets know this target is available in the current mode
  env.SetTargetProperty(prog_name, TARGET_PATH=out_nodes[0])

  # Set up deferred call to replicate resources and run test
  env.Defer(ComponentTestProgramDeferred)

  # Return the output node
  return out_nodes

#------------------------------------------------------------------------------


def ComponentProgramDeferred(env):
  """Deferred build steps for program.

  Args:
    env: Environment from ComponentProgram().

  Sets up the aliases to compile the program.
  """
  prog_name = env['PROGRAM_BASENAME']

  # Install program and resources
  all_outputs = []
  components = _RetrieveComponents(prog_name)
  for resource, dest_dir in env.get('COMPONENT_PROGRAM_RESOURCES').items():
    all_outputs += env.ReplicatePublished(dest_dir, components, resource)

  # Add installed program and resources to the alias
  env.Alias(prog_name, all_outputs)


def ComponentProgram(self, prog_name, *args, **kwargs):
  """Pseudo-builder for program to handle platform-dependent type.

  Args:
    self: Environment in which we were called.
    prog_name: Test program name.
    args: Positional arguments.
    kwargs: Keyword arguments.

  Returns:
    Output node list from env.Program().
  """
  # Clone and modify environment
  env = _ComponentPlatformSetup(self, 'ComponentProgram', **kwargs)

  env['PROGRAM_BASENAME'] = prog_name

  if env['PROGSUFFIX'] and env.subst(prog_name).endswith(env['PROGSUFFIX']):
    # Temporary hack: If there's already an extension, remove it.
    # Because PPAPI is revision locked, and expects to be able to use .nexe
    # TODO: When PPAPI deps is rolled, replace with this:
    # raise Exception("Program name shouldn't have a suffix")
    prog_name = env.subst(prog_name)
    prog_name = prog_name[:-len(env['PROGSUFFIX'])]

  # Call env.Program()
  out_nodes = env.Program(prog_name, *args, **kwargs)

  # Add dependencies on includes
  env.Depends(out_nodes, env['INCLUDES'])

  # Add dependencies on libraries marked as implicitly included in the link.
  # These are libraries that are not passed on the command line, but are
  # always linked in by the toolchain, i.e. startup files and -lc and such.
  if 'IMPLICIT_LIBS' in env:
    env.Depends(out_nodes, env['IMPLICIT_LIBS'])

  # Publish output
  env.Publish(prog_name, 'run', out_nodes[0])
  env.Publish(prog_name, 'debug', out_nodes[1:])

  # Add an alias to build the program to the right groups
  a = env.Alias(prog_name, out_nodes)
  env.ComponentProgramAlias(a)

  # Store list of components for this program
  env._StoreComponents(prog_name)

  # Let component_targets know this target is available in the current mode
  env.SetTargetProperty(prog_name, TARGET_PATH=out_nodes[0])

  # Set up deferred call to replicate resources
  env.Defer(ComponentProgramDeferred)

  # Return the executable
  return out_nodes[0]

def ComponentProgramAlias(self, program):
  for group in self['COMPONENT_PROGRAM_GROUPS']:
    SCons.Script.Alias(group, program)

#------------------------------------------------------------------------------


def ComponentTestOutput(self, test_name, nodes, **kwargs):
  """Pseudo-builder for test output.

  Args:
    self: Environment in which we were called.
    test_name: Test name.
    nodes: List of files/Nodes output by the test.
    kwargs: Keyword arguments.

  Returns:
    Passthrough return code from env.Alias().
  """

  # Clone and modify environment
  env = _ComponentPlatformSetup(self, 'ComponentTestObject', **kwargs)

  # Add an alias for the test output
  a = env.Alias(test_name, nodes)

  # Determine groups test belongs in
  if env.get('COMPONENT_TEST_ENABLED'):
    groups = env.SubstList2('$COMPONENT_TEST_OUTPUT_GROUPS')
    if env.get('COMPONENT_TEST_SIZE'):
      groups.append(env.subst('run_${COMPONENT_TEST_SIZE}_tests'))
  else:
    # Disabled tests only go in the explicit disabled tests group
    groups = ['run_disabled_tests']

  for group in groups:
    SCons.Script.Alias(group, a)

  # Let component_targets know this target is available in the current mode
  env.SetTargetProperty(test_name, TARGET_PATH=nodes[0])

  # Return the output node
  return a

#------------------------------------------------------------------------------


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  env.Replace(
      LIB_DIR='$TARGET_ROOT/lib',
      # TODO: Remove legacy COMPONENT_LIBRARY_DIR, once all users have
      # transitioned to LIB_DIR
      COMPONENT_LIBRARY_DIR='$LIB_DIR',
      STAGING_DIR='$TARGET_ROOT/staging',
      TESTS_DIR='$TARGET_ROOT/tests',
      TEST_OUTPUT_DIR='$TARGET_ROOT/test_output',
      # Default command line for a test is just the name of the file.
      # TODO: Why doesn't the following work:
      # COMPONENT_TEST_CMDLINE='${SOURCE.abspath}',
      # (it generates a SCons error)
      COMPONENT_TEST_CMDLINE='${PROGRAM_NAME}',
      # Component tests are runnable by default.
      COMPONENT_TEST_RUNNABLE=True,
      # Default test size is large
      COMPONENT_TEST_SIZE='large',
      # Default timeouts for component tests
      COMPONENT_TEST_TIMEOUT={'large': 900, 'medium': 450, 'small': 180},
      # Tests are enabled by default
      COMPONENT_TEST_ENABLED=True,
      # Static linking is a sensible default
      COMPONENT_STATIC=True,
      # Don't publish libraries to the staging dir by themselves by default.
      COMPONENT_LIBRARY_PUBLISH=False,
  )
  env.Append(
      LIBPATH=['$LIB_DIR'],
      RPATH=['$LIB_DIR'],

      # Default alias groups for component builders
      COMPONENT_PACKAGE_GROUPS=['all_packages'],
      COMPONENT_LIBRARY_GROUPS=['all_libraries'],
      COMPONENT_PROGRAM_GROUPS=['all_programs'],
      COMPONENT_TEST_PROGRAM_GROUPS=['all_test_programs'],
      COMPONENT_TEST_OUTPUT_GROUPS=['run_all_tests'],

      # Additional components whose resources should be copied into program
      # directories, in addition to those from LIBS and the program itself.
      LIBS=[],
      COMPONENTS=[],

      # Dicts of what resources should go in each destination directory for
      # programs and test programs.
      COMPONENT_PACKAGE_RESOURCES={
          'run': '$PACKAGE_DIR',
          'debug': '$PACKAGE_DIR',
      },
      COMPONENT_PROGRAM_RESOURCES={
          'run': '$STAGING_DIR',
          'debug': '$STAGING_DIR',
      },
      COMPONENT_TEST_RESOURCES={
          'run': '$TESTS_DIR',
          'debug': '$TESTS_DIR',
          'test_input': '$TESTS_DIR',
      },
  )

  # Add command line option for retest
  SCons.Script.AddOption(
      '--retest',
      dest='component_test_retest',
      action='store_true',
      help='force all tests to rerun')
  SCons.Script.Help('  --retest                    '
                    'Rerun specified tests, ignoring cached results.\n')

  # Defer per-environment initialization, but do before building SConscripts
  env.Defer(_InitializeComponentBuilders)
  env.Defer('BuildEnvironmentSConscripts', after=_InitializeComponentBuilders)

  # Add our pseudo-builder methods
  env.AddMethod(_StoreComponents)
  env.AddMethod(ComponentPackage)
  env.AddMethod(ComponentObject)
  env.AddMethod(ComponentLibrary)
  env.AddMethod(ComponentProgram)
  env.AddMethod(ComponentProgramAlias)
  env.AddMethod(ComponentTestProgram)
  env.AddMethod(ComponentTestOutput)

  # Add our target groups
  AddTargetGroup('all_libraries', 'libraries can be built')
  AddTargetGroup('all_programs', 'programs can be built')
  AddTargetGroup('all_test_programs', 'tests can be built')
  AddTargetGroup('all_packages', 'packages can be built')
  AddTargetGroup('run_all_tests', 'tests can be run')
  AddTargetGroup('run_disabled_tests', 'tests are disabled')
  AddTargetGroup('run_small_tests', 'small tests can be run')
  AddTargetGroup('run_medium_tests', 'medium tests can be run')
  AddTargetGroup('run_large_tests', 'large tests can be run')
