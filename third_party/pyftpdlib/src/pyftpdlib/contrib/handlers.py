#!/usr/bin/env python
# $Id$

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


"""This module is supposed to contain a series of classes which extend
base pyftpdlib.ftpserver's FTPHandler and DTPHandler classes.

As for now only one class is provided: TLS_FTPHandler.
It is supposed to provide basic support for FTPS (FTP over SSL/TLS) as
described in RFC-4217.

Requires PyOpenSSL module (http://pypi.python.org/pypi/pyOpenSSL).
For Python versions prior to 2.6 ssl module must be installed separately,
see: http://pypi.python.org/pypi/ssl/

Development status: experimental.
"""


import os
import asyncore
import socket
import warnings
import errno

from pyftpdlib.ftpserver import FTPHandler, DTPHandler, proto_cmds, _DISCONNECTED

__all__ = []

# requires PyOpenSSL - http://pypi.python.org/pypi/pyOpenSSL
try:
    from OpenSSL import SSL
except ImportError:
    pass
else:
    __all__.extend(['SSLConnection', 'TLS_FTPHandler', 'TLS_DTPHandler'])


    new_proto_cmds = proto_cmds.copy()
    new_proto_cmds.update({
        'AUTH': dict(perm=None, auth=False, arg=True,
                     help='Syntax: AUTH <SP> TLS|SSL (set up secure control channel).'),
        'PBSZ': dict(perm=None, auth=False,  arg=True,
                     help='Syntax: PBSZ <SP> 0 (negotiate TLS buffer).'),
        'PROT': dict(perm=None, auth=False,  arg=True,
                     help='Syntax: PROT <SP> [C|P] (set up un/secure data channel).'),
        })


    class SSLConnection(object, asyncore.dispatcher):
        """An asyncore.dispatcher subclass supporting TLS/SSL."""

        _ssl_accepting = False
        _ssl_established = False
        _ssl_closing = False

        def __init__(self, *args, **kwargs):
            super(SSLConnection, self).__init__(*args, **kwargs)
            self._error = False

        def secure_connection(self, ssl_context):
            """Secure the connection switching from plain-text to
            SSL/TLS.
            """
            try:
                self.socket = SSL.Connection(ssl_context, self.socket)
            except socket.error:
                self.close()
            else:
                self.socket.set_accept_state()
                self._ssl_accepting = True

        def _do_ssl_handshake(self):
            self._ssl_accepting = True
            try:
                self.socket.do_handshake()
            except (SSL.WantReadError, SSL.WantWriteError):
                return
            except SSL.SysCallError, (retval, desc):
                if (retval == -1 and desc == 'Unexpected EOF') or retval > 0:
                    return self.handle_close()
                raise
            except SSL.Error:
                return self.handle_failed_ssl_handshake()
            else:
                self._ssl_accepting = False
                self._ssl_established = True
                self.handle_ssl_established()

        def handle_ssl_established(self):
            """Called when SSL handshake has completed."""
            pass

        def handle_ssl_shutdown(self):
            """Called when SSL shutdown() has completed."""
            super(SSLConnection, self).close()

        def handle_failed_ssl_handshake(self):
            raise NotImplementedError("must be implemented in subclass")

        def handle_read_event(self):
            if self._ssl_accepting:
                self._do_ssl_handshake()
            elif self._ssl_closing:
                self._do_ssl_shutdown()
            else:
                super(SSLConnection, self).handle_read_event()

        def handle_write_event(self):
            if self._ssl_accepting:
                self._do_ssl_handshake()
            elif self._ssl_closing:
                self._do_ssl_shutdown()
            else:
                super(SSLConnection, self).handle_write_event()

        def handle_error(self):
            self._error = True
            try:
                raise
            except (KeyboardInterrupt, SystemExit, asyncore.ExitNow):
                raise
            except:
                self.log_exception(self)
            # when facing an unhandled exception in here it's better
            # to rely on base class (FTPHandler or DTPHandler)
            # close() method as it does not imply SSL shutdown logic
            super(SSLConnection, self).close()

        def send(self, data):
            try:
                return super(SSLConnection, self).send(data)
            except (SSL.WantReadError, SSL.WantWriteError):
                return 0
            except SSL.ZeroReturnError:
                super(SSLConnection, self).handle_close()
                return 0
            except SSL.SysCallError, (errnum, errstr):
                if errnum == errno.EWOULDBLOCK:
                    return 0
                elif errnum in _DISCONNECTED or errstr == 'Unexpected EOF':
                    super(SSLConnection, self).handle_close()
                    return 0
                else:
                    raise

        def recv(self, buffer_size):
            try:
                return super(SSLConnection, self).recv(buffer_size)
            except (SSL.WantReadError, SSL.WantWriteError):
                return ''
            except SSL.ZeroReturnError:
                super(SSLConnection, self).handle_close()
                return ''
            except SSL.SysCallError, (errnum, errstr):
                if errnum in _DISCONNECTED or errstr == 'Unexpected EOF':
                    super(SSLConnection, self).handle_close()
                    return ''
                else:
                    raise

        def _do_ssl_shutdown(self):
            """Executes a SSL_shutdown() call to revert the connection
            back to clear-text.
            twisted/internet/tcp.py code has been used as an example.
            """
            self._ssl_closing = True
            # since SSL_shutdown() doesn't report errors, an empty
            # write call is done first, to try to detect if the
            # connection has gone away
            try:
                os.write(self.socket.fileno(), '')
            except (OSError, socket.error), err:
                if err.args[0] in (errno.EINTR, errno.EWOULDBLOCK, errno.ENOBUFS):
                    return
                elif err.args[0] in _DISCONNECTED:
                    return super(SSLConnection, self).close()
                else:
                    raise
            # Ok, this a mess, but the underlying OpenSSL API simply
            # *SUCKS* and I really couldn't do any better.
            #
            # Here we just want to shutdown() the SSL layer and then
            # close() the connection so we're not interested in a
            # complete SSL shutdown() handshake, so let's pretend
            # we already received a "RECEIVED" shutdown notification
            # from the client.
            # Once the client received our "SENT" shutdown notification
            # then we close() the connection.
            #
            # Since it is not clear what errors to expect during the
            # entire procedure we catch them all and assume the
            # following:
            # - WantReadError and WantWriteError means "retry"
            # - ZeroReturnError, SysCallError[EOF], Error[] are all
            #   aliases for disconnection
            try:
                laststate = self.socket.get_shutdown()
                self.socket.set_shutdown(laststate | SSL.RECEIVED_SHUTDOWN)
                done = self.socket.shutdown()
                if not (laststate & SSL.RECEIVED_SHUTDOWN):
                    self.socket.set_shutdown(SSL.SENT_SHUTDOWN)
            except (SSL.WantReadError, SSL.WantWriteError):
                pass
            except SSL.ZeroReturnError:
                super(SSLConnection, self).close()
            except SSL.SysCallError, (errnum, errstr):
                if errnum in _DISCONNECTED or errstr == 'Unexpected EOF':
                    super(SSLConnection, self).close()
                else:
                    raise
            except SSL.Error, err:
                # see:
                # http://code.google.com/p/pyftpdlib/issues/detail?id=171
                # https://bugs.launchpad.net/pyopenssl/+bug/785985
                if err.args and not err.args[0]:
                    pass
                else:
                    raise
            except socket.error, err:
                if err.args[0] in _DISCONNECTED:
                    super(SSLConnection, self).close()
                else:
                    raise
            else:
                if done:
                    self._ssl_established = False
                    self._ssl_closing = False
                    self.handle_ssl_shutdown()

        def close(self):
            if self._ssl_established and not self._error:
                self._do_ssl_shutdown()
            else:
                self._ssl_accepting = False
                self._ssl_established = False
                self._ssl_closing = False
                super(SSLConnection, self).close()


    class TLS_DTPHandler(SSLConnection, DTPHandler):
        """A ftpserver.DTPHandler subclass supporting TLS/SSL."""

        def __init__(self, sock_obj, cmd_channel):
            super(TLS_DTPHandler, self).__init__(sock_obj, cmd_channel)
            if self.cmd_channel._prot:
                self.secure_connection(self.cmd_channel.ssl_context)

        def _use_sendfile(self, producer):
            return False

        def handle_failed_ssl_handshake(self):
            # TLS/SSL handshake failure, probably client's fault which
            # used a SSL version different from server's.
            # RFC-4217, chapter 10.2 expects us to return 522 over the
            # command channel.
            self.cmd_channel.respond("522 SSL handshake failed.")
            self.cmd_channel.log_cmd("PROT", "P", 522, "SSL handshake failed.")
            self.close()


    class TLS_FTPHandler(SSLConnection, FTPHandler):
        """A ftpserver.FTPHandler subclass supporting TLS/SSL.
        Implements AUTH, PBSZ and PROT commands (RFC-2228 and RFC-4217).

        Configurable attributes:

         - (bool) tls_control_required:
            When True requires SSL/TLS to be established on the control
            channel, before logging in.  This means the user will have
            to issue AUTH before USER/PASS (default False).

         - (bool) tls_data_required:
            When True requires SSL/TLS to be established on the data
            channel.  This means the user will have to issue PROT
            before PASV or PORT (default False).

        SSL-specific options:

         - (string) certfile:
            the path to the file which contains a certificate to be
            used to identify the local side of the connection.
            This  must always be specified, unless context is provided
            instead.

         - (string) keyfile:
            the path to the file containing the private RSA key;
            can be omittetted if certfile already contains the
            private key (defaults: None).

         - (int) protocol:
            specifies which version of the SSL protocol to use when
            establishing SSL/TLS sessions; clients can then only
            connect using the configured protocol (defaults to SSLv23,
            allowing SSLv3 and TLSv1 protocols).

            Possible values:
            * SSL.SSLv2_METHOD - allow only SSLv2
            * SSL.SSLv3_METHOD - allow only SSLv3
            * SSL.SSLv23_METHOD - allow both SSLv3 and TLSv1
            * SSL.TLSv1_METHOD - allow only TLSv1

          - (instance) context:
            a SSL Context object previously configured; if specified
            all other parameters will be ignored.
            (default None).
        """

        # configurable attributes
        tls_control_required = False
        tls_data_required = False
        certfile = None
        keyfile = None
        ssl_protocol = SSL.SSLv23_METHOD
        ssl_context = None

        # overridden attributes
        proto_cmds = new_proto_cmds
        dtp_handler = TLS_DTPHandler

        def __init__(self, conn, server):
            super(TLS_FTPHandler, self).__init__(conn, server)
            if not self.connected:
                return
            self._extra_feats = ['AUTH TLS', 'AUTH SSL', 'PBSZ', 'PROT']
            self._pbsz = False
            self._prot = False
            self.ssl_context = self.get_ssl_context()

        @classmethod
        def get_ssl_context(cls):
            if cls.ssl_context is None:
                if cls.certfile is None:
                    raise ValueError("at least certfile must be specified")
                cls.ssl_context = SSL.Context(cls.ssl_protocol)
                if cls.ssl_protocol != SSL.SSLv2_METHOD:
                    cls.ssl_context.set_options(SSL.OP_NO_SSLv2)
                else:
                    warnings.warn("SSLv2 protocol is insecure", RuntimeWarning)
                cls.ssl_context.use_certificate_file(cls.certfile)
                if not cls.keyfile:
                    cls.keyfile = cls.certfile
                cls.ssl_context.use_privatekey_file(cls.keyfile)
                TLS_FTPHandler.ssl_context = cls.ssl_context
            return cls.ssl_context

        # --- overridden methods

        def flush_account(self):
            FTPHandler.flush_account(self)
            self._pbsz = False
            self._prot = False

        def process_command(self, cmd, *args, **kwargs):
            if cmd in ('USER', 'PASS'):
                if self.tls_control_required and not self._ssl_established:
                    msg = "SSL/TLS required on the control channel."
                    self.respond("550 " + msg)
                    self.log_cmd(cmd, args[0], 550, msg)
                    return
            elif cmd in ('PASV', 'EPSV', 'PORT', 'EPRT'):
                if self.tls_data_required and not self._prot:
                    msg = "SSL/TLS required on the data channel."
                    self.respond("550 " + msg)
                    self.log_cmd(cmd, args[0], 550, msg)
                    return
            FTPHandler.process_command(self, cmd, *args, **kwargs)

        # --- new methods

        def handle_failed_ssl_handshake(self):
            # TLS/SSL handshake failure, probably client's fault which
            # used a SSL version different from server's.
            # We can't rely on the control connection anymore so we just
            # disconnect the client without sending any response.
            self.log("SSL handshake failed.")
            self.close()

        def ftp_AUTH(self, line):
            """Set up secure control channel."""
            arg = line.upper()
            if isinstance(self.socket, SSL.Connection):
                self.respond("503 Already using TLS.")
            elif arg in ('TLS', 'TLS-C', 'SSL', 'TLS-P'):
                # From RFC-4217: "As the SSL/TLS protocols self-negotiate
                # their levels, there is no need to distinguish between SSL
                # and TLS in the application layer".
                self.respond('234 AUTH %s successful.' %arg)
                self.secure_connection(self.ssl_context)
            else:
                self.respond("502 Unrecognized encryption type (use TLS or SSL).")

        def ftp_PBSZ(self, line):
            """Negotiate size of buffer for secure data transfer.
            For TLS/SSL the only valid value for the parameter is '0'.
            Any other value is accepted but ignored.
            """
            if not isinstance(self.socket, SSL.Connection):
                self.respond("503 PBSZ not allowed on insecure control connection.")
            else:
                self.respond('200 PBSZ=0 successful.')
                self._pbsz = True

        def ftp_PROT(self, line):
            """Setup un/secure data channel."""
            arg = line.upper()
            if not isinstance(self.socket, SSL.Connection):
                self.respond("503 PROT not allowed on insecure control connection.")
            elif not self._pbsz:
                self.respond("503 You must issue the PBSZ command prior to PROT.")
            elif arg == 'C':
                self.respond('200 Protection set to Clear')
                self._prot = False
            elif arg == 'P':
                self.respond('200 Protection set to Private')
                self._prot = True
            elif arg in ('S', 'E'):
                self.respond('521 PROT %s unsupported (use C or P).' %arg)
            else:
                self.respond("502 Unrecognized PROT type (use C or P).")
