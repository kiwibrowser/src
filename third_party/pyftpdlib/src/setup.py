#!/usr/bin/env python
# $Id$
#
#  pyftpdlib is released under the MIT license, reproduced below:
#  ======================================================================
#  Copyright (C) 2007-2012 Giampaolo Rodola' <g.rodola@gmail.com>
#
#                         All Rights Reserved
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
#  ======================================================================

"""pyftpdlib installer.

To install pyftpdlib just open a command shell and run:
> python setup.py install
"""

import os
import sys
try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup

name = 'pyftpdlib'
version = '0.7.0'
download_url = "http://pyftpdlib.googlecode.com/files/" + name + "-" + \
                                                          version + ".tar.gz"

setup(
    name=name,
    version=version,
    description='High-level asynchronous FTP server library',
    long_description="Python FTP server library provides an high-level portable "
                     "interface to easily write asynchronous FTP servers with "
                     "Python.",
    license='License :: OSI Approved :: MIT License',
    platforms='Platform Independent',
    author="Giampaolo Rodola'",
    author_email='g.rodola@gmail.com',
    url='http://code.google.com/p/pyftpdlib/',
    download_url=download_url,
    packages=['pyftpdlib', 'pyftpdlib/contrib'],
    keywords=['ftp', 'ftps', 'server', 'ftpd', 'daemon', 'python', 'ssl',
              'sendfile', 'rfc959', 'rfc1123', 'rfc2228', 'rfc2428', 'rfc3659'],
    classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Environment :: Console',
          'Intended Audience :: Developers',
          'Intended Audience :: System Administrators',
          'License :: OSI Approved :: MIT License',
          'Operating System :: OS Independent',
          'Programming Language :: Python',
          'Topic :: Internet :: File Transfer Protocol (FTP)',
          'Topic :: Software Development :: Libraries :: Python Modules',
          'Topic :: System :: Filesystems',
          'Programming Language :: Python',
          'Programming Language :: Python :: 2',
          'Programming Language :: Python :: 2.4',
          'Programming Language :: Python :: 2.5',
          'Programming Language :: Python :: 2.6',
          'Programming Language :: Python :: 2.7',
          ],
    )

if os.name == 'posix':
    try:
        import sendfile
    except ImportError:
        msg = "\nYou might want to install pysendfile module to speedup " \
              "transfers:\nhttp://code.google.com/p/pysendfile/\n"
        if sys.stderr.isatty():
            sys.stderr.write('\x1b[1m%s\x1b[0m' % msg)
        else:
            sys.stderr.write(msg)
