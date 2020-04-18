#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import getopt
import sys
import subprocess
import re
import os

as32 = '/third_party/gnu_binutils/files/as.exe'
as64 = '/third_party/mingw-w64/mingw/bin/x86_64-w64-mingw32-as.exe'

def main(argv):
  try:
    (opts, args) = getopt.getopt(argv[1:], 'a:p:o:')

    nacl_build_subarch = 0
    nacl_path = os.getcwd()
    output_filename = None

    for opt, val in opts:
      if opt == '-o':
        output_filename = val
      elif opt == '-a':
        if val.lower() == 'win32':
          nacl_build_arch = 'x86'
          nacl_build_subarch = 32
          as_exe = as32
        elif val.lower() == 'x64':
          nacl_build_arch = 'x86'
          nacl_build_subarch = 64
          as_exe = as64
        else:
          raise getopt.error('Unknown target architecture ' + val)
      elif opt == '-p':
        nacl_path = (val)

    if nacl_build_subarch == 0:
      raise getopt.error( 'You must specify a build architecture: '
                          + 'either -a Win32 or -a x64' )

    if output_filename is None:
      raise getopt.error( 'No output file specified' )

    for filename in args:
      # normalize the filename to avoid any DOS-type funkiness
      filename = os.path.abspath(filename)
      filename = filename.replace('\\', '/')

      #
      # Run the C compiler as a preprocessor and pipe the output into a string
      #
      cl_command = ['cl.exe',
                    '/nologo',
                    '/D__ASSEMBLER__',
                    '/DNACL_BUILD_ARCH=' + nacl_build_arch,
                    '/DNACL_BUILD_SUBARCH=' + str(nacl_build_subarch),
                    '/DNACL_WINDOWS=1',
                    '/TP',
                    '/E',
                    '/I' + nacl_path,
                    filename]
      p = subprocess.Popen(cl_command,
                           shell=True,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE)
      cl_results = p.communicate()
      cl_status = p.returncode
      cl_output = cl_results[0]
      cl_errors = cl_results[1]

      if cl_status != 0:
        print >>sys.stderr, 'FAILED: %s\n%s\n' % (' '.join(cl_command),
                                                  cl_errors)
        return cl_status

      #
      # Uncomment this if you need to see exactly what the MSVC preprocessor
      # has done to your input file.
      #print >>sys.stderr, '-------------------------------------'
      #print >>sys.stderr, '# PREPROCESSOR OUTPUT BEGINS        #'
      #print >>sys.stderr, '-------------------------------------'
      #print >>sys.stderr, cl_output
      #print >>sys.stderr, '-------------------------------------'
      #print >>sys.stderr, '# PREPROCESSOR OUTPUT ENDS          #'
      #print >>sys.stderr, '-------------------------------------'

      # GNU uses '#<linenum> for line number directives; MSVC uses
      # '#line <linenum>.'
      foo = re.compile(r'^#line ', re.MULTILINE)
      cl_output = foo.sub(r'#', cl_output)

      #
      # Pipe the preprocessor output into the assembler
      #
      as_command = [nacl_path + as_exe,
                    '-defsym','@feat.00=1',
                    '--' + str(nacl_build_subarch),
                    '-o', output_filename]
      p = subprocess.Popen(as_command,
                           stdin=subprocess.PIPE,
                           stderr=subprocess.PIPE)
      as_output, as_error = p.communicate(cl_output)
      as_status = p.returncode

      #
      # massage the assembler stderr into a format that Visual Studio likes
      #
      as_error = re.sub(r'\{standard input\}', filename, as_error)
      as_error = re.sub(r':([0-9]+):', r'(\1) :', as_error)
      as_error = re.sub(r'Error', 'error', as_error)
      as_error = re.sub(r'Warning', 'warning', as_error)

      if as_error:
        print >>sys.stderr, as_error

      if as_status != 0:
        print >>sys.stderr, 'FAILED: %s\n' % ' '.join(as_command)
        return as_status

  except getopt.error, e:
    print >>sys.stderr, str(e)
    print >>sys.stderr, ['Usage: ',
      argv[0],
      '-a {Win32|x64} -o output_file [-p native_client_path] input_file']


if __name__ == '__main__':
  sys.exit(main(sys.argv))
