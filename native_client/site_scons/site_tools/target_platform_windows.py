#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Build tool setup for Windows.

This module is a SCons tool which should be include in the topmost windows
environment.
It is used as follows:
  env = base_env.Clone(tools = ['component_setup'])
  win_env = base_env.Clone(tools = ['target_platform_windows'])
"""


import os
import time
import SCons.Script
import command_output


def WaitForWritable(target, source, env):
  """Waits for the target to become writable.

  Args:
    target: List of target nodes.
    source: List of source nodes.
    env: Environment context.

  Returns:
    Zero if success, nonzero if error.

  This is a necessary hack on Windows, where antivirus software can lock exe
  files briefly after they're written.  This can cause subsequent reads of the
  file by env.Install() to fail.  To prevent these failures, wait for the file
  to be writable.
  """
  env = env  # suppress lint
  source = source  # suppress lint
  target_path = target[0].abspath
  if not os.path.exists(target_path):
    return 0      # Nothing to wait for

  for unused_retries in range(10):
    try:
      f = open(target_path, 'a+b')
      f.close()
      return 0    # Successfully opened file for write, so we're done
    except (IOError, OSError):
      print 'Waiting for access to %s...' % target_path
      time.sleep(1)

  # If we're still here, fail
  print 'Timeout waiting for access to %s.' % target_path
  return 1


def RunManifest(target, source, env, cmd):
  """Run the Microsoft Visual Studio manifest tool (mt.exe).

  Args:
    target: List of target nodes.
    source: List of source nodes.
    env: Environment context.
    cmd: Command to run.

  Returns:
    Zero if success, nonzero if error.

  The mt.exe tool seems to experience intermittent failures trying to write to
  .exe or .dll files.  Antivirus software makes this worse, but the problem
  can still occur even if antivirus software is disabled.  The failures look
  like:

      mt.exe : general error c101008d: Failed to write the updated manifest to
      the resource of file "(name of exe)". Access is denied.

  with mt.exe returning an errorlevel (return code) of 31.  The workaround is
  to retry running mt.exe after a short delay.
  """
  cmdline = env.subst(cmd, target=target, source=source)

  for retry in range(5):
    # If this is a retry, print a message and delay first
    if retry:
      # mt.exe failed to write to the target file.  Print a warning message,
      # delay 3 seconds, and retry.
      print 'Warning: mt.exe failed to write to %s; retrying.' % target[0]
      time.sleep(3)

    return_code, output = command_output.RunCommand(
        cmdline, env=env['ENV'], echo_output=False)
    if return_code != 31:    # Something other than the intermittent error
      break

  # Pass through output (if any) and return code from manifest
  if output:
    print output
  return return_code


def RunManifestExe(target, source, env):
  """Calls RunManifest for updating an executable (resource_num=1)."""
  return RunManifest(target, source, env, cmd='$MANIFEST_COM')


def RunManifestDll(target, source, env):
  """Calls RunManifest for updating a dll (resource_num=2)."""
  return RunManifest(target, source, env, cmd='$SHMANIFEST_COM')


def ComponentPlatformSetup(env, builder_name):
  """Hook to allow platform to modify environment inside a component builder.

  This is called on a clone of the environment passed into the component
  builder, and is the last modification done to that environment before using
  it to call the underlying SCons builder (env.Program(), env.Library(), etc.)

  Args:
    env: Environment to modify
    builder_name: Name of the builder
  """
  if env.get('ENABLE_EXCEPTIONS'):
    env.FilterOut(
        CPPDEFINES=['_HAS_EXCEPTIONS=0'],
        # There are problems with LTCG when some files are compiled with
        # exceptions and some aren't (the v-tables for STL and BOOST classes
        # don't match).  Therefore, turn off LTCG when exceptions are enabled.
        CCFLAGS=['/GL'],
        LINKFLAGS=['/LTCG'],
        ARFLAGS=['/LTCG'],
    )
    env.Append(CCFLAGS=['/EHsc'])

  if builder_name in ('ComponentObject', 'ComponentLibrary'):
    if env.get('COMPONENT_STATIC'):
      env.Append(CPPDEFINES=['_LIB'])
    else:
      env.Append(CPPDEFINES=['_USRDLL', '_WINDLL'])

  if (not env.get('COMPONENT_TEST_SUBSYSTEM_WINDOWS') and
      builder_name == 'ComponentTestProgram'):
    env.FilterOut(
        LINKFLAGS=['/SUBSYSTEM:WINDOWS'],
    )
    env.Append(
        LINKFLAGS=['/SUBSYSTEM:CONSOLE'],
    )

  # Make sure link methods are lists, so we can append to them below
  def MakeList(env, name):
    if name in env:
      env[name] = [env[name]]

  MakeList(env, 'LINKCOM')
  MakeList(env, 'SHLINKCOM')

  # Support manifest file generation and consumption
  if env.get('MANIFEST_FILE'):
    env.Append(
        LINKCOM=[SCons.Script.Action(RunManifestExe, '$MANIFEST_COMSTR')],
        SHLINKCOM=[SCons.Script.Action(RunManifestDll, '$SHMANIFEST_COMSTR')],
    )

    # If manifest file should be autogenerated, add the -manifest link line and
    # delete the generated manifest after running mt.exe.
    if env.get('MANIFEST_FILE_GENERATED_BY_LINK'):
      env.Append(
          LINKFLAGS=['-manifest'],
          LINKCOM=[SCons.Script.Delete('$MANIFEST_FILE_GENERATED_BY_LINK')],
          SHLINKCOM=[SCons.Script.Delete('$MANIFEST_FILE_GENERATED_BY_LINK')],
      )

  # Wait for the output file to be writable before releasing control to
  # SCons.  Windows virus scanners temporarily lock modified executable files
  # for scanning, which causes SCons's env.Install() to fail intermittently.
  env.Append(
      LINKCOM=[SCons.Script.Action(WaitForWritable, None)],
      SHLINKCOM=[SCons.Script.Action(WaitForWritable, None)],
  )

#------------------------------------------------------------------------------


def _CoverageInstall(dest, source, env):
  """Version of Install that instruments EXEs and DLLs going to $TESTS_DIR.

  When the destination is under a path in $COVERAGE_INSTRUMENTATION_PATHS and
  an EXE/DLL is involved, instrument after copy. Other files are passed through
  to the original Install method. PDBs are handled specially. They are ignored
  when installed to $COVERAGE_INSTRUMENTATION_PATHS, and are instead copied
  explicitly before the corresponding EXE/DLL. Only files from under
  $DESTINATION_ROOT are considered.
  Arguments:
    dest: destination filename for the install
    source: source filename for the install
    env: the environment context in which this occurs
  """
  # Determine if this path is under $COVERAGE_INSTRUMENTATION_PATHS.
  in_instrumentation_paths = False
  for path in env.get('COVERAGE_INSTRUMENTATION_PATHS'):
    if not env.RelativePath(path, dest).startswith('..'):
      in_instrumentation_paths = True

  # Determine if source is under $DESTINATION_ROOT.
  source_under_dst_root = not env.RelativePath('$DESTINATION_ROOT',
                                               source).startswith('..')

  # get the source extension.
  source_ext = os.path.splitext(source)[1]

  if (source_ext == '.pdb' and
      source_under_dst_root and in_instrumentation_paths):
    # PDBs going into $TESTS_DIR will be copied as part of the EXE/DLL copy.
    # TODO: Put in the proper env.Requires steps instead.
    return
  elif (source_ext in ['.exe', '.dll'] and
        source_under_dst_root and in_instrumentation_paths):
    # Copy PDBs (assumed for now to match the filenames of EXEs/DLLs) into
    # place before instrumenting. The PDB is assumed to match the source PDB
    # file name.
    source_pdb = env.subst('$PDB', target=env.File(source))
    dest_pdb = env.subst('$PDB', target=env.File(dest))
    dest_pdb = os.path.join(os.path.split(dest_pdb)[0],
                            os.path.split(source_pdb)[1])
    if os.path.exists(source_pdb):
      env.Execute('copy "%s" "%s"' % (source_pdb, dest_pdb))
      WaitForWritable([env.File(dest_pdb)], None, env)
    # Copy EXEs/DLLs and then instrument.
    env.Execute('copy "%s" "%s"' % (source, dest))
    WaitForWritable([env.File(dest)], None, env)
    env.Execute('$COVERAGE_VSINSTR /COVERAGE "%s"' % dest)
  else:
    env['PRECOVERAGE_INSTALL'](dest, source, env)


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  # TODO(ncbray): Several sections here are gated out on to prevent failure on
  # non-Windows platforms.  This appears to be SCons issue 1720 manifesting
  # itself.  A more principled fix would be nice.

  use_msvc_tools = (env['PLATFORM'] in ('win32', 'cygwin')
                    and not env.Bit('built_elsewhere'))

  # Preserve some variables that get blown away by the tools.
  saved = dict()
  for k in ['CFLAGS', 'CCFLAGS', 'CXXFLAGS', 'LINKFLAGS', 'LIBS']:
    saved[k] = env.get(k, [])
    env[k] = []

  # Bring in the outside PATH, INCLUDE, and LIB if not blocked.
  if not env.get('MSVC_BLOCK_ENVIRONMENT_CHANGES'):
    env.AppendENVPath('PATH', os.environ.get('PATH', '[]'))
    env.AppendENVPath('INCLUDE', os.environ.get('INCLUDE', '[]'))
    env.AppendENVPath('LIB', os.environ.get('LIB', '[]'))

  # Load various Visual Studio related tools.
  if use_msvc_tools:
    env.Tool('as')
    env.Tool('msvs')
    env.Tool('windows_hard_link')

  pre_msvc_env = env['ENV'].copy()

  if use_msvc_tools:
    env.Tool('msvc')
    env.Tool('mslib')
    env.Tool('mslink')
  else:
    # Make sure we have all the builders even when MSVC is not available.
    # Without these (fake) builders, SCons cannot be run on a Windows bot
    # that does not have MSVC installed - even if MSVC is never invoked.
    env.Tool('cc')
    env.Tool('c++')
    env.Tool('ar')
    env.Tool('link')
    def RES(*argv, **karg):
      return []
    env.AddMethod(RES)

  # Find VC80_DIR if it isn't already set.
  if not env.get('VC80_DIR'):
    # Look in each directory in the path for cl.exe.
    for p in env['ENV']['PATH'].split(os.pathsep):
      # Use the directory two layers up if it exists.
      if os.path.exists(os.path.join(p, 'cl.exe')):
        env['VC80_DIR'] = os.path.dirname(os.path.dirname(p))

  # The msvc, mslink, and mslib tools search the registry for installed copies
  # of Visual Studio and prepends them to the PATH, INCLUDE, and LIB
  # environment variables.  Block these changes if necessary.
  if env.get('MSVC_BLOCK_ENVIRONMENT_CHANGES'):
    env['ENV'] = pre_msvc_env

  # Set target platform bits
  env.SetBits('windows')

  env.Replace(
      TARGET_PLATFORM='WINDOWS',
      COMPONENT_PLATFORM_SETUP=ComponentPlatformSetup,

      # A better rebuild command (actually cleans, then rebuild)
      MSVSREBUILDCOM=''.join(['$MSVSSCONSCOM -c "$MSVSBUILDTARGET" && ',
                              '$MSVSSCONSCOM "$MSVSBUILDTARGET"']),
  )

  env.SetDefault(
      # Command line option to include a header
      CCFLAG_INCLUDE='/FI',

      # Generate PDBs matching target name by default.
      PDB='${TARGET.base}.pdb',

      # Code coverage related.
      COVERAGE_CCFLAGS='-DCOVERAGE',
      COVERAGE_LINKFLAGS='/PROFILE',  # Requires vc_80 or higher.
      COVERAGE_SHLINKFLAGS='$COVERAGE_LINKFLAGS',
      # Change install step for coverage to cause instrumentation.
      COVERAGE_INSTALL=_CoverageInstall,
      # NOTE: need to ignore error in return type here, the tool has issues.
      #   Thus a - is added.
      COVERAGE_START_CMD=[
          # If a previous build was cancelled or crashed, VSPerfCmd may still
          # be running, which causes future coverage runs to fail.  Make sure
          # it's shut down before starting coverage up again.
          '-$COVERAGE_VSPERFCMD -shutdown',
          '$COVERAGE_VSPERFCMD -start:coverage '
          '-output:${COVERAGE_OUTPUT_FILE}.pre'],
      COVERAGE_STOP_CMD=[
          '-$COVERAGE_VSPERFCMD -shutdown',
          'c:\\Windows\\System32\\regsvr32.exe /S '
          '$COVERAGE_ANALYZER_DIR/msdia80.dll',
          '$COVERAGE_ANALYZER -sym_path=. ${COVERAGE_OUTPUT_FILE}.pre.coverage',
          'c:\\Windows\\System32\\regsvr32.exe /S /U '
          '$COVERAGE_ANALYZER_DIR/msdia80.dll',
          # TODO(bradnelson): eliminate cygwin dependency.
          #     Cygwin is assumed to be present so this script can call
          #     cygpath inside.
          #     NOTE: cygwin is only required for coverage on windows.
          ('c:\\cygwin\\bin\\bash -c "'
           'PATH=/cygdrive/c/cygwin/bin '
           'build/filter_windows_lcov.py '
           "< `cygpath '${COVERAGE_OUTPUT_FILE}.pre.coverage.lcov'` "
           "> `cygpath '${COVERAGE_OUTPUT_FILE}'` \""),
          # TODO(bradnelson): eliminate cygwin dependency.
          #     Cygwin is assumed to be present because genhtml is filled with
          #     unix-y pathing assumptions.
          #     NOTE: cygwin is only required for coverage on windows.
          ('c:\\cygwin\\bin\\bash -c "'
           'PATH=/cygdrive/c/cygwin/bin '
           '$COVERAGE_GENHTML '
           '--output-directory '
           "`/usr/bin/cygpath '${COVERAGE_HTML_DIR}'` "
           "`/usr/bin/cygpath '${COVERAGE_OUTPUT_FILE}'` "
           '"'),
      ],
      COVERAGE_EXTRA_PATHS=['$COVERAGE_ANALYZER_DIR'],
      # Directories for which EXEs and DLLs should by instrumented on install.
      COVERAGE_INSTRUMENTATION_PATHS=['$TESTS_DIR', '$ARTIFACTS_DIR'],

      # Manifest options
      # When link.exe is run with '-manifest', it always generated a manifest
      # with this name.
      MANIFEST_FILE_GENERATED_BY_LINK='${TARGET}.manifest',
      # Manifest file to use as input to mt.exe.  Can be overridden to pass in
      # a pregenerated manifest file.
      MANIFEST_FILE='$MANIFEST_FILE_GENERATED_BY_LINK',
      MANIFEST_COM=('mt.exe -nologo -manifest "$MANIFEST_FILE" '
                    '-outputresource:"$TARGET";1'),
      MANIFEST_COMSTR='$MANIFEST_COM',
      SHMANIFEST_COM=('mt.exe -nologo -manifest "$MANIFEST_FILE" '
                      '-outputresource:"$TARGET";2'),
      SHMANIFEST_COMSTR='$SHMANIFEST_COM',
  )

  env.Append(
      # Turn up the warning level
      CCFLAGS=['/W3'],

      # Settings for debug
      CCFLAGS_DEBUG=[
          '/Od',     # disable optimizations
          '/RTC1',   # enable fast checks
          '/MTd',    # link with LIBCMTD.LIB debug lib
      ],
      LINKFLAGS_DEBUG=['/DEBUG'],

      # Settings for optimized
      CCFLAGS_OPTIMIZED=[
          '/O1',     # optimize for size
          '/MT',     # link with LIBCMT.LIB (multi-threaded, static linked crt)
          '/GS',     # enable security checks
      ],
      # Omit the absolute pathname of the .pdb debugging info file
      # from the executable, and just use a relative pathname.
      LINKFLAGS_OPTIMIZED=['/PDBALTPATH:%_PDB%'],

      # Settings for component_builders
      COMPONENT_LIBRARY_LINK_SUFFIXES=['.lib'],
      COMPONENT_LIBRARY_DEBUG_SUFFIXES=['.pdb'],
  )

  # TODO: mslink.py creates a shlibLinkAction which doesn't specify
  # '$SHLINKCOMSTR' as its command string.  This breaks --brief.  For now,
  # hack into the existing action and override its command string.
  if use_msvc_tools:
    env['SHLINKCOM'].list[0].cmdstr = '$SHLINKCOMSTR'

  # Restore saved flags.
  env.Append(**saved)
