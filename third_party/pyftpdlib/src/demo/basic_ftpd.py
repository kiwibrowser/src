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

"""A basic FTP server which uses a DummyAuthorizer for managing 'virtual
users', setting a limit for incoming connections.
"""

import os

from pyftpdlib import ftpserver


def main():
    # Instantiate a dummy authorizer for managing 'virtual' users
    authorizer = ftpserver.DummyAuthorizer()

    # Define a new user having full r/w permissions and a read-only
    # anonymous user
    authorizer.add_user('user', '12345', os.getcwd(), perm='elradfmwM')
    authorizer.add_anonymous(os.getcwd())

    # Instantiate FTP handler class
    ftp_handler = ftpserver.FTPHandler
    ftp_handler.authorizer = authorizer

    # Define a customized banner (string returned when client connects)
    ftp_handler.banner = "pyftpdlib %s based ftpd ready." %ftpserver.__ver__

    # Specify a masquerade address and the range of ports to use for
    # passive connections.  Decomment in case you're behind a NAT.
    #ftp_handler.masquerade_address = '151.25.42.11'
    #ftp_handler.passive_ports = range(60000, 65535)

    # Instantiate FTP server class and listen to 0.0.0.0:21
    address = ('', 21)
    ftpd = ftpserver.FTPServer(address, ftp_handler)

    # set a limit for connections
    ftpd.max_cons = 256
    ftpd.max_cons_per_ip = 5

    # start ftp server
    ftpd.serve_forever()

if __name__ == '__main__':
    main()
