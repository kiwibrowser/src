#!/usr/bin/env python
# $Id$

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

from pyftpdlib.ftpserver import AbstractedFS

__all__ = ['UnixFilesystem']


class UnixFilesystem(AbstractedFS):
    """Represents the real UNIX filesystem.

    Differently from AbstractedFS the client will login into
    /home/<username> and will be able to escape its home directory
    and navigate the real filesystem.
    """

    def __init__(self, root, cmd_channel):
        AbstractedFS.__init__(self, root, cmd_channel)
        # initial cwd was set to "/" to emulate a chroot jail
        self.cwd = root

    def ftp2fs(self, ftppath):
        return self.ftpnorm(ftppath)

    def fs2ftp(self, fspath):
        return fspath

    def validpath(self, path):
        # validpath was used to check symlinks escaping user home
        # directory; this is no longer necessary.
        return True
