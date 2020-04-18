# Author: Trevor Perrin
# See the LICENSE file for legal information regarding use of this file.

"""TLS Lite + smtplib."""

from smtplib import SMTP
from tlslite.tlsconnection import TLSConnection
from tlslite.integration.clienthelper import ClientHelper

class SMTP_TLS(SMTP):
    """This class extends L{smtplib.SMTP} with TLS support."""

    def starttls(self,
                 username=None, password=None,
                 certChain=None, privateKey=None,
                 checker=None,
                 settings=None):
        """Puts the connection to the SMTP server into TLS mode.

        If the server supports TLS, this will encrypt the rest of the SMTP
        session.

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
        (resp, reply) = self.docmd("STARTTLS")
        if resp == 220:
            helper = ClientHelper(
                     username, password, 
                     certChain, privateKey,
                     checker,
                     settings)
            conn = TLSConnection(self.sock)
            helper._handshake(conn)
            self.sock = conn
            self.file = conn.makefile('rb')
        return (resp, reply)