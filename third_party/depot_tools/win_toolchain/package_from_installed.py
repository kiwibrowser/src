# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
From a system-installed copy of the toolchain, packages all the required bits
into a .zip file.

It assumes default install locations for tools, in particular:
- C:\Program Files (x86)\Microsoft Visual Studio 12.0\...
- C:\Program Files (x86)\Windows Kits\10\...

1. Start from a fresh Win7 VM image.
2. Install VS Pro. Deselect everything except MFC.
3. Install Windows 10 SDK. Select only the Windows SDK and Debugging Tools for
Windows.
4. Run this script, which will build a <sha1>.zip.

Express is not yet supported by this script, but patches welcome (it's not too
useful as the resulting zip can't be redistributed, and most will presumably
have a Pro license anyway).
"""

import collections
import glob
import json
import optparse
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import zipfile

import get_toolchain_if_necessary


VS_VERSION = None
WIN_VERSION = None
VC_TOOLS = None


def GetVSPath():
  if VS_VERSION == '2015':
    return r'C:\Program Files (x86)\Microsoft Visual Studio 14.0'
  elif VS_VERSION == '2017':
    # Use vswhere to find the VS 2017 installation. This will find prerelease
    # versions because -prerelease is specified. This assumes that only one
    # version is installed.
    command = (r'C:\Program Files (x86)\Microsoft Visual Studio\Installer'
               r'\vswhere.exe -prerelease')
    marker = 'installationPath: '
    for line in subprocess.check_output(command).splitlines():
      if line.startswith(marker):
        return line[len(marker):]
    raise Exception('VS 2017 path not found in vswhere output')
  else:
    raise ValueError(VS_VERSION)


def ExpandWildcards(root, sub_dir):
  # normpath is needed to change '/' to '\\' characters.
  path = os.path.normpath(os.path.join(root, sub_dir))
  matches = glob.glob(path)
  if len(matches) != 1:
    raise Exception('%s had %d matches - should be one' % (path, len(matches)))
  return matches[0]


def BuildRepackageFileList(src_dir):
  # Strip off a trailing slash if present
  if src_dir.endswith('\\'):
    src_dir = src_dir[:-1]
  result = []
  for root, _, files in os.walk(src_dir):
    for f in files:
      final_from = os.path.normpath(os.path.join(root, f))
      dest = final_from[len(src_dir) + 1:]
      result.append((final_from, dest))
  return result


def BuildFileList(override_dir):
  result = []

  # Subset of VS corresponding roughly to VC.
  paths = [
      'DIA SDK/bin',
      'DIA SDK/idl',
      'DIA SDK/include',
      'DIA SDK/lib',
      VC_TOOLS + '/atlmfc',
      VC_TOOLS + '/crt',
      'VC/redist',
  ]

  if override_dir:
    paths += [
        (os.path.join(override_dir, 'bin'), VC_TOOLS + '/bin'),
        (os.path.join(override_dir, 'include'), VC_TOOLS + '/include'),
        (os.path.join(override_dir, 'lib'), VC_TOOLS + '/lib'),
    ]
  else:
    paths += [
        VC_TOOLS + '/bin',
        VC_TOOLS + '/include',
        VC_TOOLS + '/lib',
    ]

  if VS_VERSION == '2015':
    paths += [
        ('VC/redist/x86/Microsoft.VC140.CRT', 'sys32'),
        ('VC/redist/x86/Microsoft.VC140.CRT', 'win_sdk/bin/x86'),
        ('VC/redist/x86/Microsoft.VC140.MFC', 'sys32'),
        ('VC/redist/debug_nonredist/x86/Microsoft.VC140.DebugCRT', 'sys32'),
        ('VC/redist/debug_nonredist/x86/Microsoft.VC140.DebugMFC', 'sys32'),
        ('VC/redist/x64/Microsoft.VC140.CRT', 'sys64'),
        ('VC/redist/x64/Microsoft.VC140.CRT', 'VC/bin/amd64_x86'),
        ('VC/redist/x64/Microsoft.VC140.CRT', 'VC/bin/amd64'),
        ('VC/redist/x64/Microsoft.VC140.CRT', 'win_sdk/bin/x64'),
        ('VC/redist/x64/Microsoft.VC140.MFC', 'sys64'),
        ('VC/redist/debug_nonredist/x64/Microsoft.VC140.DebugCRT', 'sys64'),
        ('VC/redist/debug_nonredist/x64/Microsoft.VC140.DebugMFC', 'sys64'),
    ]
  elif VS_VERSION == '2017':
    paths += [
        ('VC/redist/MSVC/14.*.*/x86/Microsoft.VC*.CRT', 'sys32'),
        ('VC/redist/MSVC/14.*.*/x86/Microsoft.VC*.CRT', 'win_sdk/bin/x86'),
        ('VC/redist/MSVC/14.*.*/debug_nonredist/x86/Microsoft.VC*.DebugCRT', 'sys32'),
        ('VC/redist/MSVC/14.*.*/x64/Microsoft.VC*.CRT', 'sys64'),
        ('VC/redist/MSVC/14.*.*/x64/Microsoft.VC*.CRT', 'VC/bin/amd64_x86'),
        ('VC/redist/MSVC/14.*.*/x64/Microsoft.VC*.CRT', 'VC/bin/amd64'),
        ('VC/redist/MSVC/14.*.*/x64/Microsoft.VC*.CRT', 'win_sdk/bin/x64'),
        ('VC/redist/MSVC/14.*.*/debug_nonredist/x64/Microsoft.VC*.DebugCRT', 'sys64'),
    ]
  else:
    raise ValueError('VS_VERSION %s' % VS_VERSION)

  vs_path = GetVSPath()

  for path in paths:
    src = path[0] if isinstance(path, tuple) else path
    # Note that vs_path is ignored if src is an absolute path.
    combined = ExpandWildcards(vs_path, src)
    if not os.path.exists(combined):
      raise Exception('%s missing.' % combined)
    if not os.path.isdir(combined):
      raise Exception('%s not a directory.' % combined)
    for root, _, files in os.walk(combined):
      for f in files:
        # vctip.exe doesn't shutdown, leaving locks on directories. It's
        # optional so let's avoid this problem by not packaging it.
        # https://crbug.com/735226
        if f.lower() =='vctip.exe':
          continue
        final_from = os.path.normpath(os.path.join(root, f))
        if isinstance(path, tuple):
          assert final_from.startswith(combined)
          dest = final_from[len(combined) + 1:]
          result.append(
              (final_from, os.path.normpath(os.path.join(path[1], dest))))
        else:
          assert final_from.startswith(vs_path)
          dest = final_from[len(vs_path) + 1:]
          result.append((final_from, dest))

  sdk_path = r'C:\Program Files (x86)\Windows Kits\10'
  for root, _, files in os.walk(sdk_path):
    for f in files:
      combined = os.path.normpath(os.path.join(root, f))
      # Some of the files in this directory are exceedingly long (and exceed
      # _MAX_PATH for any moderately long root), so exclude them. We don't need
      # them anyway. Exclude others just to save space.
      tail = combined[len(sdk_path) + 1:]
      skip_dir = False
      for dir in ['References\\', 'Windows Performance Toolkit\\', 'Testing\\',
                  'App Certification Kit\\', 'Extension SDKs\\']:
        if tail.startswith(dir):
          skip_dir = True
      if skip_dir:
        continue
      # There may be many Include\Lib\Source\bin directories for many different
      # versions of Windows and packaging them all wastes ~450 MB
      # (uncompressed) per version and wastes time. Only copy the specified
      # version. Note that the SDK version number started being part of the bin
      # path with 10.0.15063.0.
      if (tail.startswith('Include\\') or tail.startswith('Lib\\') or
          tail.startswith('Source\\') or tail.startswith('bin\\')):
        if tail.count(WIN_VERSION) == 0:
          continue
      to = os.path.join('win_sdk', tail)
      # The versions of dbghelp.dll that ship with the latest 10.0.14393.0 SDK
      # (yes, there are multiple versions) as of the VS 2017 launch cannot
      # handle /debug:fastlink binaries created by VS 2017. This leads to hangs
      # during symbol lookup, as reported here:
      # https://developercommunity.visualstudio.com/content/problem/36255/chromes-base-unittests-fails-with-vs-2017-due-to-s.html
      # The recommended fix is to copy dbghelp.dll from the VS install instead,
      # as done here:
      if VS_VERSION == '2017' and combined.endswith('dbghelp.dll'):
        prefix_path = os.path.join(vs_path, 'Common7', 'IDE',
                                   'CommonExtensions', 'Microsoft',
                                   'TestWindow', 'Extensions')
        arch_dir = 'x64' if combined.count('\\x64\\') > 0 else ''
        good_dbghelp_path_candidates = [
            os.path.join(prefix_path, 'CppUnitFramework', arch_dir,
                         'dbghelp.dll'),
            os.path.join(prefix_path, 'Cpp', arch_dir, 'dbghelp.dll')
        ]
        combined = good_dbghelp_path_candidates[0]
        for c in good_dbghelp_path_candidates:
          if os.path.exists(c):
            combined = c
            break
      result.append((combined, to))

  # Copy the x86 ucrt DLLs to all directories with 32-bit binaries that are
  # added to the path by SetEnv.cmd, and to sys32.
  ucrt_paths = glob.glob(os.path.join(sdk_path, r'redist\ucrt\dlls\x86\*'))
  for ucrt_path in ucrt_paths:
    ucrt_file = os.path.split(ucrt_path)[1]
    for dest_dir in [ r'win_sdk\bin\x86', 'sys32' ]:
      result.append((ucrt_path, os.path.join(dest_dir, ucrt_file)))

  # Copy the x64 ucrt DLLs to all directories with 64-bit binaries that are
  # added to the path by SetEnv.cmd, and to sys64.
  ucrt_paths = glob.glob(os.path.join(sdk_path, r'redist\ucrt\dlls\x64\*'))
  for ucrt_path in ucrt_paths:
    ucrt_file = os.path.split(ucrt_path)[1]
    for dest_dir in [ r'VC\bin\amd64_x86', r'VC\bin\amd64',
                      r'win_sdk\bin\x64', 'sys64']:
      result.append((ucrt_path, os.path.join(dest_dir, ucrt_file)))

  system_crt_files = [
      # Needed to let debug binaries run.
      'ucrtbased.dll',
  ]
  bitness = platform.architecture()[0]
  # When running 64-bit python the x64 DLLs will be in System32
  x64_path = 'System32' if bitness == '64bit' else 'Sysnative'
  x64_path = os.path.join(r'C:\Windows', x64_path)
  for system_crt_file in system_crt_files:
      result.append((os.path.join(r'C:\Windows\SysWOW64', system_crt_file),
                      os.path.join('sys32', system_crt_file)))
      result.append((os.path.join(x64_path, system_crt_file),
                      os.path.join('sys64', system_crt_file)))

  # Generically drop all arm stuff that we don't need, and
  # drop .msi files because we don't need installers, and drop windows.winmd
  # because it is unneeded and is different on every machine.
  return [(f, t) for f, t in result if 'arm\\' not in f.lower() and
                                       'arm64\\' not in f.lower() and
                                       not f.lower().endswith('.msi') and
                                       not f.lower().endswith('.msm') and
                                       not f.lower().endswith('windows.winmd')]


def GenerateSetEnvCmd(target_dir):
  """Generate a batch file that gyp expects to exist to set up the compiler
  environment.

  This is normally generated by a full install of the SDK, but we
  do it here manually since we do not do a full install."""
  vc_tools_parts = VC_TOOLS.split('/')

  # All these paths are relative to the directory containing SetEnv.cmd.
  include_dirs = [
    ['..', '..', 'win_sdk', 'Include', WIN_VERSION, 'um'],
    ['..', '..', 'win_sdk', 'Include', WIN_VERSION, 'shared'],
    ['..', '..', 'win_sdk', 'Include', WIN_VERSION, 'winrt'],
  ]
  include_dirs.append(['..', '..', 'win_sdk', 'Include', WIN_VERSION, 'ucrt'])
  include_dirs.extend([
    ['..', '..'] + vc_tools_parts + ['include'],
    ['..', '..'] + vc_tools_parts + ['atlmfc', 'include'],
  ])
  # Common to x86 and x64
  env = collections.OrderedDict([
    # Yuck: These have a trailing \ character. No good way to represent this in
    # an OS-independent way.
    ('VSINSTALLDIR', [['..', '..\\']]),
    ('VCINSTALLDIR', [['..', '..', 'VC\\']]),
    ('INCLUDE', include_dirs),
  ])
  # x86. Always use amd64_x86 cross, not x86 on x86.
  if VS_VERSION == '2017':
    env['VCToolsInstallDir'] = [['..', '..'] + vc_tools_parts[:]]
    # Yuck: This one ends in a slash as well.
    env['VCToolsInstallDir'][0][-1] += '\\'
    env_x86 = collections.OrderedDict([
      ('PATH', [
        ['..', '..', 'win_sdk', 'bin', WIN_VERSION, 'x64'],
        ['..', '..'] + vc_tools_parts + ['bin', 'HostX64', 'x86'],
        ['..', '..'] + vc_tools_parts + ['bin', 'HostX64', 'x64'],  # Needed for mspdb1x0.dll.
      ]),
      ('LIB', [
        ['..', '..'] + vc_tools_parts + ['lib', 'x86'],
        ['..', '..', 'win_sdk', 'Lib', WIN_VERSION, 'um', 'x86'],
        ['..', '..', 'win_sdk', 'Lib', WIN_VERSION, 'ucrt', 'x86'],
        ['..', '..'] + vc_tools_parts + ['atlmfc', 'lib', 'x86'],
      ]),
    ])
  else:
    env_x86 = collections.OrderedDict([
      ('PATH', [
        ['..', '..', 'win_sdk', 'bin', WIN_VERSION, 'x86'],
        ['..', '..', 'VC', 'bin', 'amd64_x86'],
        ['..', '..', 'VC', 'bin', 'amd64'],  # Needed for mspdb1x0.dll.
      ]),
      ('LIB', [
        ['..', '..', 'VC', 'lib'],
        ['..', '..', 'win_sdk', 'Lib', WIN_VERSION, 'um', 'x86'],
        ['..', '..', 'win_sdk', 'Lib', WIN_VERSION, 'ucrt', 'x86'],
        ['..', '..', 'VC', 'atlmfc', 'lib'],
      ]),
    ])
  # x64.
  if VS_VERSION == '2017':
    env_x64 = collections.OrderedDict([
      ('PATH', [
        ['..', '..', 'win_sdk', 'bin', WIN_VERSION, 'x64'],
        ['..', '..'] + vc_tools_parts + ['bin', 'HostX64', 'x64'],
      ]),
      ('LIB', [
        ['..', '..'] + vc_tools_parts + ['lib', 'x64'],
        ['..', '..', 'win_sdk', 'Lib', WIN_VERSION, 'um', 'x64'],
        ['..', '..', 'win_sdk', 'Lib', WIN_VERSION, 'ucrt', 'x64'],
        ['..', '..'] + vc_tools_parts + ['atlmfc', 'lib', 'x64'],
      ]),
    ])
  else:
    env_x64 = collections.OrderedDict([
      ('PATH', [
        ['..', '..', 'win_sdk', 'bin', WIN_VERSION, 'x64'],
        ['..', '..', 'VC', 'bin', 'amd64'],
      ]),
      ('LIB', [
        ['..', '..', 'VC', 'lib', 'amd64'],
        ['..', '..', 'win_sdk', 'Lib', WIN_VERSION, 'um', 'x64'],
        ['..', '..', 'win_sdk', 'Lib', WIN_VERSION, 'ucrt', 'x64'],
        ['..', '..', 'VC', 'atlmfc', 'lib', 'amd64'],
      ]),
    ])
  def BatDirs(dirs):
    return ';'.join(['%~dp0' + os.path.join(*d) for d in dirs])
  set_env_prefix = os.path.join(target_dir, r'win_sdk\bin\SetEnv')
  with open(set_env_prefix + '.cmd', 'w') as f:
    f.write('@echo off\n'
            ':: Generated by win_toolchain\\package_from_installed.py.\n')
    for var, dirs in env.iteritems():
      f.write('set %s=%s\n' % (var, BatDirs(dirs)))
    f.write('if "%1"=="/x64" goto x64\n')

    for var, dirs in env_x86.iteritems():
      f.write('set %s=%s%s\n' % (
          var, BatDirs(dirs), ';%PATH%' if var == 'PATH' else ''))
    f.write('goto :EOF\n')

    f.write(':x64\n')
    for var, dirs in env_x64.iteritems():
      f.write('set %s=%s%s\n' % (
          var, BatDirs(dirs), ';%PATH%' if var == 'PATH' else ''))
  with open(set_env_prefix + '.x86.json', 'wb') as f:
    assert not set(env.keys()) & set(env_x86.keys()), 'dupe keys'
    json.dump({'env': collections.OrderedDict(env.items() + env_x86.items())},
              f)
  with open(set_env_prefix + '.x64.json', 'wb') as f:
    assert not set(env.keys()) & set(env_x64.keys()), 'dupe keys'
    json.dump({'env': collections.OrderedDict(env.items() + env_x64.items())},
              f)


def AddEnvSetup(files):
  """We need to generate this file in the same way that the "from pieces"
  script does, so pull that in here."""
  tempdir = tempfile.mkdtemp()
  os.makedirs(os.path.join(tempdir, 'win_sdk', 'bin'))
  GenerateSetEnvCmd(tempdir)
  files.append((os.path.join(tempdir, 'win_sdk', 'bin', 'SetEnv.cmd'),
                'win_sdk\\bin\\SetEnv.cmd'))
  files.append((os.path.join(tempdir, 'win_sdk', 'bin', 'SetEnv.x86.json'),
                'win_sdk\\bin\\SetEnv.x86.json'))
  files.append((os.path.join(tempdir, 'win_sdk', 'bin', 'SetEnv.x64.json'),
                'win_sdk\\bin\\SetEnv.x64.json'))
  vs_version_file = os.path.join(tempdir, 'VS_VERSION')
  with open(vs_version_file, 'wb') as version:
    print >>version, VS_VERSION
  files.append((vs_version_file, 'VS_VERSION'))


def RenameToSha1(output):
  """Determine the hash in the same way that the unzipper does to rename the
  # .zip file."""
  print 'Extracting to determine hash...'
  tempdir = tempfile.mkdtemp()
  old_dir = os.getcwd()
  os.chdir(tempdir)
  rel_dir = 'vs_files'
  with zipfile.ZipFile(
      os.path.join(old_dir, output), 'r', zipfile.ZIP_DEFLATED, True) as zf:
    zf.extractall(rel_dir)
  print 'Hashing...'
  sha1 = get_toolchain_if_necessary.CalculateHash(rel_dir, None)
  os.chdir(old_dir)
  shutil.rmtree(tempdir)
  final_name = sha1 + '.zip'
  os.rename(output, final_name)
  print 'Renamed %s to %s.' % (output, final_name)


def main():
  usage = 'usage: %prog [options] 2015|2017'
  parser = optparse.OptionParser(usage)
  parser.add_option('-w', '--winver', action='store', type='string',
                    dest='winver', default='10.0.14393.0',
                    help='Windows SDK version, such as 10.0.14393.0')
  parser.add_option('-d', '--dryrun', action='store_true', dest='dryrun',
                    default=False,
                    help='scan for file existence and prints statistics')
  parser.add_option('--override', action='store', type='string',
                    dest='override_dir', default=None,
                    help='Specify alternate bin/include/lib directory')
  parser.add_option('--repackage', action='store', type='string',
                    dest='repackage_dir', default=None,
                    help='Specify raw directory to be packaged, for hot fixes.')
  (options, args) = parser.parse_args()

  if options.repackage_dir:
    files = BuildRepackageFileList(options.repackage_dir)
  else:
    if len(args) != 1 or args[0] not in ('2015', '2017'):
      print 'Must specify 2015 or 2017'
      parser.print_help();
      return 1

    if options.override_dir:
      if (not os.path.exists(os.path.join(options.override_dir, 'bin')) or
          not os.path.exists(os.path.join(options.override_dir, 'include')) or
          not os.path.exists(os.path.join(options.override_dir, 'lib'))):
        print 'Invalid override directory - must contain bin/include/lib dirs'
        return 1

    global VS_VERSION
    VS_VERSION = args[0]
    global WIN_VERSION
    WIN_VERSION = options.winver
    global VC_TOOLS
    if VS_VERSION == '2017':
      vs_path = GetVSPath()
      temp_tools_path = ExpandWildcards(vs_path, 'VC/Tools/MSVC/14.*.*')
      # Strip off the leading vs_path characters and switch back to / separators.
      VC_TOOLS = temp_tools_path[len(vs_path) + 1:].replace('\\', '/')
    else:
      VC_TOOLS = 'VC'

    print 'Building file list for VS %s Windows %s...' % (VS_VERSION, WIN_VERSION)
    files = BuildFileList(options.override_dir)

    AddEnvSetup(files)

  if False:
    for f in files:
      print f[0], '->', f[1]
    return 0

  output = 'out.zip'
  if os.path.exists(output):
    os.unlink(output)
  count = 0
  version_match_count = 0
  total_size = 0
  missing_files = False
  with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, True) as zf:
    for disk_name, archive_name in files:
      sys.stdout.write('\r%d/%d ...%s' % (count, len(files), disk_name[-40:]))
      sys.stdout.flush()
      count += 1
      if not options.repackage_dir and disk_name.count(WIN_VERSION) > 0:
        version_match_count += 1
      if os.path.exists(disk_name):
        if options.dryrun:
          total_size += os.path.getsize(disk_name)
        else:
          zf.write(disk_name, archive_name)
      else:
        missing_files = True
        sys.stdout.write('\r%s does not exist.\n\n' % disk_name)
        sys.stdout.flush()
  if options.dryrun:
    sys.stdout.write('\r%1.3f GB of data in %d files, %d files for %s.%s\n' %
        (total_size / 1e9, count, version_match_count, WIN_VERSION, ' '*50))
    return 0
  if missing_files:
    raise Exception('One or more files were missing - aborting')
  if not options.repackage_dir and version_match_count == 0:
    raise Exception('No files found that match the specified winversion')
  sys.stdout.write('\rWrote to %s.%s\n' % (output, ' '*50))
  sys.stdout.flush()

  RenameToSha1(output)

  return 0


if __name__ == '__main__':
  sys.exit(main())
