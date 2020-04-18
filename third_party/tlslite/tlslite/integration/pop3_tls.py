# Author: Trevor Perrin
# See the LICENSE file for legal information regarding use of this file.

"""TLS Lite + poplib."""

import socket
from poplib import POP3, POP3_SSL_PORT
from tlslite.tlsconnection import TLSConnection
from tlslite.integration.clienthelper import ClientHelper

class POP3_TLS(POP3, ClientHelper):
    """This class extends L{poplib.POP3} with TLS support."""

    def __init__(self, host, port = POP3_SSL_PORT,
                 timeout=socket._GLOBAL_DEFAULT_TIMEOUT,
                 username=None, password=None,
                 certChain=None, privateKey=None,
                 checker=None,
                 settings=None):
        """Create a new POP3_TLS.

        For client authentication, use one of these argument
        combinations:
         - username, password (SRP)
         - certChain, privateKey (certificate)

        For server authentication, you can either rely on the
        implicit mutual authentication performed by SRP or
        you can do certificate-based server
        authentication with one of these argument combinations:
         - x509Fingerprint

        Certificate-based server authentication is compatible with
        SRP or certificate-based client authentication.

        The caller should be prepared to handle TLS-specific
        exceptions.  See the client handshake functions in
        L{tlslite.TLSConnection.TLSConnection} for details on which
        exceptions might be raised.

        @type host: str
        @param host: Server to connect to.

        @type port: int
        @param port: Port to connect to.

        @type username: str
        @param username: SRP username.
        
        @type password: str
        @param password: SRP password for mutual authentication.
        Requires the 'username' argument.

        @type certChain: L{tlslite.x509certchain.X509CertChain}
        @param certChain: Certificate chain for client authentication.
        Requires the 'privateKey' argument.  Excludes the SRP argument.

        @type privateKey: L{tlslite.utils.rsakey.RSAKey}
        @param privateKey: Private key for client authentication.
        Requires the 'certChain' argument.  Excludes the SRP argument.

        @type checker: L{tlslite.checker.Checker}
        @param checker: Callable object called after handshaking to 
        evaluate the connection and raise an Exception if necessary.

        @type settings: L{tlslite.handshakesettings.HandshakeSettings}
        @param settings: Various settings which can be used to control
        the ciphersuites, certificate types, and SSL/TLS versions
        offered by the client.
        """
        self.host = host
        self.port = port
        sock = socket.create_connection((host, port), timeout)
        ClientHelper.__init__(self,
                 username, password,
                 certChain, privateKey,
                 checker,
                 settings)
        connection = TLSConnection(sock) 
        ClientHelper._handshake(self, connection)
        self.sock = connection
        self.file = self.sock.makefile('rb')
        self._debugging = 0
        self.welcome = self._getresp()