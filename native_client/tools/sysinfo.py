#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Simple system info tool for bug reports, etc.

"""



# More info at:
#
# http://code.activestate.com/recipes/511491/
# http://www.koders.com/python/fidB436B8043AA994C550C0961247DACC3E04E84734.aspx?s=config
# http://developer.apple.com/documentation/Darwin/Reference/ManPages/man8/sysctl.8.html

# imports
import os
import sys
import time


VERBOSE = 0


def Banner(text):
  print '=' * 70
  print text
  print '=' * 70
  # quick hack to keep banner in sync with os.system output
  sys.stdout.flush()


def InfoLinux():
  Banner('OS:')
  os.system('uname -a')

  Banner('CPU:')
  if VERBOSE:
    os.system('cat /proc/cpuinfo')
  else:
    os.system("egrep 'name|MHz|stepping' /proc/cpuinfo")

  Banner('RAM:')
  if VERBOSE:
    os.system('cat /proc/meminfo')
  else:
    os.system('cat /proc/meminfo | egrep Mem')

  Banner('LOAD:')
  os.system('cat /proc/loadavg')

  Banner('UPTIME:')
  os.system('cat /proc/uptime')


def InfoDarwin():
  Banner('OS:')
  os.system('sysctl kern | egrep "kern\.os|version"')

  Banner('CPU:')
  os.system('sysctl hw.machine')
  os.system('sysctl hw.model')
  os.system('sysctl hw.ncpu')
  if VERBOSE:
    os.system("sysctl hw")
  else:
    os.system("sysctl hw | egrep 'cpu'")

  Banner('RAM:')
  if VERBOSE:
    os.system("sysctl hw")
  else:
    os.system("sysctl hw | egrep 'mem'")

  Banner('LOAD:')
  print 'TBD'

  Banner('UPTIME:')
  os.system('sysctl kern | egrep "kern\.boottime"')


def InfoWin32():
  import _winreg

  def GetRegistryOS( value):
    db = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE,
                         'SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion')

    return _winreg.QueryValueEx(db, value)[0]

  Banner('OS:')
  for key in ['ProductName',
              'CSDVersion',
              'CurrentBuildNumber']:
    print  GetRegistryOS(key)

  Banner('CPU:')
  db = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE,
                       'HARDWARE\\DESCRIPTION\\System\\CentralProcessor')
  for n in range(0, 1000):
     try:
       cpu = _winreg.EnumKey(db, n)
     except Exception:
       break
     print "\nProcessor :", cpu
     db_cpu = _winreg.OpenKey(db, cpu)
     for i in range(0, 1000):
       try:
         name, value, type =_winreg.EnumValue(db_cpu, i)
       except Exception:
         break
       # skip binary data
       if type == _winreg.REG_BINARY: continue
       if type == _winreg.REG_FULL_RESOURCE_DESCRIPTOR: continue
       print name, type, value

  Banner('RAM:')
  print 'TBD'
  # TODO: this is currently broken since ctypes is not available

  Banner('LOAD:')
  print 'TBD'

  Banner('UPTIME:')
  print 'TBD'


PLATFORM_INFO = {
    'linux2': InfoLinux,
    'linux3': InfoLinux,
    'darwin': InfoDarwin,
    'win32': InfoWin32,
    }


def main():
  Banner('Python Info:')
  print sys.platform
  print sys.version

  Banner('ENV:')
  for e in ['PATH']:
    print e, os.getenv(e)


  if sys.platform in PLATFORM_INFO:
    try:
      PLATFORM_INFO[sys.platform]()
    except Exception, err:
      print 'ERRROR: processing sys info', str(err)
  else:
    print 'ERROR: unknwon platform', system.platform

  return 0


sys.exit(main())
