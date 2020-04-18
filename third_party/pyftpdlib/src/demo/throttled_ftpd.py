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

"""An FTP server which uses the ThrottledDTPHandler class for limiting the
speed of downloads and uploads.
"""

import os

from pyftpdlib import ftpserver


def main():
    authorizer = ftpserver.DummyAuthorizer()
    authorizer.add_user('user', '12345', os.getcwd(), perm='elradfmw')
    authorizer.add_anonymous(os.getcwd())

    dtp_handler = ftpserver.ThrottledDTPHandler
    dtp_handler.read_limit = 30720  # 30 Kb/sec (30 * 1024)
    dtp_handler.write_limit = 30720  # 30 Kb/sec (30 * 1024)

    ftp_handler = ftpserver.FTPHandler
    ftp_handler.authorizer = authorizer
    # have the ftp handler use the alternative dtp handler class
    ftp_handler.dtp_handler = dtp_handler

    ftpd = ftpserver.FTPServer(('', 21), ftp_handler)
    ftpd.serve_forever()

if __name__ == '__main__':
    main()
