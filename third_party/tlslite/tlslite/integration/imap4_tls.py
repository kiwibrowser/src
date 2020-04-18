# Author: Trevor Perrin
# See the LICENSE file for legal information regarding use of this file.

"""TLS Lite + imaplib."""

import socket
from imaplib import IMAP4
from tlslite.tlsconnection import TLSConnection
from tlslite.integration.clienthelper import ClientHelper

# IMAP TLS PORT
IMAP4_TLS_PORT = 993

class IMAP4_TLS(IMAP4, ClientHelper):
    """This class extends L{imaplib.IMAP4} with TLS support."""

    def __init__(self, host = '', port = IMAP4_TLS_PORT,
                 username=None, password=None,
                 certChain=None, privateKey=None,
                 checker=None,
                 settings=None):
        """Create a new IMAP4_TLS.

        For client authentication, use one of these argument
        combinations:
         - username, password (SRP)
         - certChain, privateKey (certificate)

        For server authentication, you can either rely on the
        implicit mutual authentication performed by SRP
        or you can do certificate-based server
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
        @param username: SRP username.  Requires the
        'password' argument.

        @type password: str
        @param password: SRP password for mutual authentication.
        Requires the 'username' argument.

        @type certChain: L{tlslite.x509certchain.X509CertChain}
        @param certChain: Certificate chain for client authentication.
        Requires the 'privateKey' argument.  Excludes the SRP arguments.

        @type privateKey: L{tlslite.utils.rsakey.RSAKey}
        @param privateKey: Private key for client authentication.
        Requires the 'certChain' argument.  Excludes the SRP arguments.
        
        @type checker: L{tlslite.checker.Checker}
        @param checker: Callable object called after handshaking to 
        evaluate the connection and raise an Exception if necessary.

        @type settings: L{tlslite.handshakesettings.HandshakeSettings}
        @param settings: Various settings which can be used to control
        the ciphersuites, certificate types, and SSL/TLS versions
        offered by the client.
        """

        ClientHelper.__init__(self,
                 username, password,
                 certChain, privateKey,
                 checker,
                 settings)

        IMAP4.__init__(self, host, port)


    def open(self, host = '', port = IMAP4_TLS_PORT):
        """Setup connection to remote server on "host:port".

        This connection will be used by the routines:
        read, readline, send, shutdown.
        """
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))
        self.sock = TLSConnection(self.sock)
        ClientHelper._handshake(self, self.sock)
        self.file = self.sock.makefile('rb')